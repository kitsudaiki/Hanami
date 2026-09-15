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

use actix_web::web::Path;
use apistos::actix::NoContent;
use apistos::api_operation;
use std::net::Ipv4Addr;

use crate::core::routing_interface::ROUTE_HANDLER;

use ainari_api::errors::ErrorResponse;
use ainari_api_structs::user_context::UserContext;

#[api_operation(
    tag = "floating_ip",
    summary = "Delete floating-ip",
    description = r###"Remove a floating-ip NAT configuration.

The NAT definitions are dropped from the internal tracking state as well as from
both eBPF NAT maps, which terminates the external access."###,
    error_code = 400,
    error_code = 401,
    error_code = 404,
    error_code = 500
)]
pub async fn delete_floating_ip_internal(
    floating_ip: Path<String>,
    _context: UserContext,
) -> Result<NoContent, ErrorResponse> {
    let floating_ip = floating_ip.into_inner();

    let fip_addr: Ipv4Addr = match floating_ip.parse() {
        Ok(addr) => addr,
        Err(_) => return Err(ErrorResponse::BadRequest("Invalid FIP".to_string())),
    };

    let mut st = ROUTE_HANDLER.lock().await;

    let internal_ip = match st.floating_ips.remove(&floating_ip) {
        Some(internal_ip) => internal_ip,
        None => return Err(ErrorResponse::NotFound("Floating IP not found".to_string())),
    };

    let _ = st.fip_dnat_map.remove(&u32::from(fip_addr));
    if let Ok(int_addr) = internal_ip.parse::<Ipv4Addr>() {
        let _ = st.fip_snat_map.remove(&u32::from(int_addr));
    }

    Ok(NoContent)
}
