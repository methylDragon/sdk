# Copyright 2023 Intrinsic Innovation LLC

"""Entry point of the Intrinsic solution building libraries.

## Usage example

```
from intrinsic.solutions import deployments

solution = deployments.connect(...)
skills = solution.skills
executive = solution.executive

throw_ball = skills.throw_ball(...)
executive.run(throw_ball)
```
"""

import enum
import inspect
import os
import sys
from typing import Any, Dict, Optional, Union
import warnings

from google.protobuf import empty_pb2
import grpc
from intrinsic.frontend.cloud.api import solutiondiscovery_api_pb2
from intrinsic.frontend.cloud.api import solutiondiscovery_api_pb2_grpc
from intrinsic.kubernetes.workcell_spec.proto import installer_pb2
from intrinsic.kubernetes.workcell_spec.proto import installer_pb2_grpc
from intrinsic.resources.client import resource_registry_client
from intrinsic.skills.client import skill_registry_client
from intrinsic.solutions import auth
from intrinsic.solutions import dialerutil
from intrinsic.solutions import error_processing
from intrinsic.solutions import errors as solution_errors
from intrinsic.solutions import execution
from intrinsic.solutions import ipython
from intrinsic.solutions import pbt_registration
from intrinsic.solutions import pose_estimation
from intrinsic.solutions import proto_building
from intrinsic.solutions import providers
from intrinsic.solutions import simulation
from intrinsic.solutions import userconfig
from intrinsic.solutions import worlds
from intrinsic.solutions.internal import resources as resources_mod
from intrinsic.solutions.internal import skill_providing
from intrinsic.solutions.internal import stubs
from intrinsic.util.grpc import error_handling


_DEFAULT_HOSTPORT = "localhost:17080"
_XFA_BOX_ADDRESS_ENVIRONMENT_VAR = "XFA_BOX_ADDRESS"
_GRPC_OPTIONS = [
    # Remove limit on message size for e.g. images.
    ("grpc.max_receive_message_length", -1),
    ("grpc.max_send_message_length", -1),
]

# If an app is missing any of those services, the connect() method will raise an
# error.
_REQUIRED_BACKENDS = [
    "executive",
    "resource-registry",
    "skill-registry",
]

_CSS_FAILURE_STYLE = (
    "color: #ab0000; font-family: monospace; font-weight: bold; "
    "padding-left: var(--jp-code-padding);"
)

_WORLD_ID = "world"


class Solution:
  """Wraps a connection to a deployed solution.

  Attributes:
    grpc_channel: gRPC channel to the cluster which hosts the deployed solution.
    is_simulated: Whether the solution is deployed on a simulated workcell
      rather than on a physical workcell.
    executive: Executive instance to communicate with executive.
    skills: Wrapper to easily access skills.
    resources: Provides access to resources.
    simulator: Simulator instance for controlling simulation.
    errors: Exposes error reports from executions.
    pose_estimators: Optional. Wrapper to access pose estimators.
    world: default world in world service.
    pbt_registry: gRPC wrapper to sideload PBTs
    proto_builder: service to build proto FileDescriptorSets from proto schemas
  """

  class HealthStatus(enum.Enum):
    """Health status of the solution's backend."""

    UNKNOWN = 0
    # Ready to receive requests.
    HEALTHY = 1
    # Not ready to receive requests, but should fix itself.
    PENDING = 2
    # Non-recoverable error.
    ERROR = 3

  grpc_channel: grpc.Channel
  is_simulated: bool
  executive: execution.Executive
  resources: providers.ResourceProvider
  world: worlds.ObjectWorld
  simulator: Optional[simulation.Simulation]
  skills: providers.SkillProvider
  errors: error_processing.ErrorsLoader
  pose_estimators: Optional[pose_estimation.PoseEstimators]
  _skill_registry: skill_registry_client.SkillRegistryClient
  _resource_registry: resource_registry_client.ResourceRegistryClient
  _installer_service_stub: installer_pb2_grpc.InstallerServiceStub
  pbt_registry: Optional[pbt_registration.BehaviorTreeRegistry]
  proto_builder: Optional[proto_building.ProtoBuilder]

  def __init__(
      self,
      grpc_channel: grpc.Channel,
      is_simulated: bool,
      executive: execution.Executive,
      skill_registry: skill_registry_client.SkillRegistryClient,
      resource_registry: resource_registry_client.ResourceRegistryClient,
      object_world: worlds.ObjectWorld,
      simulator: Optional[simulation.Simulation],
      errors: error_processing.ErrorsLoader,
      pose_estimators: Optional[pose_estimation.PoseEstimators],
      installer: installer_pb2_grpc.InstallerServiceStub,
      pbt_registry: Optional[pbt_registration.BehaviorTreeRegistry] = None,
      proto_builder: Optional[proto_building.ProtoBuilder] = None,
  ):
    self.grpc_channel: grpc.Channel = grpc_channel
    self.is_simulated: bool = is_simulated

    self.executive = executive
    self._skill_registry = skill_registry
    self._resource_registry = resource_registry
    self.resources = resources_mod.Resources(self._resource_registry)

    self.world: worlds.ObjectWorld = object_world
    self.simulator: Optional[simulation.Simulation] = simulator
    self._installer_service_stub = installer

    self.skills = skill_providing.Skills(
        self._skill_registry,
        self._resource_registry,
    )
    self.pose_estimators = pose_estimators
    self.errors = errors
    self.pbt_registry = pbt_registry
    self.proto_builder = proto_builder

  @classmethod
  def for_channel(
      cls,
      grpc_channel: grpc.Channel,
      *,
      options: Optional[Dict[str, Any]] = None,
  ) -> "Solution":
    """Creates a Solution for the given channel and options.

    Args:
      grpc_channel: gRPC channel to the cluster which hosts the deployed
        solution.
      options: An optional Dict[str, Any] containing additional options. See
        'deployments.connect()' for available values.

    Returns:
      A fully initialized Workcell instance.
    """

    print("Connecting to deployed solution...")

    if options is None:
      options = {}

    installer_stub = installer_pb2_grpc.InstallerServiceStub(grpc_channel)
    try:
      solution_status = _get_solution_status_with_retry(installer_stub)
    except solution_errors.BackendNoWorkcellError as e:
      ipython.display_html_or_print_msg(
          f'<span style="{_CSS_FAILURE_STYLE}">{str(e)}</span>', str(e)
      )
      raise

    # Note that the ports of the services in the 'solution_status' are
    # irrelevant, as we use the Ingress by default.
    backend_names = [s.name for s in solution_status.services]

    missing = list(filter(lambda x: x not in backend_names, _REQUIRED_BACKENDS))

    if missing:
      raise solution_errors.NotFoundError(
          "n\nMissing backend.\nRequired backends: {}\nExisting backends: {}"
          "\nMissing: {}".format(
              ", ".join(_REQUIRED_BACKENDS),
              ", ".join(backend_names),
              missing,
          )
      )

    # Optional backends.
    simulator = None
    if solution_status.simulated:
      simulator = simulation.Simulation.connect(grpc_channel)

    # Required backends. (see _REQUIRED_BACKENDS)
    error_loader = error_processing.ErrorsLoader(installer_stub)
    executive = execution.Executive.connect(
        grpc_channel, error_loader, simulator
    )
    skill_registry = skill_registry_client.SkillRegistryClient.connect(
        grpc_channel
    )
    resource_registry = resource_registry_client.ResourceRegistryClient.connect(
        grpc_channel
    )

    object_world = worlds.ObjectWorld.connect(_WORLD_ID, grpc_channel)

    pose_estimators = None
    if "perception" in backend_names:
      pose_estimators = pose_estimation.PoseEstimators(
          resource_registry,
      )

    pbt_registry = pbt_registration.BehaviorTreeRegistry.connect(grpc_channel)
    proto_builder = proto_building.ProtoBuilder.connect(grpc_channel)

    print(
        f'Connected successfully to "{solution_status.display_name}'
        f'({solution_status.version})" at "{solution_status.workcell_name}".'
    )
    return cls(
        grpc_channel,
        solution_status.simulated,
        executive,
        skill_registry,
        resource_registry,
        object_world,
        simulator,
        error_loader,
        pose_estimators,
        installer_stub,
        pbt_registry,
        proto_builder,
    )

  def get_health_status(self) -> "HealthStatus":
    """Returns the health status of the solution backend.

    Can be called before or after connecting to the deployed solution.

    Returns:
      Health status of solution backend
    """
    status = self._installer_service_stub.GetInstalledSpec(
        empty_pb2.Empty()
    ).status
    if status == installer_pb2.GetInstalledSpecResponse.HEALTHY:
      return self.HealthStatus.HEALTHY
    if status == installer_pb2.GetInstalledSpecResponse.PENDING:
      return self.HealthStatus.PENDING
    if status == installer_pb2.GetInstalledSpecResponse.ERROR:
      return self.HealthStatus.ERROR
    return self.HealthStatus.UNKNOWN

  def skills_overview(
      self,
      with_signatures: bool = False,
      with_type_annotations: bool = False,
      with_doc: bool = False,
  ) -> None:
    """Prints an overview of the registered skills.

    Args:
      with_signatures: Include signatures for skill construction.
      with_type_annotations: Include type annotations and not just the parameter
        name.
      with_doc: Also print out docstring for each skill.
    """

    def build_signature(skill, with_type_annotations: bool) -> str:
      """Build a signature for skill, optionally including type annotations.

      Args:
        skill: The skill to build the signature for.
        with_type_annotations: Include type annotations and not just the
          parameter name.

      Returns:
        The skill signature.
      """
      signature = inspect.signature(skill)
      parameters = []
      for k, v in signature.parameters.items():
        if not with_type_annotations:
          parameters.append(k)
          continue
        param = str(v)
        parameters.append(param)
      return ", ".join(parameters)

    for skill_id, skill in self.skills.get_skill_ids_and_classes():
      if with_signatures:
        signature_str = build_signature(skill, with_type_annotations)
        print(f"{skill_id}({signature_str})")
      else:
        print(skill_id)
      if with_doc:
        print(f"\n{inspect.getdoc(skill)}\n")

  def update_skills(self) -> None:
    self.skills.update()

  def generate_stubs(self, output_path: str) -> None:
    """Generates type stubs for all available skill classes in the solution.

    The generated stubs can be provided to an IDE or type checker to get proper
    language and type support when working with the auto-generated skill classes
    of the solution building library. Usage examples:
      - VS Code: The given 'output_path' should match the value of the
        'python.analysis.stubPath' setting. After generating the stubs, a
        restart of the Python language server is usually required.
      - mypy: The given 'output_path' should be included in the $MYPYPATH
        environment variable.

    The generated stubs are specific to a solution. They match the skills
    installed in the solution at their respective version. Hence the stubs need
    to be updated when the skills in the solution change or when connecting to a
    different solution.

    Args:
      output_path: The root directory into which the stubs shall be written.
        E.g., the file '<output_path>/intrinsic-stubs/solutions/providers.pyi'
        will be generated.
    """
    stubs.generate(output_path, self.skills, sys.stdout)


def connect(
    grpc_channel_or_hostport: Optional[Union[grpc.Channel, str]] = None,
    *,
    grpc_channel: Optional[grpc.Channel] = None,
    address: Optional[str] = None,
    org: Optional[str] = None,
    solution: Optional[str] = None,
    cluster: Optional[str] = None,
    options: Optional[Dict[str, Any]] = None,
) -> "Solution":
  # pyformat: disable
  """Connects to a deployed solution.

  Args:
    grpc_channel_or_hostport: gRPC channel or address. Deprecated: Use explicit `address` or `grpc_channel`
    grpc_channel: gRPC channel to use for connection.
    address: Connect directly to an address (e.g. localhost). Only one of [project, solution] and address is allowed.
    org: Organization of the solution to connect to.
    solution: Id (not display name!) of the solution to connect to.
    cluster: Name of cluster to connect to (instead of specifying 'solution').
    options: An optional Dict[str, Any] containing additional options.
  Raises:
    ValueError: if parameter combination is incorrect.

  Returns:
    A fully initialized Solution object that represents the deployed solution.
  """
  if (
      sum([
          bool(grpc_channel_or_hostport),
          bool(grpc_channel),
          bool(
              org
              or solution
              or cluster
          ),
          bool(address),
      ])
      > 1
  ):
    solution_params = ["org", "solution"]
    solution_params.append("cluster")
    solution_params = ", ".join(solution_params)
    raise ValueError(
        f"Only one of grpc_channel_or_host_port, [{solution_params}],"
        " grpc_channel or address is allowed!"
    )

  if grpc_channel:
    channel = grpc_channel
  else:
    channel = create_grpc_channel(
        grpc_channel_or_hostport,
        address=address,
        org=org,
        solution=solution,
        cluster=cluster,
    )

  return Solution.for_channel(channel, options=options)


_NO_SOLUTION_SELECTED_ERROR = (
    "No solution selection can be found in the current environment! E.g., in VS"
    " Code you can use the Intrinsic extension to select a deployed solution."
)


def connect_to_selected_solution(
    *,
    options: Optional[Dict[str, Any]] = None,
) -> "Solution":
  """Connects to the deployed solution that is currently selected.

  Connects to the deployed solution that is selected in the current environment.
  E.g., in VS Code you can use the Intrinsic extension to select a deployed
  solution from a list of available solutions and then use this method to
  connect to this solution.

  Args:
    options: Same as for connect().

  Raises:
    NotFoundError: If no solution selection can be found in the current
    environment.

  Returns:
    A fully initialized Solution object that represents the deployed solution.
  """
  try:
    config = userconfig.read()
  except userconfig.NotFoundError as e:
    raise solution_errors.NotFoundError(_NO_SOLUTION_SELECTED_ERROR) from e

  if (
      config.get(userconfig.SELECTED_SOLUTION_TYPE, None)
      == userconfig.SELECTED_SOLUTION_TYPE_LOCAL
  ):
    return connect(options=options)

  # If we reach this point, config.get(userconfig.SELECTED_SOLUTION_TYPE) is
  # either None or userconfig.SELECTED_SOLUTION_TYPE_REMOTE. For backward
  # compatibility we treat both cases equally.

  selected_org = config.get(userconfig.SELECTED_ORGANIZATION, None)
  if selected_org is None:
    raise solution_errors.NotFoundError(
        _NO_SOLUTION_SELECTED_ERROR + " No organization selected."
    )

  selected_solution = config.get(userconfig.SELECTED_SOLUTION, None)
  if selected_solution is None:
    raise solution_errors.NotFoundError(
        _NO_SOLUTION_SELECTED_ERROR + " No solution selected."
    )

  return connect(org=selected_org, solution=selected_solution, options=options)


def create_grpc_channel(
    grpc_channel_or_hostport: Optional[Union[grpc.Channel, str]] = None,
    *,
    address: Optional[str] = None,
    org: Optional[str] = None,
    solution: Optional[str] = None,
    cluster: Optional[str] = None,
) -> grpc.Channel:
  # pyformat: disable
  """Creates a gRPC channel to a deployed solution.

  Args:
    grpc_channel_or_hostport: gRPC channel to the intrinsic box or a string of
      the form "host:port" to connect to (creates a channel with default
      parameters).
    address: Connect directly to an address (e.g. localhost). Only one of
      [project, solution] and address is allowed.
    org: Organization of the solution to connect to.
    solution: Id (not display name!) of the solution to connect to.
    cluster: Name of cluster to connect to (instead of specifying 'solution').

  Returns:
    A gRPC channel
  """
  # pyformat: enable

  params: dialerutil.CreateChannelParams = None
  if not any([
      grpc_channel_or_hostport,
      address,
      org,
      solution,
      cluster,
  ]):
    # Legacy behavior: Use default hostport if called without params.
    default_address = os.environ.get(
        _XFA_BOX_ADDRESS_ENVIRONMENT_VAR, _DEFAULT_HOSTPORT
    )
    params = dialerutil.CreateChannelParams(address=default_address)
  elif grpc_channel_or_hostport:
    warnings.warn(
        "grpc_channel_or_host_port is deprecated. Use `address` or"
        " `grpc_channel` instead.",
        DeprecationWarning,
        stacklevel=2,
    )
    if isinstance(grpc_channel_or_hostport, str):
      params = dialerutil.CreateChannelParams(address=grpc_channel_or_hostport)
    elif isinstance(grpc_channel_or_hostport, grpc.Channel):
      return grpc_channel_or_hostport
    else:
      raise ValueError(
          "Unsupported grpc_channel_or_host_port type"
          f" ({type(grpc_channel_or_hostport)})!"
      )
  elif address is not None:
    params = dialerutil.CreateChannelParams(address=address)
  elif (
      (org is not None)
      or (solution is not None)
      or (cluster is not None)
  ):
    # pyformat: disable
    if not (
        (
            (org is not None)
        )
        and
        (
            (solution is not None)
            or (cluster is not None)
        )
    ):
      # pylint: disable-next=unused-variable
      msg = f"'org' ({org}) and one of 'solution' ({solution}) or 'cluster' ({cluster}) are required together!"
      raise ValueError(msg)
    # pyformat: enable

    resolved_project = None
    if org is not None:
      try:
        resolved_project = auth.read_org_info(org).project
      except auth.OrgNotFoundError as error:
        raise solution_errors.NotFoundError(
            f"Credentials for organization '{error.organization}' not found."
            f" Run 'inctl auth login --org {error.organization}' on a terminal"
            " to login with this organization, or run 'inctl auth list' to see"
            " the organizations you are currently logged in with."
        ) from error

    resolved_cluster = None
    if cluster is not None:
      resolved_cluster = cluster
    if solution is not None:
      resolved_cluster = _get_cluster_from_solution(resolved_project, solution)

    params = dialerutil.CreateChannelParams(
        organization_name=org,
        project_name=resolved_project,
        cluster=resolved_cluster,
    )

  return dialerutil.create_channel(params, grpc_options=_GRPC_OPTIONS)


def _get_cluster_from_solution(project: str, solution_id: str) -> str:
  # Open a temporary gRPC channel to the cloud cluster to resolve the cluster
  # on which the solution is running.
  params = dialerutil.CreateChannelParams(project_name=project)
  channel = dialerutil.create_channel(params)
  stub = solutiondiscovery_api_pb2_grpc.SolutionDiscoveryServiceStub(channel)
  response = stub.GetSolutionDescription(
      solutiondiscovery_api_pb2.GetSolutionDescriptionRequest(name=solution_id)
  )
  channel.close()

  return response.solution.cluster_name


@solution_errors.retry_on_pending_backend
@error_handling.retry_on_grpc_unavailable
def _get_solution_status_with_retry(
    stub: installer_pb2_grpc.InstallerServiceStub,
) -> installer_pb2.GetInstalledSpecResponse:
  """Loads a solution's status.

  Args:
    stub: Installer service to query health.

  Returns:
    Workcell status

  Raises:
    solution_errors.BackendPendingError: Will lead to retry.
    solution_errors.BackendHealthError: Not recoverable.
  """
  try:
    response = stub.GetInstalledSpec(empty_pb2.Empty())

    if response.status != installer_pb2.GetInstalledSpecResponse.HEALTHY:
      if response.status == installer_pb2.GetInstalledSpecResponse.PENDING:
        print("Solution backends not healthy yet. Retrying...")
        print(f"Reason: {response.error_reason}")
        # Note this error leads to a retry given the retry_on_pending_backend
        # decorator.
        raise solution_errors.BackendPendingError(
            f"Workcell backends not healthy yet. {response.error_reason}"
        )
      if response.status == installer_pb2.GetInstalledSpecResponse.ERROR:
        raise solution_errors.BackendHealthError(
            "Workcell backend is unhealthy and not expected to recover "
            "without intervention. Try restarting your solution. "
            f"{response.error_reason}"
        )
      else:
        raise solution_errors.BackendHealthError(
            "Unexpected solution health status. Try restarting your "
            f"solution. {response.error_reason}"
        )
    return response
  except grpc.RpcError as e:
    if hasattr(e, "code"):
      if e.code() == grpc.StatusCode.NOT_FOUND:
        raise solution_errors.BackendNoWorkcellError(
            "No workcell spec has been installed. "
            "Start a solution before connecting."
        )
    raise
