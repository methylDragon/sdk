// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

// Package stop defines the skill stop command which removes a skill.
package stop

import (
	"fmt"
	"log"

	installerpb "intrinsic/kubernetes/workcell_spec/proto/installer_go_grpc_proto"

	"github.com/google/go-containerregistry/pkg/v1/google"
	"github.com/google/go-containerregistry/pkg/v1/remote"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
	"intrinsic/skills/tools/skill/cmd/cmd"
	"intrinsic/skills/tools/skill/cmd/dialerutil"
	"intrinsic/skills/tools/skill/cmd/imagetransfer"
	"intrinsic/skills/tools/skill/cmd/imageutil"
	"intrinsic/skills/tools/skill/cmd/solutionutil"
)

const (
	keyContext          = "context"
	keySolution         = "solution"
	keyInstallerAddress = "installer_address"
	keyType             = "type"
)

var viperLocal = viper.New()

var stopCmd = &cobra.Command{
	Use:   "stop --type=TYPE TARGET",
	Short: "Remove a skill",
	Example: `Stop a running skill using its build target
$ inctl skill stop --type=build //abc:skill.tar --context=minikube

Stop a running skill using an already-built image file
$ inctl skill stop --type=archive abc/skill.tar --context=minikube

Stop a running skill using an already-pushed image
$ inctl skill stop --type=image gcr.io/my-workcell/abc@sha256:20ab4f --context=minikube

Use the solution flag to automatically resolve the context (requires the solution to run)
$ inctl skill stop --type=archive abc/skill.tar --solution=my-solution

Stop a running skill by specifying its id
$ inctl skill stop --type=id com.foo.skill

Stop a running skill by specifying its name [deprecated]
$ inctl skill stop --type=id skill
`,
	Args: cobra.ExactArgs(1),
	RunE: func(command *cobra.Command, args []string) error {
		target := args[0]
		targetType := imageutil.TargetType(viperLocal.GetString(keyType))
		if targetType != imageutil.Build && targetType != imageutil.Archive && targetType != imageutil.Image && targetType != imageutil.ID && targetType != imageutil.Name {
			return fmt.Errorf("type must be one of (%s, %s, %s, %s, %s)", imageutil.Build, imageutil.Archive, imageutil.Image, imageutil.ID, imageutil.Name)
		}

		context := viperLocal.GetString(keyContext)
		installerAddress := viperLocal.GetString(keyInstallerAddress)
		solution := viperLocal.GetString(keySolution)
		project := viper.GetString(cmd.KeyProject)

		skillID, err := imageutil.SkillIDFromTarget(target, imageutil.TargetType(targetType), imagetransfer.RemoteTransferer(remote.WithAuthFromKeychain(google.Keychain)))
		if err != nil {
			return fmt.Errorf("could not get skill ID: %v", err)
		}

		ctx, conn, err := dialerutil.DialConnectionCtx(command.Context(), dialerutil.DialInfoParams{
			Address:  installerAddress,
			CredName: project,
		})
		if err != nil {
			return fmt.Errorf("could not create connection: %w", err)
		}
		defer conn.Close()

		cluster, err := solutionutil.GetClusterNameFromSolutionOrDefault(
			ctx,
			conn,
			solution,
			context,
		)
		if err != nil {
			return fmt.Errorf("could not resolve solution to cluster: %s", err)
		}

		// Remove the skill from the registry
		ctx, conn, err = dialerutil.DialConnectionCtx(command.Context(), dialerutil.DialInfoParams{
			Address:  installerAddress,
			Cluster:  cluster,
			CredName: project,
		})
		if err != nil {
			return fmt.Errorf("could not remove the skill: %w", err)
		}

		log.Printf("Removing skill %q using the installer service at %q", skillID, installerAddress)
		if err := imageutil.RemoveContainer(ctx, &imageutil.RemoveContainerParams{
			Address:    installerAddress,
			Connection: conn,
			Request: &installerpb.RemoveContainerAddonRequest{
				Id:   skillID,
				Type: installerpb.AddonType_ADDON_TYPE_SKILL,
			},
		}); err != nil {
			return fmt.Errorf("could not remove the skill: %w", err)
		}
		log.Print("Finished removing the skill")

		return nil
	},
}

func init() {
	cmd.SkillCmd.AddCommand(stopCmd)

	stopCmd.PersistentFlags().StringP(keyContext, "c", "", "The Kubernetes cluster to use. Not required if using localhost for the installer_address. You can set the environment variable INTRINSIC_CONTEXT=cluster to set a default cluster.")
	stopCmd.PersistentFlags().String(keySolution, "", `The solution in which the skill runs. Not required if using localhost for the installer_address.
You can set the environment variable INTRINSIC_SOLUTION=solution to set a default solution.`)
	stopCmd.PersistentFlags().String(keyInstallerAddress, "xfa.lan:17080", "The address of the installer service. When not running the cluster on localhost, this should be the address of the relay (e.g. dns:///www.endpoints.<gcloud_project_name>.cloud.goog:443). You can set the environment variable INTRINSIC_INSTALLER_ADDRESS=address to change the default address.")
	stopCmd.PersistentFlags().String(keyType, "", fmt.Sprintf(`(required) The target's type:
%s	build target that creates a skill image
%s	file path pointing to an already-built image
%s	container image name
%s		skill id
%s		skill name [deprecated: prefer to use "id"]`, imageutil.Build, imageutil.Archive, imageutil.Image, imageutil.ID, imageutil.Name))
	stopCmd.MarkPersistentFlagRequired(keyType)
	stopCmd.MarkFlagsMutuallyExclusive(keyContext, keySolution)

	viperLocal.BindPFlag(keyContext, stopCmd.PersistentFlags().Lookup(keyContext))
	viperLocal.BindPFlag(keySolution, stopCmd.PersistentFlags().Lookup(keySolution))
	viperLocal.BindPFlag(keyInstallerAddress, stopCmd.PersistentFlags().Lookup(keyInstallerAddress))
	viperLocal.BindPFlag(keyType, stopCmd.PersistentFlags().Lookup(keyType))
	viperLocal.SetEnvPrefix("intrinsic")
	viperLocal.BindEnv(keyInstallerAddress)
	viperLocal.BindEnv(cmd.KeyProject)
}
