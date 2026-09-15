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
	routeDestIp      string
	routeTargetIface string
	routeGatewayIp   string
	routeNextHopIp   string
	routeNextHopMac  string
	routeEncrypted   bool
)

var addRouteCmd = &cobra.Command{
	Use:   "add -i TARGET_IFACE -g GATEWAY_IP DEST_IP",
	Short: "Add a new route to the gateway.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		destIp := args[0]
		content, err := ainari_sdk.AddRoute(context,
			destIp,
			routeTargetIface,
			routeGatewayIp,
			routeNextHopIp,
			routeNextHopMac,
			routeEncrypted)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var listRouteCmd = &cobra.Command{
	Use:   "list",
	Short: "List all route.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.ListRoute(context)
		if err == nil {
			ainarictl_common.PrintList(content["routes"].([]interface{}))
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var updateRouteCmd = &cobra.Command{
	Use:   "update -d DEST_IP -i TARGET_IFACE -g GATEWAY_IP ROUTE_UUID",
	Short: "Update an existing route of the gateway.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.UpdateRoute(context,
			routeUuid,
			routeDestIp,
			routeTargetIface,
			routeGatewayIp,
			routeNextHopIp,
			routeNextHopMac,
			routeEncrypted)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var deleteRouteCmd = &cobra.Command{
	Use:   "delete ROUTE_UUID",
	Short: "Delete a specific route from the gateway.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		_, err = ainari_sdk.DeleteRoute(context, routeUuid)
		if err == nil {
			fmt.Printf("successfully deleted route '%v'\n", routeUuid)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var routeCmd = &cobra.Command{
	Use:   "route",
	Short: "Manage route.",
}

func Init_Route_Commands(rootCmd *cobra.Command) {
	rootCmd.AddCommand(routeCmd)

	routeCmd.AddCommand(addRouteCmd)
	addRouteCmd.Flags().StringVarP(&routeTargetIface, "iface", "i", "", "Name of the interface, which the traffic has to leave (mandatory)")
	addRouteCmd.Flags().StringVarP(&routeGatewayIp, "gateway", "g", "", "Address of the remote gateway, or empty for a local route")
	addRouteCmd.Flags().StringVar(&routeNextHopIp, "next_hop_ip", "", "Address of the next hop on the link-layer")
	addRouteCmd.Flags().StringVar(&routeNextHopMac, "next_hop_mac", "", "MAC-address of the next hop on the link-layer")
	addRouteCmd.Flags().BoolVar(&routeEncrypted, "encrypted", false, "Reach the destination over an IPsec protected connection")
	addRouteCmd.MarkFlagRequired("iface")

	routeCmd.AddCommand(listRouteCmd)

	routeCmd.AddCommand(updateRouteCmd)
	updateRouteCmd.Flags().StringVarP(&routeDestIp, "dest", "d", "", "Destination-address of the route (mandatory)")
	updateRouteCmd.Flags().StringVarP(&routeTargetIface, "iface", "i", "", "Name of the interface, which the traffic has to leave (mandatory)")
	updateRouteCmd.Flags().StringVarP(&routeGatewayIp, "gateway", "g", "", "Address of the remote gateway, or empty for a local route")
	updateRouteCmd.Flags().StringVar(&routeNextHopIp, "next_hop_ip", "", "Address of the next hop on the link-layer")
	updateRouteCmd.Flags().StringVar(&routeNextHopMac, "next_hop_mac", "", "MAC-address of the next hop on the link-layer")
	updateRouteCmd.Flags().BoolVar(&routeEncrypted, "encrypted", false, "Reach the destination over an IPsec protected connection")
	updateRouteCmd.MarkFlagRequired("dest")
	updateRouteCmd.MarkFlagRequired("iface")

	routeCmd.AddCommand(deleteRouteCmd)
}
