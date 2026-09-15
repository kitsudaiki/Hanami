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
	networkInterfaceIpCidr string
	networkInterfaceUp     bool
	networkInterfaceVmMac  string
	networkInterfaceVmIp   string
)

var configNetworkInterfaceCmd = &cobra.Command{
	Use:   "config IFACE_NAME",
	Short: "Configure an existing network-interface.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		ifaceName := args[0]
		content, err := ainari_sdk.ConfigNetworkInterface(context, ifaceName, networkInterfaceIpCidr, networkInterfaceUp)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var registerNetworkTapCmd = &cobra.Command{
	Use:   "tap TAP_NAME",
	Short: "Create a new TAP-device for a virtual machine.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		tapName := args[0]
		content, err := ainari_sdk.RegisterNetworkTap(context, tapName, networkInterfaceVmMac, networkInterfaceVmIp)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var networkInterfaceCmd = &cobra.Command{
	Use:   "network_interface",
	Short: "Manage network interfaces.",
}

func Init_NetworkInterface_Commands(rootCmd *cobra.Command) {
	rootCmd.AddCommand(networkInterfaceCmd)

	networkInterfaceCmd.AddCommand(configNetworkInterfaceCmd)
	configNetworkInterfaceCmd.Flags().StringVar(&networkInterfaceIpCidr, "ip_cidr", "", "Address of the interface in CIDR-notation")
	configNetworkInterfaceCmd.Flags().BoolVar(&networkInterfaceUp, "up", false, "Bring the interface up instead of down")

	networkInterfaceCmd.AddCommand(registerNetworkTapCmd)
	registerNetworkTapCmd.Flags().StringVar(&networkInterfaceVmMac, "vm_mac", "", "MAC-address of the VM, which is attached to the device")
	registerNetworkTapCmd.Flags().StringVar(&networkInterfaceVmIp, "vm_ip", "", "Address of the VM, which is attached to the device")
}
