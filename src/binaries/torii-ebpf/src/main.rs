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

#![no_std]
#![no_main]

mod decap;
mod encap;
mod headers;
mod maps;
mod nat;
mod utils; // Add the new module

use aya_ebpf::{bindings::xdp_action, macros::xdp, programs::XdpContext};
use decap::{is_tunnel_packet, process_tunnel_packet};
use encap::encap_and_redirect;
use maps::lookup_route;
use nat::apply_dnat;
use network_types::eth::EthHdr;
use utils::ptr_at;

/// Processes incoming packets on overlay network interfaces.
///
/// This XDP program evaluates traffic entering through virtual overlay interfaces (like TAP
/// devices). It translates destination IP addresses via DNAT if required, checks the central
/// `ROUTE_MAP`, and either redirects the packet locally or encapsulates it in a UDP tunnel
/// to transit the underlay network.
///
/// # Arguments
/// * `ctx` - The eBPF XDP Context containing raw packet data and metadata
///
/// # Returns
/// An eBPF `xdp_action` determining whether to redirect, pass, or drop the packet
#[xdp]
pub fn overlay_ingress(ctx: XdpContext) -> u32 {
    let ethhdr = match ptr_at::<EthHdr>(&ctx, 0) {
        Ok(hdr) => hdr,
        Err(_) => return xdp_action::XDP_PASS,
    };

    let eth_type = unsafe { core::ptr::read_unaligned(ethhdr).ether_type };

    // Apply DNAT if necessary. Returns the true Target IP (or original IP if no DNAT).
    if let Some(dest_ip) = apply_dnat(&ctx, eth_type) {
        if let Some(target) = lookup_route(dest_ip) {
            if target.action == 1 {
                return encap_and_redirect(&ctx, &target);
            } else {
                return unsafe { aya_ebpf::helpers::bpf_redirect(target.ifindex, 0) } as u32;
            }
        }
    }

    xdp_action::XDP_PASS
}

/// Processes incoming packets on the physical underlay interface.
///
/// This XDP program attaches to the primary host interface (e.g., `eth0`). Its
/// role is to identify encapsulated traffic (UDP port 5555) destined for our virtual
/// network, unwrap it, and route the internal payload to the appropriate TAP interface.
/// Normal traffic is passed through unaffected.
///
/// # Arguments
/// * `ctx` - The eBPF XDP Context containing raw packet data and metadata
///
/// # Returns
/// An eBPF `xdp_action` code specifying the next step for the packet
#[xdp]
pub fn underlay_ingress(ctx: XdpContext) -> u32 {
    // If this packet is encapsulated VM traffic, process and route it locally
    if is_tunnel_packet(&ctx) {
        return process_tunnel_packet(&ctx);
    }

    // Otherwise, let standard host networking handle it
    xdp_action::XDP_PASS
}

/// Fallback handler invoked when the eBPF program encounters an unrecoverable error.
///
/// eBPF environments do not support standard panic unwinding. This function
/// guarantees that the program safely aborts without crashing the kernel execution context.
///
/// # Arguments
/// * `_info` - Information about the panic context
///
/// # Returns
/// This function diverges and never returns
#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    unsafe { core::hint::unreachable_unchecked() }
}
