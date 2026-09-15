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

use apistos::ApiComponent;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use std::net::Ipv4Addr;
use uuid::Uuid;
use validator::Validate;

/// Data Transfer Object for creating a new route.
///
/// `next_hop_ip` and `next_hop_mac` are only relevant for local routes (empty
/// `gateway_ip`). They describe the link layer next hop the packet has to be
/// addressed to when it leaves `target_iface`. For TAP devices this is resolved
/// automatically from the VM registered with the device, so both fields can stay
/// empty there.
///
/// `encrypted` marks a destination that is reached over an IPsec protected
/// connection. Such a route is not served by the eBPF overlay at all: the packet
/// is handed to the kernel, which applies the ESP transformation of the matching
/// xfrm policy and tunnels it to `gateway_ip` - the underlay address of the
/// remote gateway. Until a key is installed for the VM pair, the fail-closed
/// block policy of the route discards the traffic.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct RouteRequest {
    pub dest_ip: String,
    pub target_iface: String,
    pub gateway_ip: String,
    #[serde(default)]
    pub next_hop_ip: Option<String>,
    #[serde(default)]
    pub next_hop_mac: Option<String>,
    #[serde(default)]
    pub encrypted: bool,
}

/// Data Transfer Object for installing one IPsec key on a VM-to-VM connection.
///
/// A key is one xfrm Security Association and therefore always belongs to a
/// single direction of a single connection. `local_ip` is the VM on this host,
/// `remote_ip` the VM on the other side and `peer_gateway_ip` the underlay
/// address of the gateway hosting it. `spi` identifies the key on the wire: an
/// incoming ESP packet carries it in its header, which is how the receiver picks
/// the right key out of the several it may hold for one connection.
///
/// `key` is the raw AES-256-GCM key material as a hex string: 32 bytes of key
/// followed by the 4 byte salt required by `rfc4106(gcm(aes))`, i.e. 72 hex
/// digits, optionally prefixed with `0x`.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct CryptoKeyRequest {
    pub direction: String,
    pub local_ip: String,
    pub remote_ip: String,
    pub peer_gateway_ip: String,
    pub spi: u32,
    pub key: String,
}

/// Data Transfer Object for switching the encryption of a connection on or off.
///
/// The toggle works on a connection, not on a destination address: several VMs
/// of this host may talk to the same remote VM over the same route, and each of
/// those connections is protected by its own key and can be switched
/// independently.
///
/// Switching a connection off leaves its keys untouched - they stay in the
/// kernel, ready to be used again - but replaces the policies of the connection
/// with plain allow rules, so the traffic is neither encrypted on the way out
/// nor required to arrive encrypted. `peer_gateway_ip` only has to be given for
/// a connection this gateway does not know a key for yet.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct CryptoToggleRequest {
    pub local_ip: String,
    pub remote_ip: String,
    #[serde(default)]
    pub peer_gateway_ip: Option<String>,
    pub enabled: bool,
}

/// Response payload for listing the known connections.
#[derive(Debug, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct ConnectionListResponse {
    pub connections: Vec<Connection>,
}

/// Response payload for listing the installed IPsec keys.
#[derive(Debug, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct CryptoKeyListResponse {
    pub keys: Vec<CryptoKey>,
}

/// Data Transfer Object for assigning a floating IP to an internal IP.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct FloatingIpRequest {
    pub floating_ip: String,
    pub internal_ip: String,
}

/// Data Transfer Object for creating a TAP device.
///
/// TAP devices are deliberately created without any IP address or subnet: the
/// eBPF datapath answers ARP and routes every frame, so the host side of the
/// link needs no address at all. That is what allows several VMs of the same
/// subnet to be hosted on one machine without colliding TAP addresses.
///
/// `vm_mac` and `vm_ip` describe the VM that will be attached to the device.
/// They are used to program the eBPF ARP responder and to address frames that
/// are delivered to the VM.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct TapRequest {
    pub tap_name: String,
    #[serde(default)]
    pub vm_mac: Option<String>,
    #[serde(default)]
    pub vm_ip: Option<String>,
}

/// Data Transfer Object for configuring an existing network interface.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct IfaceConfigRequest {
    pub iface_name: String,
    pub ip_cidr: Option<String>,
    pub up: bool,
}

/// Response payload for route-related API operations.
#[derive(Debug, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct RouteResponse {
    pub success: bool,
    pub message: String,
    pub route: Option<Route>,
}

/// Response payload for listing all active routes.
#[derive(Debug, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct RouteListResponse {
    pub routes: Vec<Route>,
}

/// Response payload for TAP device creation.
#[derive(Debug, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct TapResponse {
    pub success: bool,
    pub message: String,
    pub tap_name: String,
}

/// Data Transfer Object for adding or removing IP ranges of a route filter.
///
/// Every entry may be written as a plain address (`10.0.0.7`), as a subnet in
/// CIDR notation (`10.0.0.0/24`) or as an explicit range (`10.0.0.5-10.0.0.9`).
#[derive(Debug, Deserialize, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct FilterIpRangeRequest {
    pub ranges: Vec<String>,
}

/// Data Transfer Object for adding or removing ports of a route filter.
///
/// Every entry may be written as a single port (`22`) or as a range
/// (`8000-8100`).
#[derive(Debug, Deserialize, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct FilterPortRequest {
    pub ports: Vec<String>,
}

/// Response payload of every packet filter operation.
///
/// It always carries the complete filter of the route afterwards, so a client
/// never has to guess what its change ended up doing.
#[derive(Debug, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct FilterResponse {
    pub success: bool,
    pub message: String,
    pub route_uuid: Uuid,
    pub dest_ip: String,
    pub filter: RouteFilterRules,
}

/// Response payload for listing the packet filters of all routes.
#[derive(Debug, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct FilterListResponse {
    pub filters: Vec<FilterEntry>,
}

/// The two include-lists attached to one route.
///
/// An empty list is not a filter that denies everything but the absence of a
/// restriction: a route without IP ranges carries every address, a route
/// without ports carries every port. A route whose lists are both empty is not
/// programmed into the eBPF filter map at all.
#[derive(Debug, Clone, Default, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct RouteFilterRules {
    pub ip_ranges: Vec<IpRangeRule>,
    pub ports: Vec<PortRangeRule>,
}

impl RouteFilterRules {
    /// Reports whether this route is unfiltered, i.e. both include-lists are empty.
    ///
    /// # Arguments
    /// None
    ///
    /// # Returns
    /// `true` when neither an IP range nor a port has been added yet
    pub fn is_empty(&self) -> bool {
        self.ip_ranges.is_empty() && self.ports.is_empty()
    }
}

/// Internal representation of one protected VM-to-VM connection.
///
/// It ties the two VMs, the gateway behind the remote one and the currently
/// active outbound key together, and remembers whether the encryption of this
/// connection is switched on.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct Connection {
    pub local_ip: String,
    pub remote_ip: String,
    pub peer_gateway_ip: String,
    pub enabled: bool,
    pub active_egress_spi: Option<u32>,
}

/// Internal representation of an installed IPsec key.
///
/// The key material itself is deliberately not kept around - it lives in the
/// kernel's Security Association Database and nowhere else.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct CryptoKey {
    pub direction: String,
    pub local_ip: String,
    pub remote_ip: String,
    pub peer_gateway_ip: String,
    pub spi: u32,
}

/// Internal representation of a network route.
///
/// The `uuid` the gateway assigns on creation is what every later operation on
/// the route - update, delete and the packet filter endpoints - addresses it by.
#[derive(Debug, Deserialize, Serialize, Clone, JsonSchema, ApiComponent, Validate)]
pub struct Route {
    pub uuid: Uuid,
    pub dest_ip: String,
    pub target_iface: String,
    pub gateway_ip: String,
    pub next_hop_ip: Option<String>,
    pub next_hop_mac: Option<String>,
    pub encrypted: bool,
}

/// One entry of the IP include-list of a route.
///
/// `spec` is the canonical text form of the entry - the shape the client gets
/// back and can hand to the remove endpoint again - while `first` and `last`
/// are the inclusive bounds the datapath actually compares against. A subnet
/// and the equivalent explicit range therefore collapse onto the same entry.
#[derive(Debug, Clone, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct IpRangeRule {
    pub spec: String,
    pub first: Ipv4Addr,
    pub last: Ipv4Addr,
}

/// One entry of the port include-list of a route, as an inclusive range.
#[derive(Debug, Clone, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct PortRangeRule {
    pub spec: String,
    pub first: u16,
    pub last: u16,
}

/// One entry of the filter overview.
#[derive(Debug, Serialize, JsonSchema, ApiComponent, Validate)]
pub struct FilterEntry {
    pub route_uuid: Uuid,
    pub dest_ip: String,
    pub filter: RouteFilterRules,
}
