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

use aya_ebpf::{macros::map, maps::HashMap};
use torii_common::RouteTarget;

#[map]
pub static ROUTE_MAP: HashMap<u32, RouteTarget> = HashMap::with_max_entries(1024, 0);

#[map]
pub static FIP_DNAT_MAP: HashMap<u32, u32> = HashMap::with_max_entries(1024, 0);

#[map]
pub static FIP_SNAT_MAP: HashMap<u32, u32> = HashMap::with_max_entries(1024, 0);

/// Queries the routing map for a target IP address.
///
/// This function attempts to find an exact match for the given IP address in the
/// `ROUTE_MAP`. If a specific route does not exist, it falls back to the default
/// route (0.0.0.0).
///
/// # Arguments
/// * `ip` - The destination IPv4 address represented as a `u32`
///
/// # Returns
/// An `Option<RouteTarget>` containing the routing instruction, or `None` if no route matches.
#[inline(always)]
pub fn lookup_route(ip: u32) -> Option<RouteTarget> {
    if let Some(target) = unsafe { ROUTE_MAP.get(&ip) } {
        return Some(*target);
    }
    // Fallback to default route (0.0.0.0)
    if let Some(target) = unsafe { ROUTE_MAP.get(&0) } {
        return Some(*target);
    }
    None
}
