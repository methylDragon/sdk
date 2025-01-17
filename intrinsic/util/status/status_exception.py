# Copyright 2023 Intrinsic Innovation LLC

"""Exception implementation with additional extended status."""

from __future__ import annotations

import datetime
import textwrap
import traceback
from typing import Optional, Sequence, Tuple, Union

from google.rpc import code_pb2
from google.rpc import status_pb2
import grpc
from intrinsic.logging.proto import context_pb2
from intrinsic.util.status import extended_status_pb2

# This key is taken from the grpc implementation and generates special behavior
# when sending it as trailing metadata.
_GRPC_DETAILS_METADATA_KEY = "grpc-status-details-bin"


class ExtendedStatusError(Exception, grpc.Status):
  """Class that represents an error with extended status information.

  The class uses the builder pattern, so you can parameterize and raise the
  exception like this:

  raise status_exception.ExtendedStatusError("ai.intrinsic.my_skill", 24543)
    .set_title("Failed to do fancy thing")

  Attributes:
    proto: proto representing the extended state
  """

  _extended_status: extended_status_pb2.ExtendedStatus
  _emit_traceback: bool

  def __init__(
      self,
      component: str,
      code: int,
      *,
      title: str = "",
      external_report_message: str = "",
      internal_report_message: str = "",
      timestamp: datetime.datetime | None = None,
  ):
    """Initializes the instance.

    Args:
      component: Component for StatusCode where error originated.
      code: Numeric code specific to component for StatusCode.
      title: brief title of the error, make this meaningful and keep to a length
        of 75 characters if possible.
      external_report_message: if non-empty, set extended status external report
        message to this string.
      internal_report_message: if non-empty, set extended status internal report
        message to this string. Only set this in an environment where the data
        may be shared.
      timestamp: The time of the error. If None sets current time.
    """
    self._extended_status = extended_status_pb2.ExtendedStatus(
        status_code=extended_status_pb2.StatusCode(
            component=component, code=code
        )
    )
    self._emit_traceback = False
    if title:
      self.set_title(title)
    if external_report_message:
      self.set_external_report_message(external_report_message)
    if internal_report_message:
      self.set_internal_report_message(internal_report_message)
    self.set_timestamp(timestamp or datetime.datetime.now())
    super().__init__(external_report_message)

  @classmethod
  def create_from_proto(
      cls, proto: extended_status_pb2.ExtendedStatus
  ) -> ExtendedStatusError:
    if proto.HasField("status_code"):
      es_err = cls(proto.status_code.component, proto.status_code.code)
    else:
      # Bad, this shouldn't happen but we cannot do better here
      es_err = cls("", 0)
    es_err._extended_status = proto
    return es_err

  @property
  def proto(self) -> extended_status_pb2.ExtendedStatus:
    """Retrieves extended status encoded as ExtendedStatus proto."""
    if self._emit_traceback:
      message_parts: list[str] = []
      if self._extended_status.internal_report.message:
        message_parts.append(self._extended_status.internal_report.message)
        message_parts.append("\n\n")

      message_parts.extend(traceback.format_exception(self))

      self._extended_status.internal_report.message = "".join(message_parts)

    return self._extended_status

  def set_extended_status(
      self, extended_status: extended_status_pb2.ExtendedStatus
  ) -> ExtendedStatusError:
    """Sets extended status directly from a proto."""
    # We do not allow overwriting the status code once set
    status_code = extended_status_pb2.StatusCode(
        component=self._extended_status.status_code.component,
        code=self._extended_status.status_code.code,
    )
    self._extended_status.CopyFrom(extended_status)
    self._extended_status.status_code.CopyFrom(status_code)
    return self

  def set_timestamp(
      self, timestamp: datetime.datetime | None = None
  ) -> ExtendedStatusError:
    """Sets time of error.

    Args:
      timestamp: The time of the error. If None sets current time.

    Returns:
      self
    """
    self._extended_status.timestamp.FromDatetime(
        timestamp or datetime.datetime.now()
    )
    return self

  @property
  def status_code(self) -> tuple[str, int]:
    if self._extended_status.HasField("status_code"):
      return (
          self._extended_status.status_code.component,
          self._extended_status.status_code.code,
      )
    return ("", 0)

  @property
  def timestamp(self) -> datetime.datetime:
    return self._extended_status.timestamp.ToDatetime()

  def set_title(self, title: str) -> ExtendedStatusError:
    """Sets title of error.

    Args:
      title: title string for the error

    Returns:
      self
    """
    self._extended_status.title = title
    return self

  @property
  def title(self) -> str:
    return self._extended_status.title

  def add_context(
      self, context_status: extended_status_pb2.ExtendedStatus
  ) -> ExtendedStatusError:
    """Adds context status.

    Args:
      context_status: another extended status that helps explain or further
        analyze this error.

    Returns:
      self
    """
    self._extended_status.context.append(context_status)
    return self

  def set_internal_report_message(self, message: str) -> ExtendedStatusError:
    """Sets internal report message.

    Call this function only if you intend for the receiver of the status to see
    it. An indicator could be running in the context of specific organizations.

    Args:
      message: human-readable error message intended for internal developers

    Returns:
      self
    """
    self._extended_status.internal_report.message = message
    return self

  @property
  def internal_report(self) -> extended_status_pb2.ExtendedStatus.Report | None:
    return self._extended_status.internal_report

  def set_external_report_message(self, message: str) -> ExtendedStatusError:
    """Sets external report message.

    This is the main error message and should almost always be set.

    Args:
      message: human-readable error message intended for users of the component.

    Returns:
      self
    """
    self._extended_status.external_report.message = message
    return self

  @property
  def external_report(self) -> extended_status_pb2.ExtendedStatus.Report | None:
    return self._extended_status.external_report

  def set_log_context(
      self, context: context_pb2.Context
  ) -> ExtendedStatusError:
    """Sets logging context.

    Note that this is specific to the structured data logger. This is NOT
    other extended status as context (use add_context() for those).

    Args:
      context: log context

    Returns:
      self
    """
    self._extended_status.related_to.log_context.CopyFrom(context)
    return self

  def emit_traceback_to_internal_report(self) -> ExtendedStatusError:
    """Enables emitting a traceback to internal report when proto is retrieved."""
    # We cannot directly add the traceback, as that is only created when the
    # error is raised (which also performs useful depth limitations on the
    # traceback).
    self._emit_traceback = True
    return self

  def __str__(self) -> str:
    """Converts the error to a string.

    Note that this does not include a traceback, even if enabled via emit. This
    is primarily because it would cause an infinite loop (traceback generation
    converts encountered exceptions to a string), and it may not always be set,
    e.g., if the raiser of the exception prints this for debugging.

    Returns:
      string representation of error.
    """
    strs: list[str] = []
    status_code = self._extended_status.status_code
    strs.append(f"StatusCode: {status_code.component}:{status_code.code}\n")
    if self._extended_status.HasField("timestamp"):
      strs.append(
          "Timestamp: "
          f" {self._extended_status.timestamp.ToDatetime().strftime('%c')}\n"
      )
    if self._extended_status.HasField("external_report"):
      strs.append(
          "External"
          f" Report:\n{textwrap.indent(self._extended_status.external_report.message, '  ')}\n"
      )
    if self._extended_status.HasField("internal_report"):
      strs.append(
          "Internal"
          f" Report:\n{textwrap.indent(self._extended_status.internal_report.message, '  ')}\n"
      )

    if self._extended_status.context:
      strs.append("\nContext\n")
      for i, context in enumerate(self._extended_status.context):
        strs.append(f"***** Context {i} *****\n")
        context_es = ExtendedStatusError.create_from_proto(context)
        strs.append(f"{context_es}\n")
      strs.append("\n")

    return "".join(strs)

  ### The following are for compatibility with grpc.Status
  @property
  def code(self) -> grpc.StatusCode:
    """Returns GRPC status code UNKNOWN.

    Only added to comply with grpc.Status interface.
    """
    code = self._extended_status.status_code.code
    for status_code in grpc.StatusCode:
      if status_code.value[0] == code:
        return status_code
    return grpc.StatusCode.UNKNOWN

  @property
  def details(self) -> str:
    """Returns the title as GRPC status message.

    Only added to comply with grpc.Status interface.
    """
    return self._extended_status.title

  @property
  def trailing_metadata(
      self,
  ) -> Optional[Sequence[Tuple[str, Union[str, bytes]]]]:
    """Returns GRPC trailing metadata encoding this extended status.

    Only added to comply with grpc.Status interface.
    """
    rpc_status = status_pb2.Status(
        code=code_pb2.Code.Value(self.code.name),
        message=self._extended_status.title,
    )
    rpc_status.details.add().Pack(self._extended_status)
    return ((_GRPC_DETAILS_METADATA_KEY, rpc_status.SerializeToString()),)
