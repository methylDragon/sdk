// Copyright 2023 Intrinsic Innovation LLC

// Package cmdutils provides utils for asset inctl commands.
package cmdutils

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/google/go-containerregistry/pkg/authn"
	"github.com/google/go-containerregistry/pkg/v1/google"
	"github.com/google/go-containerregistry/pkg/v1/remote"
	"github.com/pkg/errors"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
	"intrinsic/assets/imagetransfer"
	"intrinsic/assets/imageutils"
	"intrinsic/tools/inctl/util/orgutil"
)

const (
	// KeyAddress is the name of the address flag.
	KeyAddress = "address"
	// KeyAPIKey is the name of the arg to specify the api-key to use.
	KeyAPIKey = "api_key"
	// KeyAuthUser is the name of the auth user flag.
	KeyAuthUser = "auth_user"
	// KeyAuthPassword is the name of the auth password flag.
	KeyAuthPassword = "auth_password"
	// KeyCatalogAddress is the name of the catalog address flag.
	KeyCatalogAddress = "catalog_address"
	// KeyCluster is the name of the cluster flag.
	KeyCluster = "cluster"
	// KeyContext is the name of the context flag.
	KeyContext = "context"
	// KeyDefault is the name of the default flag.
	KeyDefault = "default"
	// KeyEnvironment is the name of the environment flag.
	KeyEnvironment = "environment"
	// KeyDryRun is the name of the dry run flag.
	KeyDryRun = "dry_run"
	// KeyFilter is the name of the filter flag.
	KeyFilter = "filter"
	// KeyIgnoreExisting is the name of the flag to ignore AlreadyExists errors.
	KeyIgnoreExisting = "ignore_existing"
	// KeyInstallerAddress is the name of the installer address flag.
	KeyInstallerAddress = "installer_address"
	// KeyManifestFile is the file path to the manifest binary.
	KeyManifestFile = "manifest_file"
	// KeyManifestTarget is the build target to the skill manifest.
	KeyManifestTarget = "manifest_target"
	// KeyOrgPrivate is the name of the org-private flag.
	KeyOrgPrivate = "org_private"
	// KeyRegistry is the name of the registry flag.
	KeyRegistry = "registry"
	// KeyReleaseNotes is the name of the release notes flag.
	KeyReleaseNotes = "release_notes"
	// KeySolution is the name of the solution flag.
	KeySolution = "solution"
	// KeyType is the name of the type flag.
	KeyType = "type"
	// KeyTimeout is the name of the timeout flag.
	KeyTimeout = "timeout"
	// KeyUseBorgCredentials is the name of the flag to use borg credentials.
	KeyUseBorgCredentials = "use_borg_credentials"
	// KeyUseInProcCatalog is the name of the flag for using an in-proc catalog.
	KeyUseInProcCatalog = "use_in_proc_catalog"
	// KeyVendor is the name of the vendor flag.
	KeyVendor = "vendor"
	// KeyVersion is the name of the version flag.
	KeyVersion = "version"

	// KeySkipDirectUpload is boolean flag controlling direct upload behavior
	KeySkipDirectUpload = "skip_direct_upload"

	// Flags copied from orgutil for consistency

	// KeyProject is used as central flag name for passing a project name to inctl.
	KeyProject = orgutil.KeyProject
	// KeyOrganization is used as central flag name for passing an organization name to inctl.
	KeyOrganization = orgutil.KeyOrganization

	envPrefix = "intrinsic"

	gcpEndpointsURLFormat = "dns:///www.endpoints.%s.cloud.goog:443"
)

// CmdFlags abstracts interaction with inctl command flags.
type CmdFlags struct {
	cmd        *cobra.Command
	viperLocal *viper.Viper
}

// NewCmdFlags returns a new CmdFlags instance.
func NewCmdFlags() *CmdFlags {
	viperLocal := viper.New()
	viperLocal.SetEnvPrefix(envPrefix)

	return NewCmdFlagsWithViper(viperLocal)
}

// NewCmdFlagsWithViper returns a new CmdFlags instance with a custom Viper.
func NewCmdFlagsWithViper(viperLocal *viper.Viper) *CmdFlags {
	return &CmdFlags{cmd: nil, viperLocal: viperLocal}
}

// SetCommand sets the cobra Command to interact with.
//
// The command must be set before any flags are added.
func (cf *CmdFlags) SetCommand(cmd *cobra.Command) {
	cf.cmd = cmd
}

// AddFlagsCatalogInProcEnvironment adds flags for using an in-proc catalog and specifying the
// Firestore environment.
func (cf *CmdFlags) AddFlagsCatalogInProcEnvironment() {
	cf.OptionalBool(KeyUseInProcCatalog, false, "DEPRECATED DO NOT USE. Whether to use an in-proc catalog service.")
	cf.OptionalString(KeyEnvironment, "", "DEPRECATED DO NOT USE. The Firestore DB environment (only used with the in-proc catalog).")
	cf.cmd.MarkFlagsRequiredTogether(KeyUseInProcCatalog, KeyEnvironment)
}

// GetFlagsCatalogInProcEnvironment gets the values of the in-proc catalog and environment flags
// added by AddFlagsCatalogInProcEnvironment.
func (cf *CmdFlags) GetFlagsCatalogInProcEnvironment() (bool, string) {
	return cf.GetBool(KeyUseInProcCatalog), cf.GetString(KeyEnvironment)
}

// AddFlagsCredentials adds args for specifying credentials.
func (cf *CmdFlags) AddFlagsCredentials() {
	cf.OptionalBool(KeyUseBorgCredentials, false, "Use credentials associated with the current borg user, rather than application-default credentials.")
	cf.OptionalString(KeyAPIKey, "", "The API key to use for authentication.")
}

// GetFlagsCredentials gets the values of the credential args.
func (cf *CmdFlags) GetFlagsCredentials() (useBorgCredentials bool, apiKey string) {
	useBorgCredentials = cf.GetBool(KeyUseBorgCredentials)
	apiKey = cf.GetString(KeyAPIKey)

	return useBorgCredentials, apiKey
}

// AddFlagDefault adds a flag for marking a released asset as default.
func (cf *CmdFlags) AddFlagDefault(assetType string) {
	cf.OptionalBool(KeyDefault, false, fmt.Sprintf("Whether this %s version should be tagged as the default.", assetType))
}

// GetFlagDefault gets the value of the default flag added by AddFlagDefault.
func (cf *CmdFlags) GetFlagDefault() bool {
	return cf.GetBool(KeyDefault)
}

// AddFlagDryRun adds a flag for performing a dry run.
func (cf *CmdFlags) AddFlagDryRun() {
	cf.OptionalBool(KeyDryRun, false, "Dry-run by validating but not performing any actions.")
}

// GetFlagDryRun gets the value of the dry run flag added by AddFlagDryRun.
func (cf *CmdFlags) GetFlagDryRun() bool {
	return cf.GetBool(KeyDryRun)
}

// AddFlagIgnoreExisting adds a flag to ignore AlreadyExists errors.
func (cf *CmdFlags) AddFlagIgnoreExisting(assetType string) {
	cf.OptionalBool(KeyIgnoreExisting, false, fmt.Sprintf("Ignore errors if the specified %s version already exists in the catalog.", assetType))
}

// GetFlagIgnoreExisting gets the value of the flag added by AddFlagIgnoreExisting.
func (cf *CmdFlags) GetFlagIgnoreExisting() bool {
	return cf.GetBool(KeyIgnoreExisting)
}

// AddFlagAddress adds a flag for the installer service address.
func (cf *CmdFlags) AddFlagAddress() {
	cf.OptionalEnvString(KeyAddress, "xfa.lan:17080", `The address of the cluster.
When not running the cluster on localhost, this should be the address of the relay
(e.g.: dns:///www.endpoints.<gcp_project_name>.cloud.goog:443).`)
}

// GetFlagAddress gets the value of the cluster address flag added by
// AddFlagAddress.
func (cf *CmdFlags) GetFlagAddress() string {
	return cf.GetString(KeyAddress)
}

// AddFlagInstallerAddress adds a flag for the installer service address.
func (cf *CmdFlags) AddFlagInstallerAddress() {
	cf.OptionalEnvString(KeyInstallerAddress, "xfa.lan:17080", `The address of the installer service.
When not running the cluster on localhost, this should be the address of the relay
(e.g.: dns:///www.endpoints.<gcp_project_name>.cloud.goog:443).`)
}

// GetFlagInstallerAddress gets th value of the installer address flag added by
// AddFlagInstallerAddress.
func (cf *CmdFlags) GetFlagInstallerAddress() string {
	return cf.GetString(KeyInstallerAddress)
}

// GetNormalizedInstallerAddress gets the normalized installer address based
// on --project, --org, --solution and --installer_address flags specified
// on command line. It tries to avoid returning default value of the
// --installer_address flag if user sets --solution flag.
func (cf *CmdFlags) GetNormalizedInstallerAddress() string {
	installerAddress := cf.GetString(KeyInstallerAddress)
	if cf.IsSet(KeyInstallerAddress) {
		// user set this on cmd line, we honor their choice.
		return installerAddress
	}

	solution := cf.GetString(KeySolution)
	project := cf.GetFlagProject()
	if solution != "" && project != "" {
		// user selected either `--project` or `--org` and `--solution`
		// but didn't set `--installer_address`, we are not going to contact
		// local minikube (as this is highly probably 3P anyway).
		return fmt.Sprintf(gcpEndpointsURLFormat, project)
	}
	// fallback in case we have some weirdness in command flags.
	return installerAddress
}

// AddFlagsListClusterSolution  adds flags for the cluster and solution when listing assets.
func (cf *CmdFlags) AddFlagsListClusterSolution(assetType string) {
	cf.OptionalString(KeyCluster, "", fmt.Sprintf("The Kubernetes cluster from which the %ss should be read.", assetType))
	cf.OptionalEnvString(KeySolution, "", fmt.Sprintf("The solution from which the %ss should be listed. Needs to run on a cluster.", assetType))

	cf.cmd.MarkFlagsMutuallyExclusive(KeyCluster, KeySolution)
}

// GetFlagsListClusterSolution gets the values of the cluster and solution flags added by
// AddFlagsListClusterSolution.
func (cf *CmdFlags) GetFlagsListClusterSolution() (string, string, error) {
	cluster := cf.GetString(KeyCluster)
	solution := cf.GetString(KeySolution)
	if cluster == "" && solution == "" {
		return "", "", fmt.Errorf("One of `--%s` or `--%s` needs to be set", KeyCluster, KeySolution)
	}

	return cluster, solution, nil
}

// AddFlagsManifest adds flags for specifying a manifest.
func (cf *CmdFlags) AddFlagsManifest() {
	cf.OptionalString(KeyManifestFile, "", "The path to the manifest binary file.")
	cf.OptionalString(KeyManifestTarget, "", "The manifest bazel target.")

	cf.cmd.MarkFlagsMutuallyExclusive(KeyManifestFile, KeyManifestTarget)
}

// GetFlagsManifest gets the values of the manifest flags added by AddFlagsManifest.
func (cf *CmdFlags) GetFlagsManifest() (string, string, error) {
	mf := cf.GetString(KeyManifestFile)
	mt := cf.GetString(KeyManifestTarget)

	if mf == "" && mt == "" {
		return "", "", fmt.Errorf("one of --%s or --%s must provided", KeyManifestFile, KeyManifestTarget)
	}

	return mf, mt, nil
}

// AddFlagOrgPrivate adds a flag for marking a released asset as private to the organization.
func (cf *CmdFlags) AddFlagOrgPrivate() {
	cf.OptionalBool(KeyOrgPrivate, false, "Whether this asset should be private to the organization that owns it.")
}

// GetFlagOrgPrivate gets the value of the org_private flag added by AddFlagOrgPrivate.
func (cf *CmdFlags) GetFlagOrgPrivate() bool {
	return cf.GetBool(KeyOrgPrivate)
}

// AddFlagsProjectOrg adds both the project and org flag, including the necessary handling.
func (cf *CmdFlags) AddFlagsProjectOrg() {
	// While WrapCmd returns the pointer to make it inline, it's modifying, so we can use it here.
	orgutil.WrapCmd(cf.cmd, cf.viperLocal)
}

// AddFlagProject adds a flag for the GCP project.
func (cf *CmdFlags) AddFlagProject() {
	cf.RequiredEnvString(KeyProject, "", "The Google Cloud Platform (GCP) project to use.")
}

// AddFlagProjectOptional adds an optional flag for the GCP project.
func (cf *CmdFlags) AddFlagProjectOptional() {
	cf.OptionalEnvString(KeyProject, "", "The Google Cloud Platform (GCP) project to use.")
}

// GetFlagProject gets the value of the project flag added by AddFlagProject.
func (cf *CmdFlags) GetFlagProject() string {
	return cf.GetString(KeyProject)
}

// GetFlagOrganization gets the value of the project flag added by AddFlagProject.
func (cf *CmdFlags) GetFlagOrganization() string {
	return cf.GetString(KeyOrganization)
}

// AddFlagRegistry adds a flag for the registry when side-loading an asset.
func (cf *CmdFlags) AddFlagRegistry() {
	cf.OptionalEnvString(KeyRegistry, "", fmt.Sprintf("The container registry address. This option is ignored when --%s=image.", KeyType))
}

// GetFlagRegistry gets the value of the registry flag added by AddFlagRegistry.
func (cf *CmdFlags) GetFlagRegistry() string {
	return cf.GetString(KeyRegistry)
}

// AddFlagsRegistryAuthUserPassword adds flags for user/password authentication for a private
// container registry.
func (cf *CmdFlags) AddFlagsRegistryAuthUserPassword() {
	cf.OptionalString(KeyAuthUser, "", "The username used to access the private container registry.")
	cf.OptionalString(KeyAuthPassword, "", "The password used to authenticate private container registry access.")
	cf.cmd.MarkFlagsRequiredTogether(KeyAuthUser, KeyAuthPassword)
}

// GetFlagsRegistryAuthUserPassword gets the values of the user/password flags added by
// AddFlagsRegistryAuthUserPassword
func (cf *CmdFlags) GetFlagsRegistryAuthUserPassword() (string, string) {
	return cf.GetString(KeyAuthUser), cf.GetString(KeyAuthPassword)
}

// AddFlagReleaseNotes adds a flag for release notes.
func (cf *CmdFlags) AddFlagReleaseNotes(assetType string) {
	cf.OptionalString(KeyReleaseNotes, "", fmt.Sprintf("Release notes for this version of the %s.", assetType))
}

// GetFlagReleaseNotes gets the value of the release notes flag added by AddFlagReleaseNotes.
func (cf *CmdFlags) GetFlagReleaseNotes() string {
	return cf.GetString(KeyReleaseNotes)
}

// AddFlagSkillReleaseType adds a flag for the type when releasing a skill.
func (cf *CmdFlags) AddFlagSkillReleaseType() {
	targetTypeDescriptions := []string{}

	targetTypeDescriptions = append(targetTypeDescriptions, `"build": a build target that creates a skill image.`)
	targetTypeDescriptions = append(targetTypeDescriptions, `"archive": a file path to an already-built image.`)
	targetTypeDescriptions = append(targetTypeDescriptions, `"image": a container image name.`)

	cf.RequiredString(
		KeyType,
		fmt.Sprintf("The type of target that is being specified. One of the following:\n   %s", strings.Join(targetTypeDescriptions, "\n   ")),
	)
}

// GetFlagSkillReleaseType gets the value of the type flag added by AddFlagSkillReleaseType.
func (cf *CmdFlags) GetFlagSkillReleaseType() string {
	return cf.GetString(KeyType)
}

// AddFlagSideloadContext adds a flag for the context when side-loading an asset.
func (cf *CmdFlags) AddFlagSideloadContext() {
	cf.OptionalEnvString(KeyContext, "", fmt.Sprintf("The Kubernetes cluster to use. Required unless using localhost for %s.", KeyInstallerAddress))
}

// GetFlagSideloadContext gets the value of the context flag added by AddFlagSideloadContext.
func (cf *CmdFlags) GetFlagSideloadContext() string {
	return cf.GetString(KeyContext)
}

// AddFlagsSideloadContextSolution adds flags for the context and solution when side-loading an
// asset.
func (cf *CmdFlags) AddFlagsSideloadContextSolution(assetType string) {
	cf.OptionalEnvString(KeyContext, "", fmt.Sprintf("The Kubernetes cluster to use. Required unless %s is set or using localhost for %s.", KeySolution, KeyInstallerAddress))

	cf.OptionalEnvString(KeySolution, "", fmt.Sprintf(`The solution into which the %s is loaded. Needs to run on a cluster.
Required unless %s is set.`, assetType, KeyContext))
	cf.cmd.MarkFlagsMutuallyExclusive(KeyContext, KeySolution)
}

// GetFlagsSideloadContextSolution gets the values of the context and solution flags added by
// AddFlagsSideloadContextSolution.
func (cf *CmdFlags) GetFlagsSideloadContextSolution() (string, string) {
	return cf.GetString(KeyContext), cf.GetString(KeySolution)
}

// AddFlagSideloadStartType adds a flag for the type when starting an asset.
func (cf *CmdFlags) AddFlagSideloadStartType() {
	cf.RequiredString(KeyType, fmt.Sprintf(
		`The target's type:
%s	build target that creates an image
%s	file path pointing to an already-built image
%s	container image name`,
		imageutils.Build,
		imageutils.Archive,
		imageutils.Image,
	))
}

// GetFlagSideloadStartType gets the value of the type flag added by AddFlagSideloadStartType.
func (cf *CmdFlags) GetFlagSideloadStartType() string {
	return cf.GetString(KeyType)
}

// AddFlagSideloadStopType adds a flag for the type when stopping an asset.
func (cf *CmdFlags) AddFlagSideloadStopType(assetType string) {
	cf.RequiredString(KeyType, fmt.Sprintf(
		`The target's type:
%s	build target that creates an image
%s	file path pointing to an already-built image
%s	container image name
%s		%s id
%s		%s name [deprecated: prefer to use %s]`,
		imageutils.Build,
		imageutils.Archive,
		imageutils.Image,
		imageutils.ID,
		assetType,
		imageutils.Name,
		assetType,
		imageutils.ID,
	))
}

// GetFlagSideloadStopType gets the value of the type flag added by AddFlagSideloadStopType.
func (cf *CmdFlags) GetFlagSideloadStopType() string {
	return cf.GetString(KeyType)
}

// AddFlagSideloadStartTimeout adds a flag for the timeout when starting an asset.
func (cf *CmdFlags) AddFlagSideloadStartTimeout(assetType string) {
	cf.OptionalString(KeyTimeout, "180s", fmt.Sprintf(`Maximum time to wait for the %s to
become available in the cluster after starting it. Can be set to any valid duration
(\"60s\", \"5m\", ...) or to \"0\" to disable waiting.`, assetType))
}

// GetFlagSideloadStartTimeout gets the value of the flag added by AddFlagSideloadStartTimeout.
func (cf *CmdFlags) GetFlagSideloadStartTimeout() (time.Duration, string, error) {
	timeoutStr := cf.GetString(KeyTimeout)
	timeout, err := parseNonNegativeDuration(timeoutStr)
	if err != nil {
		return timeout, timeoutStr, errors.Wrapf(err, "invalid value passed for --%s", KeyTimeout)
	}

	return timeout, timeoutStr, nil
}

// AddFlagVendor adds a flag for the asset vendor.
func (cf *CmdFlags) AddFlagVendor(assetType string) {
	cf.RequiredString(KeyVendor, fmt.Sprintf("The %s vendor.", assetType))
}

// GetFlagVendor gets the value of the vendor flag added by AddFlagVendor.
func (cf *CmdFlags) GetFlagVendor() string {
	return cf.GetString(KeyVendor)
}

// AddFlagVersion adds a flag for the asset version.
func (cf *CmdFlags) AddFlagVersion(assetType string) {
	cf.RequiredString(KeyVersion, fmt.Sprintf("The %s version, in sem-ver format.", assetType))
}

// GetFlagVersion gets the value of the version flag added by AddFlagVersion.
func (cf *CmdFlags) GetFlagVersion() string {
	return cf.GetString(KeyVersion)
}

// AddFlagSkipDirectUpload adds a flag for disabling direct upload to workcells
func (cf *CmdFlags) AddFlagSkipDirectUpload(assetType string) {
	usage := fmt.Sprintf("Skips direct upload of %s to workcell. Requires "+
		"external repository. (default false)\nCan be defined via the %s_%s "+
		"environment variable.", assetType, envPrefix, strings.ToUpper(KeySkipDirectUpload))
	cf.OptionalBool(KeySkipDirectUpload, false, usage)
	cf.cmd.PersistentFlags().Lookup(KeySkipDirectUpload).Hidden = true
	cf.viperLocal.BindEnv(KeySkipDirectUpload)
}

// GetFlagSkipDirectUpload gets the value of the flag added by AddFlagSkipDirectUpload
func (cf *CmdFlags) GetFlagSkipDirectUpload() bool {
	return cf.GetBool(KeySkipDirectUpload)
}

// String adds a new string flag.
func (cf *CmdFlags) String(name string, value string, usage string) {
	cf.cmd.PersistentFlags().String(name, value, usage)
	cf.viperLocal.BindPFlag(name, cf.cmd.PersistentFlags().Lookup(name))
}

// RequiredString adds a new required string flag.
func (cf *CmdFlags) RequiredString(name string, usage string) {
	cf.String(name, "", fmt.Sprintf("(required) %s", usage))
	cf.cmd.MarkFlagRequired(name)
}

// OptionalString adds a new optional string flag.
func (cf *CmdFlags) OptionalString(name string, value string, usage string) {
	cf.String(name, value, fmt.Sprintf("(optional) %s", usage))
}

// RequiredEnvString adds a new required string flag that is bound to the corresponding ENV
// variable.
func (cf *CmdFlags) RequiredEnvString(name string, value string, usage string) {
	envVarName := strings.ToUpper(fmt.Sprintf("%s_%s", envPrefix, name))
	cf.envString(name, value, fmt.Sprintf("%s\nRequired unless %s environment variable is defined.", usage, envVarName))

	if cf.GetString(name) == "" {
		cf.cmd.MarkPersistentFlagRequired(name)
	}
}

// OptionalEnvString adds a new optional string flag that is bound to the corresponding ENV
// variable.
func (cf *CmdFlags) OptionalEnvString(name string, value string, usage string) {
	envVarName := strings.ToUpper(fmt.Sprintf("%s_%s", envPrefix, name))
	cf.envString(name, value, fmt.Sprintf("%s\nCan be defined via the %s environment variable.", usage, envVarName))
}

// GetString gets the value of a string flag.
func (cf *CmdFlags) GetString(name string) string {
	return cf.viperLocal.GetString(name)
}

// Bool adds a new bool flag.
func (cf *CmdFlags) Bool(name string, value bool, usage string) {
	cf.cmd.PersistentFlags().Bool(name, value, usage)
	cf.viperLocal.BindPFlag(name, cf.cmd.PersistentFlags().Lookup(name))
}

// RequiredBool adds a new required bool flag.
func (cf *CmdFlags) RequiredBool(name string, usage string) {
	cf.Bool(name, false, fmt.Sprintf("(required) %s", usage))
	cf.cmd.MarkFlagRequired(name)
}

// OptionalBool adds a new optional bool flag.
func (cf *CmdFlags) OptionalBool(name string, value bool, usage string) {
	cf.Bool(name, value, fmt.Sprintf("(optional) %s", usage))
}

// GetBool gets the value of a bool flag.
func (cf *CmdFlags) GetBool(name string) bool {
	return cf.viperLocal.GetBool(name)
}

// Int adds a new int flag.
func (cf *CmdFlags) Int(name string, value int, usage string) {
	cf.cmd.PersistentFlags().Int(name, value, usage)
	cf.viperLocal.BindPFlag(name, cf.cmd.PersistentFlags().Lookup(name))
}

// RequiredInt adds a new required int flag.
func (cf *CmdFlags) RequiredInt(name string, usage string) {
	cf.Int(name, 0, fmt.Sprintf("(required) %s", usage))
	cf.cmd.MarkFlagRequired(name)
}

// OptionalInt adds a new optional int flag.
func (cf *CmdFlags) OptionalInt(name string, value int, usage string) {
	cf.Int(name, value, fmt.Sprintf("(optional) %s", usage))
}

// GetInt gets the value of an int flag.
func (cf *CmdFlags) GetInt(name string) int {
	return cf.viperLocal.GetInt(name)
}

// IsSet checks if value of given flag was set on command line.
// Allows to check if value is coming from user, or is default value.
func (cf *CmdFlags) IsSet(name string) bool {
	return cf.viperLocal.IsSet(name)
}

func (cf *CmdFlags) envString(name string, value string, usage string) {
	cf.String(name, value, usage)
	cf.viperLocal.BindEnv(name)
}

func parseNonNegativeDuration(durationStr string) (time.Duration, error) {
	duration, err := time.ParseDuration(durationStr)
	if err != nil {
		return 0, fmt.Errorf("parsing duration: %w", err)
	}
	if duration < 0 {
		return 0, fmt.Errorf("duration must not be negative, but got %q", durationStr)
	}
	return duration, nil
}

func (cf *CmdFlags) createBasicAuth() *imageutils.BasicAuth {
	user, pwd := cf.GetFlagsRegistryAuthUserPassword()
	if len(user) == 0 || len(pwd) == 0 {
		return nil
	}
	return &imageutils.BasicAuth{
		User: user,
		Pwd:  pwd,
	}
}

func (cf *CmdFlags) authOpt() remote.Option {
	if auth := cf.createBasicAuth(); auth != nil {
		return remote.WithAuth(authn.FromConfig(authn.AuthConfig{
			Username: auth.User,
			Password: auth.Pwd,
		}))
	}
	return remote.WithAuthFromKeychain(google.Keychain)
}

// CreateRegistryOpts creates registry options for processing images.
func (cf *CmdFlags) CreateRegistryOpts(ctx context.Context) imageutils.RegistryOptions {
	opts := imageutils.RegistryOptions{
		Transferer: imagetransfer.RemoteTransferer(remote.WithContext(ctx), cf.authOpt()),
		URI:        cf.GetFlagRegistry(),
	}
	if auth := cf.createBasicAuth(); auth != nil {
		opts.BasicAuth = *auth
	}
	return opts
}

// MarkHidden marks all flags in the list as hidden for the given command
func (cf *CmdFlags) MarkHidden(flagsToHide ...string) {
	flags := cf.cmd.PersistentFlags()
	for _, flagName := range flagsToHide {
		flag := flags.Lookup(flagName)
		if flag != nil {
			flag.Hidden = true
		}
	}
}
