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

use aya::maps::{HashMap as AyaHashMap, MapData};
use aya::programs::{Xdp, XdpFlags};
use aya::{Bpf, include_bytes_aligned};
use std::collections::HashMap;
use std::net::Ipv4Addr;
use std::sync::Arc;
use tokio::signal;
use tokio::sync::Mutex;
use torii_common::RouteTarget;
use uuid::Uuid;

use ainari_api_structs::route_structs::*;

// --- Application State ---
pub struct GatewayState {
    pub routes: HashMap<String, Route>,
    pub floating_ips: HashMap<String, String>,
    pub route_map: AyaHashMap<MapData, u32, RouteTargetPod>,
    pub fip_dnat_map: AyaHashMap<MapData, u32, u32>,
    pub fip_snat_map: AyaHashMap<MapData, u32, u32>,
    pub bpf: Bpf, // Bpf instance kept in state for dynamic XDP attaching
    pub xdp_links: Vec<aya::programs::xdp::XdpLinkId>, // Keeps XDP attachments alive
}

lazy_static::lazy_static! {
    pub static ref ROUTE_HANDLER: Arc<Mutex<GatewayState>> = Arc::new(Mutex::new(init_routing()));
}

/// Retrieves the system index of a network interface.
///
/// This function reads the `/sys/class/net/{name}/ifindex` file to resolve the
/// numeric interface index used by the kernel and eBPF.
///
/// # Arguments
/// * `name` - The name of the network interface (e.g., "eth0")
///
/// # Returns
/// A `u32` representing the interface index, or 0 if not found
fn get_ifindex(name: &str) -> u32 {
    let path = format!("/sys/class/net/{}/ifindex", name);
    std::fs::read_to_string(path)
        .unwrap_or_else(|_| "0\n".to_string())
        .trim()
        .parse()
        .unwrap_or(0)
}

pub fn init_routing() -> GatewayState {
    let overlay_iface = std::env::var("OVERLAY_IFACE").unwrap_or_else(|_| "veth-gw".to_string());
    let underlay_iface = std::env::var("UNDERLAY_IFACE").unwrap_or_else(|_| "eth0".to_string());

    let mut bpf = Bpf::load(include_bytes_aligned!(concat!(env!("OUT_DIR"), "/torii"))).unwrap();

    let route_map_data = bpf.take_map("ROUTE_MAP").expect("Missing ROUTE_MAP");
    let route_map: AyaHashMap<_, u32, RouteTargetPod> =
        AyaHashMap::try_from(route_map_data).unwrap();

    let fip_dnat_map_data = bpf.take_map("FIP_DNAT_MAP").expect("Missing FIP_DNAT_MAP");
    let fip_dnat_map: AyaHashMap<_, u32, u32> = AyaHashMap::try_from(fip_dnat_map_data).unwrap();

    let fip_snat_map_data = bpf.take_map("FIP_SNAT_MAP").expect("Missing FIP_SNAT_MAP");
    let fip_snat_map: AyaHashMap<_, u32, u32> = AyaHashMap::try_from(fip_snat_map_data).unwrap();

    // STATIC eBPF ATTACHMENT (Safely skips if interface doesn't exist yet)
    let overlay: &mut Xdp = bpf
        .program_mut("overlay_ingress")
        .unwrap()
        .try_into()
        .unwrap();
    overlay.load().unwrap();
    if get_ifindex(&overlay_iface) > 0 {
        overlay.attach(&overlay_iface, XdpFlags::SKB_MODE).unwrap();
        println!("Attached overlay_ingress to {}", overlay_iface);
    } else {
        println!(
            "Waiting for dynamic TAP creation. Skipping initial overlay attach for {}",
            overlay_iface
        );
    }

    let underlay: &mut Xdp = bpf
        .program_mut("underlay_ingress")
        .unwrap()
        .try_into()
        .unwrap();
    underlay.load().unwrap();
    if get_ifindex(&underlay_iface) > 0 {
        underlay
            .attach(&underlay_iface, XdpFlags::SKB_MODE)
            .unwrap();
        println!("Attached underlay_ingress to {}", underlay_iface);
    } else {
        println!("Warning: Underlay interface {} not found.", underlay_iface);
    }

    let state = GatewayState {
        routes: HashMap::new(),
        floating_ips: HashMap::new(),
        route_map,
        fip_dnat_map,
        fip_snat_map,
        bpf,                   // Retain Bpf context for dynamic API attachments
        xdp_links: Vec::new(), // Initialize storage
    };

    state
}

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct RouteTargetPod(pub RouteTarget);

#[allow(unsafe_attr_outside_unsafe)]
unsafe impl aya::Pod for RouteTargetPod {}
