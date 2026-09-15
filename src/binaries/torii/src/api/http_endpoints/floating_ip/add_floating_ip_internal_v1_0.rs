// Copyright 2022-2026 Tobias Anker <tobias.anker@kitsunemimi.moe>

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use actix_web::web::Json;
use apistos::actix::CreatedJson;
use apistos::api_operation;
use std::net::Ipv4Addr;
use validator::Validate;

use crate::core::routing_interface::ROUTE_HANDLER;

use ainari_api::errors::ErrorResponse;
use ainari_api_structs::route_structs::*;
use ainari_api_structs::user_context::UserContext;

#[api_operation(
    tag = "floating_ip",
    summary = "Register new floating-ip",
    description = r###"Provision a floating IP and register its SNAT/DNAT rules in eBPF.

The floating IP is associated with a private internal IP, which updates both the
`FIP_DNAT_MAP` and the `FIP_SNAT_MAP` of the datapath."###,
    error_code = 400,
    error_code = 401,
    error_code = 500
)]
pub async fn register_floating_ip_internal(
    body: Json<FloatingIpRequest>,
    _context: UserContext,
) -> Result<CreatedJson<RouteResponse>, ErrorResponse> {
    // validate incoming json
    body.validate()
        .map_err(|e| ErrorResponse::BadRequest(format!("Invalid input: {e}")))?;

    let fip_addr: Ipv4Addr = match body.floating_ip.parse() {
        Ok(ip) => ip,
        Err(_) => return Err(ErrorResponse::BadRequest("Invalid FIP".to_string())),
    };
    let int_addr: Ipv4Addr = match body.internal_ip.parse() {
        Ok(ip) => ip,
        Err(_) => return Err(ErrorResponse::BadRequest("Invalid Internal IP".to_string())),
    };

    let mut st = ROUTE_HANDLER.lock().await;

    st.floating_ips
        .insert(body.floating_ip.clone(), body.internal_ip.clone());

    if st
        .fip_dnat_map
        .insert(u32::from(fip_addr), u32::from(int_addr), 0)
        .is_err()
    {
        return Err(ErrorResponse::InternalError(
            "eBPF Map error (DNAT)".to_string(),
        ));
    }
    if st
        .fip_snat_map
        .insert(u32::from(int_addr), u32::from(fip_addr), 0)
        .is_err()
    {
        return Err(ErrorResponse::InternalError(
            "eBPF Map error (SNAT)".to_string(),
        ));
    }

    let resp = RouteResponse {
        success: true,
        message: "Floating IP mapped".to_string(),
        route: None,
    };

    Ok(CreatedJson(resp))
}
