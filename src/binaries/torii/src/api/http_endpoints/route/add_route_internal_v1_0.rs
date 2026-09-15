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
use uuid::Uuid;
use validator::Validate;

use crate::core::routing_interface::*;


use actix_web::{App, HttpResponse, HttpServer, Responder, delete, get, post, put, web};
use aya::maps::HashMap as AyaHashMap;
use aya::programs::{Xdp, XdpFlags};
use aya::{Bpf, include_bytes_aligned};
use std::collections::HashMap;
use std::net::Ipv4Addr;
use std::sync::Arc;
use tokio::signal;
use tokio::sync::Mutex;

use crate::core::crypto::{
    apply_connection_policies, install_sa, normalize_key, remove_block_policies,
};
use crate::core::filter::{build_route_filter, parse_ip_range, parse_port_range};
use crate::core::models::RouteTargetPod;
use crate::core::routing::build_route_target;
use crate::core::state::GatewayState;
use crate::core::utils::{
    enable_forwarding, get_ifindex, get_local_ip, get_mac_address, parse_mac, run_ip,
};

use ainari_api_structs::route_structs::*;
use torii_common::{ArpProxy, RouteTarget};
use ainari_api::errors::ErrorResponse;
use ainari_api_structs::route_structs::*;
use ainari_api_structs::user_context::UserContext;

#[api_operation(
    tag = "route",
    summary = "Register new route",
    description = r###"Register new route."###,
    error_code = 400,
    error_code = 401,
    error_code = 500
)]
pub async fn register_route_internal(
    body: Json<RouteRequest>,
    context: UserContext,
) -> Result<CreatedJson<RouteResponse>, ErrorResponse> {
    // validate incoming json
    body.validate()
        .map_err(|e| ErrorResponse::BadRequest(format!("Invalid input: {e}")))?;

    let ip_addr: Ipv4Addr = match body.dest_ip.parse() {
        Ok(ip) => ip,
        Err(_) => return Err(ErrorResponse::BadRequest(format!("Invalid IP"))),
    };
    let ip_u32 = u32::from(ip_addr);

    if get_ifindex(&body.target_iface) == 0 {
        return Err(ErrorResponse::NotFound(format!(
            "Interface {} not found",
            body.target_iface
        )));
    }

    // Snapshot the TAP registry so the (possibly slow) ARP resolution inside
    // the target construction does not block the rest of the gateway.
    let taps = { ROUTE_HANDLER.lock().await.taps.clone() };
    let target = match build_route_target(&body, &taps) {
        Ok(target) => target,
        Err(err) => return Err(ErrorResponse::BadRequest(format!("Invalid Input"))),
    };

    let route_uuid = Uuid::new_v4();
    let mut st = ROUTE_HANDLER.lock().await;

    let route = Route {
        uuid: route_uuid,
        dest_ip: body.dest_ip.clone(),
        target_iface: body.target_iface.clone(),
        gateway_ip: body.gateway_ip.clone(),
        next_hop_ip: body.next_hop_ip.clone(),
        next_hop_mac: body.next_hop_mac.clone(),
        encrypted: body.encrypted,
    };

    st.routes.insert(route_uuid, route.clone());
    if st
        .route_map
        .insert(ip_u32, RouteTargetPod(target), 0)
        .is_err()
    {
        return Err(ErrorResponse::InternalError(format!("eBPF Map error")));
    }

    let resp = RouteResponse {
        success: true,
        message: "Route created".to_string(),
        route: Some(route),
    };

    Ok(CreatedJson(resp))
}
