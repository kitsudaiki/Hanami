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

func GetNetworkFilter(context AccessContext, routeUuid string) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/route/%s/filter", routeUuid)
	vars := map[string]interface{}{}
	return SendGet(context, context.ToriiAddress, path, vars)
}

func ListNetworkFilter(context AccessContext) (map[string]interface{}, error) {
	path := "v1alpha/network_filter"
	vars := map[string]interface{}{}
	return SendGet(context, context.ToriiAddress, path, vars)
}

func ClearNetworkFilter(context AccessContext, routeUuid string) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/route/%s/filter/internal", routeUuid)
	vars := map[string]interface{}{}
	return SendDelete(context, context.ToriiAddress, path, vars)
}

func AddNetworkFilterIpRange(context AccessContext, routeUuid string, ranges []string) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/route/%s/filter/ip_range/internal", routeUuid)
	jsonBody := map[string]interface{}{
		"ranges": ranges,
	}
	return SendPost(context, context.ToriiAddress, path, jsonBody)
}

func DeleteNetworkFilterIpRange(context AccessContext, routeUuid string, ranges []string) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/route/%s/filter/ip_range/internal", routeUuid)
	jsonBody := map[string]interface{}{
		"ranges": ranges,
	}
	return SendDeleteWithBody(context, context.ToriiAddress, path, jsonBody)
}

func AddNetworkFilterPort(context AccessContext, routeUuid string, ports []string) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/route/%s/filter/port/internal", routeUuid)
	jsonBody := map[string]interface{}{
		"ports": ports,
	}
	return SendPost(context, context.ToriiAddress, path, jsonBody)
}

func DeleteNetworkFilterPort(context AccessContext, routeUuid string, ports []string) (map[string]interface{}, error) {
	path := fmt.Sprintf("v1alpha/route/%s/filter/port/internal", routeUuid)
	jsonBody := map[string]interface{}{
		"ports": ports,
	}
	return SendDeleteWithBody(context, context.ToriiAddress, path, jsonBody)
}
