//! Data structures of the gateway API and of its internal bookkeeping.
//!
//! Everything in here is plain data: the payloads that go in and out of the HTTP
//! endpoints, the internal representations the gateway keeps for routes, TAP
//! devices and IPsec connections, plus the wrappers that make the eBPF map
//! values usable with Aya.

use std::net::Ipv4Addr;

use torii_common::{ArpProxy, RouteFilter, RouteTarget};
use uuid::Uuid;

/// Wrapper for passing RouteTarget to Aya eBPF maps safely.
#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct RouteTargetPod(pub RouteTarget);

unsafe impl aya::Pod for RouteTargetPod {}

/// Wrapper for passing ArpProxy to Aya eBPF maps safely.
#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct ArpProxyPod(pub ArpProxy);

unsafe impl aya::Pod for ArpProxyPod {}

/// Wrapper for passing RouteFilter to Aya eBPF maps safely.
#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct RouteFilterPod(pub RouteFilter);

unsafe impl aya::Pod for RouteFilterPod {}

/// Bookkeeping for a TAP device managed by this gateway.
///
/// The gateway keeps the link layer details of every TAP around so that routes
/// pointing at the device can be programmed with the MAC of the VM behind it
/// without the control plane having to repeat that information per route. The
/// address of the VM is not kept here: it goes straight into the eBPF ARP
/// responder, the host route and the neighbour entry of the device.
#[derive(Debug, Clone)]
pub struct TapInfo {
    pub tap_mac: [u8; 6],
    pub vm_mac: Option<[u8; 6]>,
}
