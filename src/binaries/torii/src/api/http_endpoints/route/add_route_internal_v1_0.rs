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

use crate::config;
use crate::core::routing::*;

use ainari_api::common_functions::*;
use ainari_api::errors::ErrorResponse;
use ainari_api_structs::route_structs::*;
use ainari_api_structs::user_context::UserContext;

use aya::maps::{HashMap as AyaHashMap, MapData};
use std::net::Ipv4Addr;
use torii_common::RouteTarget;

// --- Utilities ---
fn get_ifindex(name: &str) -> u32 {
    let path = format!("/sys/class/net/{}/ifindex", name);
    std::fs::read_to_string(path)
        .unwrap_or_else(|_| "0\n".to_string())
        .trim()
        .parse()
        .unwrap_or(0)
}

/// Retrieves the MAC address of a network interface.
///
/// This function reads the `/sys/class/net/{iface}/address` file and parses
/// the hex string into a standard 6-byte array.
///
/// # Arguments
/// * `iface` - The name of the network interface
///
/// # Returns
/// A 6-byte array `[u8; 6]` containing the MAC address. Defaults to zeros on failure.
fn get_mac_address(iface: &str) -> [u8; 6] {
    let path = format!("/sys/class/net/{}/address", iface);
    let mac_str = std::fs::read_to_string(path).unwrap_or_else(|_| "00:00:00:00:00:00".to_string());
    let mut mac = [0u8; 6];
    for (i, byte) in mac_str.trim().split(':').enumerate() {
        if i < 6 {
            mac[i] = u8::from_str_radix(byte, 16).unwrap_or(0);
        }
    }
    mac
}

/// Retrieves the primary IPv4 address of a local network interface.
///
/// This function executes the `ip -4 addr show` command and parses the output
/// to locate the first available inet address assigned to the interface.
///
/// # Arguments
/// * `iface` - The name of the network interface
///
/// # Returns
/// An `Option<String>` containing the IP address, or None if unavailable.
fn get_local_ip(iface: &str) -> Option<String> {
    let output = std::process::Command::new("ip")
        .arg("-4")
        .arg("addr")
        .arg("show")
        .arg(iface)
        .output()
        .ok()?;
    let stdout = String::from_utf8_lossy(&output.stdout);
    for line in stdout.lines() {
        if line.contains("inet ") {
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() >= 2 {
                return Some(parts[1].split('/').next()?.to_string());
            }
        }
    }
    None
}

/// Resolves the MAC address of a target IP using ARP.
///
/// This function pings the target IP to force an ARP resolution, then parses
/// the `/proc/net/arp` table to find the corresponding MAC address. It retries
/// up to 10 times to allow network convergence.
///
/// # Arguments
/// * `ip` - The target IPv4 address as a string
///
/// # Returns
/// A 6-byte array `[u8; 6]` containing the resolved MAC address, or a broadcast address (0xff) on failure.
fn get_arp_mac(ip: &str) -> [u8; 6] {
    for _ in 0..10 {
        std::process::Command::new("ping")
            .arg("-c")
            .arg("1")
            .arg("-W")
            .arg("1")
            .arg(ip)
            .output()
            .ok();
        if let Ok(arp_table) = std::fs::read_to_string("/proc/net/arp") {
            for line in arp_table.lines().skip(1) {
                let parts: Vec<&str> = line.split_whitespace().collect();
                if parts.len() >= 4 && parts[0] == ip {
                    let mac_str = parts[3];
                    if mac_str != "00:00:00:00:00:00" {
                        let mut mac = [0u8; 6];
                        for (i, byte) in mac_str.split(':').enumerate() {
                            if i < 6 {
                                mac[i] = u8::from_str_radix(byte, 16).unwrap_or(0);
                            }
                        }
                        return mac;
                    }
                }
            }
        }
        std::thread::sleep(std::time::Duration::from_millis(200));
    }
    [0xff; 6]
}

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

    // eBPF maps operate on raw binary data, so we must parse the destination IP
    // string into an Ipv4Addr, and then convert it into a 32-bit unsigned integer (u32).
    let ip_addr: Ipv4Addr = match body.dest_ip.parse() {
        Ok(ip) => ip,
        Err(_) => return Err(ErrorResponse::BadRequest(format!("Invalid IP"))),
    };
    let ip_u32 = u32::from(ip_addr);

    // The kernel's networking stack and eBPF redirect helpers don't understand
    // interface names (like "eth0" or "tap-vm1"). They require the numeric ifindex.
    let ifindex = get_ifindex(&body.target_iface);
    if ifindex == 0 {
        return Err(ErrorResponse::NotFound(format!(
            "Interface {} not found",
            body.target_iface
        )));
    }

    // Prepare default encapsulation variables.
    // `action = 0` means standard local routing (no tunneling).
    let mut action = 0;
    let mut encap_dst_ip = 0;
    let mut encap_dst_mac = [0u8; 6];
    let mut encap_src_ip = 0;
    let mut encap_src_mac = [0u8; 6];

    // If a gateway_ip was provided, it means the target VM lives on a DIFFERENT host.
    // We must encapsulate the packet in a UDP tunnel to cross the physical underlay network.
    if !body.gateway_ip.is_empty() {
        // `action = 1` tells the eBPF dataplane to encapsulate the packet before sending.
        action = 1;

        // Parse the target underlay node's IP.
        let dst_ip: Ipv4Addr = match body.gateway_ip.parse() {
            Ok(ip) => ip,
            Err(_) => return Err(ErrorResponse::BadRequest(format!("Invalid Invalid"))),
        };
        encap_dst_ip = u32::from(dst_ip);

        // Resolve the MAC address of the target gateway IP via ARP.
        // The eBPF program needs this to construct the outer Ethernet header.
        encap_dst_mac = get_arp_mac(&body.gateway_ip);

        // Resolve our own local IP on the physical underlay interface ("eth0").
        // This will be the Source IP of the outer encapsulated packet.
        let local_ip_str = get_local_ip("eth0").unwrap_or_else(|| "0.0.0.0".to_string());
        let local_ip: Ipv4Addr = local_ip_str.parse().unwrap_or(Ipv4Addr::new(0, 0, 0, 0));
        encap_src_ip = u32::from(local_ip);

        // Resolve our own physical MAC address to use as the Source MAC.
        encap_src_mac = get_mac_address("eth0");
    }

    // Construct the RouteTarget payload matching the exact C-struct layout expected
    // by the eBPF program. `_pad1` and `_pad2` ensure proper byte alignment.
    let target = RouteTarget {
        action,
        ifindex,
        encap_dst_ip,
        encap_dst_mac,
        _pad1: [0; 2],
        encap_src_ip,
        encap_src_mac,
        _pad2: [0; 2],
    };

    let new_id = Uuid::new_v4().to_string();
    let mut st = ROUTE_HANDLER.lock().await;

    let route = Route {
        id: new_id.clone(),
        dest_ip: body.dest_ip.clone(),
        target_iface: body.target_iface.clone(),
        gateway_ip: body.gateway_ip.clone(),
    };

    st.routes.insert(new_id.clone(), route.clone());

    // Write the raw struct directly into the eBPF map.
    // This immediately alters the live packet forwarding behavior in the kernel.
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
