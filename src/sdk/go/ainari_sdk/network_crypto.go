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

import (
	"fmt"
)

func AddNetworkCryptoKey(context AccessContext, direction, localIp, remoteIp, peerGatewayIp string, spi uint32, key string) (map[string]interface{}, error) {
	path := "v1alpha/network_crypto/key/internal"
	jsonBody := map[string]interface{}{
		"direction":       direction,
		"local_ip":        localIp,
		"remote_ip":       remoteIp,
		"peer_gateway_ip": peerGatewayIp,
		"spi":             spi,
		"key":             key,
	}
	return SendPost(context, context.ToriiAddress, path, jsonBody)
}

func ListNetworkCryptoKey(context AccessContext) (map[string]interface{}, error) {
	path := "v1alpha/network_crypto/key"
	vars := map[string]interface{}{}
	return SendGet(context, context.ToriiAddress, path, vars)
}

func DeleteNetworkCryptoKey(context AccessContext, direction string, spi uint32) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/network_crypto/key/%s/%d/internal", direction, spi)
	vars := map[string]interface{}{}
	return SendDelete(context, context.ToriiAddress, path, vars)
}

func ToggleNetworkCrypto(context AccessContext, localIp, remoteIp, peerGatewayIp string, enabled bool) (map[string]interface{}, error) {
	path := "v1alpha/network_crypto/toggle/internal"
	jsonBody := map[string]interface{}{
		"local_ip":  localIp,
		"remote_ip": remoteIp,
		"enabled":   enabled,
	}
	// the gateway of the peer is only required for a connection, which is not known yet
	if peerGatewayIp != "" {
		jsonBody["peer_gateway_ip"] = peerGatewayIp
	}
	return SendPost(context, context.ToriiAddress, path, jsonBody)
}

func ListNetworkConnection(context AccessContext) (map[string]interface{}, error) {
	path := "v1alpha/network_crypto/connection"
	vars := map[string]interface{}{}
	return SendGet(context, context.ToriiAddress, path, vars)
}
