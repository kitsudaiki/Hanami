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

use aya_ebpf::{bindings::xdp_action, programs::XdpContext};
use network_types::{
    eth::{EthHdr, EtherType},
    ip::IpProto,
};

use crate::{
    headers::{Ipv4Hdr, UdpHdr},
    maps::lookup_route,
    nat::apply_snat,
    utils::ptr_at,
};

/// Evaluates whether an incoming packet is a UDP tunnel packet.
///
/// Parses the Ethernet, IPv4, and UDP headers to determine if the packet matches
/// our overlay network criteria, specifically looking for our target destination port (5555).
///
/// # Arguments
/// * `ctx` - The XDP context containing packet data pointers
///
/// # Returns
/// A boolean indicating `true` if it's a tunnel packet, `false` otherwise
#[inline(always)]
pub fn is_tunnel_packet(ctx: &XdpContext) -> bool {
    let ethhdr = match ptr_at::<EthHdr>(ctx, 0) {
        Ok(hdr) => hdr,
        Err(_) => return false,
    };

    if unsafe { core::ptr::read_unaligned(ethhdr).ether_type } != EtherType::Ipv4 {
        return false;
    }

    let ipv4hdr = match ptr_at::<Ipv4Hdr>(ctx, EthHdr::LEN) {
        Ok(hdr) => hdr,
        Err(_) => return false,
    };

    if unsafe { core::ptr::read_unaligned(ipv4hdr).protocol } != IpProto::Udp as u8 {
        return false;
    }

    let udphdr = match ptr_at::<UdpHdr>(ctx, EthHdr::LEN + Ipv4Hdr::LEN) {
        Ok(hdr) => hdr,
        Err(_) => return false,
    };

    // FIX: Read into a variable first to avoid parser ambiguity
    let dest_port = unsafe { core::ptr::read_unaligned(udphdr).dest };
    dest_port == u16::to_be(5555)
}

/// Decapsulates a tunnel packet and routes its inner payload.
///
/// This function strips away the outer UDP/IPv4/Eth headers (42 bytes total), applies
/// SNAT translations to the inner payload if mapping exists, and finally queries the
/// eBPF routing map to redirect the packet to the correct local TAP interface.
///
/// # Arguments
/// * `ctx` - The XDP context containing the raw network packet
///
/// # Returns
/// An eBPF `xdp_action` code indicating the fate of the packet (e.g., `XDP_REDIRECT`, `XDP_DROP`)
#[inline(always)]
pub fn process_tunnel_packet(ctx: &XdpContext) -> u32 {
    // Strip Outer Eth, IPv4, and UDP headers (14 + 20 + 8 = 42 bytes)
    if unsafe { aya_ebpf::helpers::bpf_xdp_adjust_head(ctx.ctx, 42) } != 0 {
        return xdp_action::XDP_DROP;
    }

    // Parse the decapsulated Inner Ethernet frame
    let inner_eth = match ptr_at::<EthHdr>(ctx, 0) {
        Ok(hdr) => hdr,
        Err(_) => return xdp_action::XDP_DROP,
    };

    let eth_type = unsafe { core::ptr::read_unaligned(inner_eth).ether_type };

    // Apply SNAT if necessary. Returns the true Target IP
    if let Some(dest_ip) = apply_snat(ctx, eth_type) {
        if let Some(target) = lookup_route(dest_ip) {
            if target.action == 0 {
                return unsafe { aya_ebpf::helpers::bpf_redirect(target.ifindex, 0) } as u32;
            }
        }
    }

    // Drop invalid tunnel packets
    xdp_action::XDP_DROP
}
