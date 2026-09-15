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
	"strconv"

	ainari_sdk "github.com/kitsudaiki/ainari"
	"github.com/spf13/cobra"
)

var (
	networkCryptoDirection     string
	networkCryptoLocalIp       string
	networkCryptoRemoteIp      string
	networkCryptoPeerGatewayIp string
	networkCryptoSpi           uint32
	networkCryptoKey           string
	networkCryptoEnabled       bool
)

var addNetworkCryptoKeyCmd = &cobra.Command{
	Use:   "add_key -d DIRECTION -l LOCAL_IP -r REMOTE_IP -g PEER_GATEWAY_IP -s SPI -k KEY",
	Short: "Register a new crypto-key for one direction of a connection.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.AddNetworkCryptoKey(context,
			networkCryptoDirection,
			networkCryptoLocalIp,
			networkCryptoRemoteIp,
			networkCryptoPeerGatewayIp,
			networkCryptoSpi,
			networkCryptoKey)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var listNetworkCryptoKeyCmd = &cobra.Command{
	Use:   "list_key",
	Short: "List all crypto-key.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.ListNetworkCryptoKey(context)
		if err == nil {
			ainarictl_common.PrintList(content["keys"].([]interface{}))
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var deleteNetworkCryptoKeyCmd = &cobra.Command{
	Use:   "delete_key DIRECTION SPI",
	Short: "Delete a specific crypto-key from the gateway.",
	Args:  cobra.ExactArgs(2),
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		direction := args[0]
		spi, err := strconv.ParseUint(args[1], 10, 32)
		if err != nil {
			fmt.Println("SPI must be a positive number")
			os.Exit(1)
		}
		_, err = ainari_sdk.DeleteNetworkCryptoKey(context, direction, uint32(spi))
		if err == nil {
			fmt.Printf("successfully deleted crypto-key '%v:%v'\n", direction, spi)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var toggleNetworkCryptoCmd = &cobra.Command{
	Use:   "toggle -l LOCAL_IP -r REMOTE_IP",
	Short: "Switch the encryption of a connection on or off.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.ToggleNetworkCrypto(context,
			networkCryptoLocalIp,
			networkCryptoRemoteIp,
			networkCryptoPeerGatewayIp,
			networkCryptoEnabled)
		if err == nil {
			ainarictl_common.PrintSingle(content)
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var listNetworkConnectionCmd = &cobra.Command{
	Use:   "list_connection",
	Short: "List all connection.",
	Run: func(cmd *cobra.Command, args []string) {
		context, err := Login()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		content, err := ainari_sdk.ListNetworkConnection(context)
		if err == nil {
			ainarictl_common.PrintList(content["connections"].([]interface{}))
		} else {
			fmt.Println(err)
			os.Exit(1)
		}
	},
}

var networkCryptoCmd = &cobra.Command{
	Use:   "network_crypto",
	Short: "Manage the encryption of the connection.",
}

func Init_NetworkCrypto_Commands(rootCmd *cobra.Command) {
	rootCmd.AddCommand(networkCryptoCmd)

	networkCryptoCmd.AddCommand(addNetworkCryptoKeyCmd)
	addNetworkCryptoKeyCmd.Flags().StringVarP(&networkCryptoDirection, "direction", "d", "", "Direction of the key: 'egress' or 'ingress' (mandatory)")
	addNetworkCryptoKeyCmd.Flags().StringVarP(&networkCryptoLocalIp, "local", "l", "", "Address of the VM on this host (mandatory)")
	addNetworkCryptoKeyCmd.Flags().StringVarP(&networkCryptoRemoteIp, "remote", "r", "", "Address of the VM on the other side (mandatory)")
	addNetworkCryptoKeyCmd.Flags().StringVarP(&networkCryptoPeerGatewayIp, "gateway", "g", "", "Address of the gateway hosting the remote VM (mandatory)")
	addNetworkCryptoKeyCmd.Flags().Uint32VarP(&networkCryptoSpi, "spi", "s", 0, "SPI, which identifies the key on the wire (mandatory)")
	addNetworkCryptoKeyCmd.Flags().StringVarP(&networkCryptoKey, "key", "k", "", "AES-256-GCM key-material as hex-string of 72 digits (mandatory)")
	addNetworkCryptoKeyCmd.MarkFlagRequired("direction")
	addNetworkCryptoKeyCmd.MarkFlagRequired("local")
	addNetworkCryptoKeyCmd.MarkFlagRequired("remote")
	addNetworkCryptoKeyCmd.MarkFlagRequired("gateway")
	addNetworkCryptoKeyCmd.MarkFlagRequired("spi")
	addNetworkCryptoKeyCmd.MarkFlagRequired("key")

	networkCryptoCmd.AddCommand(listNetworkCryptoKeyCmd)

	networkCryptoCmd.AddCommand(deleteNetworkCryptoKeyCmd)

	networkCryptoCmd.AddCommand(toggleNetworkCryptoCmd)
	toggleNetworkCryptoCmd.Flags().StringVarP(&networkCryptoLocalIp, "local", "l", "", "Address of the VM on this host (mandatory)")
	toggleNetworkCryptoCmd.Flags().StringVarP(&networkCryptoRemoteIp, "remote", "r", "", "Address of the VM on the other side (mandatory)")
	toggleNetworkCryptoCmd.Flags().StringVarP(&networkCryptoPeerGatewayIp, "gateway", "g", "", "Address of the gateway hosting the remote VM, if the connection is not known yet")
	toggleNetworkCryptoCmd.Flags().BoolVar(&networkCryptoEnabled, "enabled", false, "Switch the encryption of the connection on")
	toggleNetworkCryptoCmd.MarkFlagRequired("local")
	toggleNetworkCryptoCmd.MarkFlagRequired("remote")

	networkCryptoCmd.AddCommand(listNetworkConnectionCmd)
}
