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

package ainari_sdk

func ConfigNetworkInterface(context AccessContext, ifaceName, ipCidr string, up bool) (map[string]interface{}, error) {
	path := "v1alpha/network_interface/config/internal"
	jsonBody := map[string]interface{}{
		"iface_name": ifaceName,
		"up":         up,
	}
	// without an address the interface is only switched up or down
	if ipCidr != "" {
		jsonBody["ip_cidr"] = ipCidr
	}
	return SendPost(context, context.ToriiAddress, path, jsonBody)
}

func RegisterNetworkTap(context AccessContext, tapName, vmMac, vmIp string) (map[string]interface{}, error) {
	path := "v1alpha/network_interface/tap/internal"
	jsonBody := map[string]interface{}{
		"tap_name": tapName,
	}
	// the address of the VM is only required to program the ARP-responder of the datapath
	if vmMac != "" {
		jsonBody["vm_mac"] = vmMac
	}
	if vmIp != "" {
		jsonBody["vm_ip"] = vmIp
	}
	return SendPost(context, context.ToriiAddress, path, jsonBody)
}
