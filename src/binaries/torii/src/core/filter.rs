//! Parsing and translation of the packet filter include-lists.
//!
//! A route filter is kept twice: as the textual rules the control plane hands
//! out and takes back (`RouteFilterRules`), and as the flat `RouteFilter`
//! struct the eBPF datapath reads. This module owns the conversion between the
//! two, including the normalisation that lets a subnet and the equivalent
//! explicit range be recognised as the same entry.

use std::net::Ipv4Addr;

use uuid::Uuid;

use torii_common::{FILTER_MAX_IP_RANGES, FILTER_MAX_PORT_RANGES, IpRange, PortRange, RouteFilter};

use ainari_api_structs::route_structs::*;

use crate::core::models::RouteFilterPod;
use crate::core::state::GatewayState;

/// Renders the canonical text form of an address range.
///
/// The same range can be written in several ways, and the client is free to use
/// any of them. What comes back is always the shortest unambiguous form: a bare
/// address for a single host, CIDR notation whenever the range happens to be an
/// aligned block, and the explicit `first-last` form for everything else.
///
/// # Arguments
/// * `first` - First address of the range
/// * `last` - Last address of the range (inclusive)
///
/// # Returns
/// A `String` holding the canonical notation of the range
fn canonical_ip_spec(first: u32, last: u32) -> String {
    if first == last {
        return Ipv4Addr::from(first).to_string();
    }

    // A range is a subnet exactly when its size is a power of two and its first
    // address is aligned to that size.
    let size = u64::from(last) - u64::from(first) + 1;
    if size.is_power_of_two() && u64::from(first) % size == 0 {
        let prefix = 32 - size.trailing_zeros();
        return format!("{}/{}", Ipv4Addr::from(first), prefix);
    }

    format!("{}-{}", Ipv4Addr::from(first), Ipv4Addr::from(last))
}

/// Parses one entry of the IP include-list.
///
/// Three notations are accepted and all of them end up as an inclusive range:
///
/// * `10.0.0.7` - a single address
/// * `10.0.0.0/24` - a subnet, expanded to its first and last address
/// * `10.0.0.5-10.0.0.9` - an explicit range
///
/// # Arguments
/// * `spec` - The textual entry as it arrived from the client
///
/// # Returns
/// A `Result` with the parsed rule, or a message naming what was wrong with it
pub fn parse_ip_range(spec: &str) -> Result<IpRangeRule, String> {
    let spec = spec.trim();
    if spec.is_empty() {
        return Err("Empty IP range".to_string());
    }

    let (first, last) = if let Some((addr, prefix)) = spec.split_once('/') {
        let addr: Ipv4Addr = addr
            .trim()
            .parse()
            .map_err(|_| format!("Invalid address in '{}'", spec))?;
        let prefix: u32 = prefix
            .trim()
            .parse()
            .map_err(|_| format!("Invalid prefix length in '{}'", spec))?;
        if prefix > 32 {
            return Err(format!("Prefix length out of range in '{}'", spec));
        }
        // A /0 mask cannot be produced by shifting a u32 by 32, so widen first.
        let mask = (!0u64 << (32 - prefix)) as u32;
        let network = u32::from(addr) & mask;
        (network, network | !mask)
    } else if let Some((from, to)) = spec.split_once('-') {
        let from: Ipv4Addr = from
            .trim()
            .parse()
            .map_err(|_| format!("Invalid start address in '{}'", spec))?;
        let to: Ipv4Addr = to
            .trim()
            .parse()
            .map_err(|_| format!("Invalid end address in '{}'", spec))?;
        if u32::from(from) > u32::from(to) {
            return Err(format!("Range '{}' ends before it starts", spec));
        }
        (u32::from(from), u32::from(to))
    } else {
        let addr: Ipv4Addr = spec
            .parse()
            .map_err(|_| format!("Invalid address '{}'", spec))?;
        (u32::from(addr), u32::from(addr))
    };

    Ok(IpRangeRule {
        spec: canonical_ip_spec(first, last),
        first: Ipv4Addr::from(first),
        last: Ipv4Addr::from(last),
    })
}

/// Parses one entry of the port include-list.
///
/// Accepts a single port (`22`) as well as a range (`8000-8100`).
///
/// # Arguments
/// * `spec` - The textual entry as it arrived from the client
///
/// # Returns
/// A `Result` with the parsed rule, or a message naming what was wrong with it
pub fn parse_port_range(spec: &str) -> Result<PortRangeRule, String> {
    let spec = spec.trim();
    if spec.is_empty() {
        return Err("Empty port".to_string());
    }

    let (first, last) = if let Some((from, to)) = spec.split_once('-') {
        let from: u16 = from
            .trim()
            .parse()
            .map_err(|_| format!("Invalid start port in '{}'", spec))?;
        let to: u16 = to
            .trim()
            .parse()
            .map_err(|_| format!("Invalid end port in '{}'", spec))?;
        if from > to {
            return Err(format!("Port range '{}' ends before it starts", spec));
        }
        (from, to)
    } else {
        let port: u16 = spec
            .parse()
            .map_err(|_| format!("Invalid port '{}'", spec))?;
        (port, port)
    };

    if first == 0 {
        return Err(format!("Port 0 is not a usable port in '{}'", spec));
    }

    let canonical = if first == last {
        format!("{}", first)
    } else {
        format!("{}-{}", first, last)
    };

    Ok(PortRangeRule {
        spec: canonical,
        first,
        last,
    })
}

/// Translates the textual rules of a route into the struct the datapath reads.
///
/// # Arguments
/// * `rules` - The include-lists currently attached to the route
///
/// # Returns
/// A `Result` holding the populated `RouteFilter`, or an error when one of the
/// lists exceeds the capacity of the eBPF map value
pub fn build_route_filter(rules: &RouteFilterRules) -> Result<RouteFilter, String> {
    if rules.ip_ranges.len() > FILTER_MAX_IP_RANGES {
        return Err(format!(
            "A route filter holds at most {} IP ranges",
            FILTER_MAX_IP_RANGES
        ));
    }
    if rules.ports.len() > FILTER_MAX_PORT_RANGES {
        return Err(format!(
            "A route filter holds at most {} port ranges",
            FILTER_MAX_PORT_RANGES
        ));
    }

    let mut filter = RouteFilter::empty();
    filter.ip_range_count = rules.ip_ranges.len() as u32;
    filter.port_range_count = rules.ports.len() as u32;

    for (slot, rule) in filter.ip_ranges.iter_mut().zip(rules.ip_ranges.iter()) {
        *slot = IpRange {
            start: u32::from(rule.first),
            end: u32::from(rule.last),
        };
    }
    for (slot, rule) in filter.port_ranges.iter_mut().zip(rules.ports.iter()) {
        *slot = PortRange {
            start: rule.first,
            end: rule.last,
        };
    }

    Ok(filter)
}

/// Resolves a route UUID to its destination address and its eBPF map key.
///
/// Every filter operation addresses a route by its UUID, while both eBPF maps
/// are keyed by the destination address of that route - this is the one place
/// that translation happens.
///
/// # Arguments
/// * `st` - The locked gateway state
/// * `route_uuid` - The UUID of the route, taken from the URL
///
/// # Returns
/// An `Option` with the destination address and the map key of the route, or
/// `None` when no such route exists
pub fn route_filter_key(st: &GatewayState, route_uuid: &Uuid) -> Option<(String, u32)> {
    let route = st.routes.get(route_uuid)?;
    let addr: Ipv4Addr = route.dest_ip.parse().ok()?;
    Some((route.dest_ip.clone(), u32::from(addr)))
}

/// Commits a new set of include-lists for one route.
///
/// The translation into the eBPF representation happens first, so a filter that
/// does not fit into the map value is rejected before anything is changed.
/// A route whose lists are both empty is removed from the filter map entirely:
/// no entry means no restriction, which is exactly what an empty include-list
/// is supposed to express - and it saves the datapath a lookup per packet.
///
/// # Arguments
/// * `st` - The locked gateway state
/// * `route_uuid` - The UUID of the route the filter belongs to
/// * `dest_key` - The eBPF map key of that route
/// * `rules` - The include-lists the route should have from now on
///
/// # Returns
/// A `Result` that is `Ok(())` once both the eBPF map and the bookkeeping have
/// been updated, or an error message with nothing changed
pub fn apply_filter(
    st: &mut GatewayState,
    route_uuid: Uuid,
    dest_key: u32,
    rules: RouteFilterRules,
) -> Result<(), String> {
    if rules.is_empty() {
        let _ = st.filter_map.remove(&dest_key);
        st.filters.remove(&route_uuid);
        return Ok(());
    }

    let filter = build_route_filter(&rules)?;
    st.filter_map
        .insert(dest_key, RouteFilterPod(filter), 0)
        .map_err(|_| "eBPF Map error (filter)".to_string())?;
    st.filters.insert(route_uuid, rules);
    Ok(())
}

/// Builds the answer of a filter operation from the state of the route.
///
/// # Arguments
/// * `st` - The locked gateway state
/// * `route_uuid` - The UUID of the route that was worked on
/// * `dest_ip` - Destination address of that route
/// * `message` - Human readable summary of what the operation did
///
/// # Returns
/// A `FilterResponse` carrying the current include-lists of the route
pub fn build_filter_response(
    st: &GatewayState,
    route_uuid: Uuid,
    dest_ip: String,
    message: String,
) -> FilterResponse {
    FilterResponse {
        success: true,
        message,
        route_uuid,
        dest_ip,
        filter: st.filters.get(&route_uuid).cloned().unwrap_or_default(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Parses a spec and returns its canonical form together with its bounds.
    fn ip(spec: &str) -> (String, u32, u32) {
        let rule = parse_ip_range(spec).expect(spec);
        (rule.spec, u32::from(rule.first), u32::from(rule.last))
    }

    #[test]
    fn single_address_is_its_own_range() {
        assert_eq!(
            ip("10.0.0.7"),
            ("10.0.0.7".to_string(), 0x0a000007, 0x0a000007)
        );
    }

    #[test]
    fn subnet_expands_to_first_and_last_address() {
        assert_eq!(
            ip("10.0.0.0/24"),
            ("10.0.0.0/24".to_string(), 0x0a000000, 0x0a0000ff)
        );
        // Host bits are dropped, exactly like a router would.
        assert_eq!(ip("10.0.0.42/24").1, 0x0a000000);
        assert_eq!(ip("0.0.0.0/0"), ("0.0.0.0/0".to_string(), 0, u32::MAX));
        assert_eq!(
            ip("10.0.0.7/32"),
            ("10.0.0.7".to_string(), 0x0a000007, 0x0a000007)
        );
    }

    #[test]
    fn explicit_range_keeps_its_bounds() {
        assert_eq!(
            ip("10.0.0.5-10.0.0.9"),
            ("10.0.0.5-10.0.0.9".to_string(), 0x0a000005, 0x0a000009)
        );
    }

    #[test]
    fn a_range_that_is_a_subnet_is_reported_as_one() {
        // This is what makes removal independent of the notation used to add.
        assert_eq!(ip("10.0.0.0-10.0.0.255").0, "10.0.0.0/24");
    }

    #[test]
    fn broken_ip_specs_are_rejected() {
        assert!(parse_ip_range("").is_err());
        assert!(parse_ip_range("10.0.0.256").is_err());
        assert!(parse_ip_range("10.0.0.0/33").is_err());
        assert!(parse_ip_range("10.0.0.9-10.0.0.5").is_err());
    }

    #[test]
    fn ports_accept_singles_and_ranges() {
        let single = parse_port_range("22").unwrap();
        assert_eq!(
            (single.spec.as_str(), single.first, single.last),
            ("22", 22, 22)
        );

        let range = parse_port_range("5000-5100").unwrap();
        assert_eq!(
            (range.spec.as_str(), range.first, range.last),
            ("5000-5100", 5000, 5100)
        );
    }

    #[test]
    fn broken_port_specs_are_rejected() {
        assert!(parse_port_range("").is_err());
        assert!(parse_port_range("0").is_err());
        assert!(parse_port_range("65536").is_err());
        assert!(parse_port_range("100-10").is_err());
    }

    #[test]
    fn empty_rules_translate_to_a_filter_that_allows_everything() {
        let filter = build_route_filter(&RouteFilterRules::default()).unwrap();
        assert_eq!(filter.ip_range_count, 0);
        assert_eq!(filter.port_range_count, 0);
    }

    #[test]
    fn rules_are_copied_into_the_ebpf_representation() {
        let rules = RouteFilterRules {
            ip_ranges: vec![parse_ip_range("10.0.0.0/24").unwrap()],
            ports: vec![parse_port_range("5000-5100").unwrap()],
        };
        let filter = build_route_filter(&rules).unwrap();

        assert_eq!(filter.ip_range_count, 1);
        assert_eq!(filter.ip_ranges[0].start, 0x0a000000);
        assert_eq!(filter.ip_ranges[0].end, 0x0a0000ff);
        assert_eq!(filter.port_range_count, 1);
        assert_eq!(filter.port_ranges[0].start, 5000);
        assert_eq!(filter.port_ranges[0].end, 5100);
    }

    #[test]
    fn a_list_longer_than_the_map_value_is_rejected() {
        let rules = RouteFilterRules {
            ip_ranges: (0..=FILTER_MAX_IP_RANGES)
                .map(|i| parse_ip_range(&format!("10.0.0.{}", i)).unwrap())
                .collect(),
            ports: Vec::new(),
        };
        assert!(build_route_filter(&rules).is_err());
    }
}
