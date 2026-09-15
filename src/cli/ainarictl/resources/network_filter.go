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
	networkFilterIpRanges []string
	networkFilterPorts    []string
)

var getNetworkFilterCmd = &cobra.Command{
	Use:   "get ROUTE_UUID",
	Short: "Get the packet-filter of a specific route.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.GetNetworkFilter(context, routeUuid)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var listNetworkFilterCmd = &cobra.Command{
	Use:   "list",
	Short: "List the packet-filter of all route.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.ListNetworkFilter(context)
		if err == nil {
			ainarictl_common.PrintList(content["filters"].([]interface{}))
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var clearNetworkFilterCmd = &cobra.Command{
	Use:   "clear ROUTE_UUID",
	Short: "Remove the complete packet-filter of a specific route.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.ClearNetworkFilter(context, routeUuid)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var addNetworkFilterIpRangeCmd = &cobra.Command{
	Use:   "add_ip_range -r IP_RANGE ROUTE_UUID",
	Short: "Add IP-ranges to the packet-filter of a specific route.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.AddNetworkFilterIpRange(context, routeUuid, networkFilterIpRanges)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var deleteNetworkFilterIpRangeCmd = &cobra.Command{
	Use:   "delete_ip_range -r IP_RANGE ROUTE_UUID",
	Short: "Remove IP-ranges from the packet-filter of a specific route.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.DeleteNetworkFilterIpRange(context, routeUuid, networkFilterIpRanges)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var addNetworkFilterPortCmd = &cobra.Command{
	Use:   "add_port -p PORT ROUTE_UUID",
	Short: "Add ports to the packet-filter of a specific route.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.AddNetworkFilterPort(context, routeUuid, networkFilterPorts)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var deleteNetworkFilterPortCmd = &cobra.Command{
	Use:   "delete_port -p PORT ROUTE_UUID",
	Short: "Remove ports from the packet-filter of a specific route.",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		routeUuid := args[0]
		content, err := ainari_sdk.DeleteNetworkFilterPort(context, routeUuid, networkFilterPorts)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var networkFilterCmd = &cobra.Command{
	Use:   "network_filter",
	Short: "Manage the packet-filter of the route.",
}

func Init_NetworkFilter_Commands(rootCmd *cobra.Command) {
	rootCmd.AddCommand(networkFilterCmd)

	networkFilterCmd.AddCommand(getNetworkFilterCmd)

	networkFilterCmd.AddCommand(listNetworkFilterCmd)

	networkFilterCmd.AddCommand(clearNetworkFilterCmd)

	networkFilterCmd.AddCommand(addNetworkFilterIpRangeCmd)
	addNetworkFilterIpRangeCmd.Flags().StringSliceVarP(&networkFilterIpRanges, "range", "r", []string{}, "IP-range like '10.0.0.7', '10.0.0.0/24' or '10.0.0.5-10.0.0.9' (mandatory)")
	addNetworkFilterIpRangeCmd.MarkFlagRequired("range")

	networkFilterCmd.AddCommand(deleteNetworkFilterIpRangeCmd)
	deleteNetworkFilterIpRangeCmd.Flags().StringSliceVarP(&networkFilterIpRanges, "range", "r", []string{}, "IP-range like '10.0.0.7', '10.0.0.0/24' or '10.0.0.5-10.0.0.9' (mandatory)")
	deleteNetworkFilterIpRangeCmd.MarkFlagRequired("range")

	networkFilterCmd.AddCommand(addNetworkFilterPortCmd)
	addNetworkFilterPortCmd.Flags().StringSliceVarP(&networkFilterPorts, "port", "p", []string{}, "Port like '22' or '8000-8100' (mandatory)")
	addNetworkFilterPortCmd.MarkFlagRequired("port")

	networkFilterCmd.AddCommand(deleteNetworkFilterPortCmd)
	deleteNetworkFilterPortCmd.Flags().StringSliceVarP(&networkFilterPorts, "port", "p", []string{}, "Port like '22' or '8000-8100' (mandatory)")
	deleteNetworkFilterPortCmd.MarkFlagRequired("port")
}
