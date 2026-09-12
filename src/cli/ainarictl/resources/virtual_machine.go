/**
 * @author      Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright   Apache License Version 2.0
 *
 *      Copyright 2022-2026 Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

package ainari_resources

import (
	"fmt"
	ainarictl_common "ainarictl/common"
	"os"

	ainari_sdk "github.com/kitsudaiki/ainari"
	"github.com/spf13/cobra"
)

var (
	templatePath   string
	checkpointName string
	virtual_machineMode    string
)

var createVirtualMachineCmd = &cobra.Command{
	Use:   "create NAME",
	Short: "Create a new virtual machine.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		virtual_machineName := args[0]
		content, err := ainari_sdk.CreateVirtualMachine(context, virtual_machineName)
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		ainarictl_common.PrintSingle(content)
	},
}

var getVirtualMachineCmd = &cobra.Command{
	Use:   "get CLUSTER_UUID",
	Short: "Get information of a specific virtual machine.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		virtual_machineUuid := args[0]
		content, err := ainari_sdk.GetVirtualMachine(context, virtual_machineUuid)
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		ainarictl_common.PrintSingle(content)
	},
}

var listVirtualMachineCmd = &cobra.Command{
	Use:   "list",
	Short: "List all virtual machine.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.ListVirtualMachine(context)
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		ainarictl_common.PrintList(content["virtual_machines"].([]interface{}))
	},
}

var deleteVirtualMachineCmd = &cobra.Command{
	Use:   "delete CLUSTER_UUID",
	Short: "Delete a specific virtual machine from the backend.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		virtual_machineUuid := args[0]
		_, err = ainari_sdk.DeleteVirtualMachine(context, virtual_machineUuid)
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		fmt.Printf("successfully deleted virtual machine '%v'\n", virtual_machineUuid)
	},
}

var virtual_machineCmd = &cobra.Command{
	Use:   "vm",
	Short: "Manage virtual machines.",
}

func Init_VirtualMachine_Commands(rootCmd *cobra.Command) {
	rootCmd.AddCommand(virtual_machineCmd)

	virtual_machineCmd.AddCommand(createVirtualMachineCmd)
	// createVirtualMachineCmd.Flags().StringVarP(&templatePath, "template", "t", "", "VirtualMachine Template (mandatory)")
	// createVirtualMachineCmd.MarkFlagRequired("template")

	virtual_machineCmd.AddCommand(getVirtualMachineCmd)

	virtual_machineCmd.AddCommand(listVirtualMachineCmd)

	virtual_machineCmd.AddCommand(deleteVirtualMachineCmd)
}
