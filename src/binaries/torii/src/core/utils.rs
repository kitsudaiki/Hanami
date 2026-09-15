//! Small helpers around the network configuration of the host.
//!
//! These wrap the pieces of system state the gateway has to read or change -
//! interface indices, MAC addresses, neighbour resolution and the `ip` command
//! itself - so the endpoints and the routing logic stay readable.

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
pub fn get_ifindex(name: &str) -> u32 {
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
pub fn get_mac_address(iface: &str) -> [u8; 6] {
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
pub fn get_local_ip(iface: &str) -> Option<String> {
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
pub fn get_arp_mac(ip: &str) -> [u8; 6] {
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

/// Runs an `ip` command and turns a non-zero exit status into an error.
///
/// The gateway drives the kernel's routing, neighbour and xfrm tables through
/// iproute2 instead of talking netlink directly, which keeps the individual
/// operations readable and easy to reproduce by hand when debugging.
///
/// # Arguments
/// * `args` - The argument vector handed to the `ip` binary
///
/// # Returns
/// `Ok(())` when the command succeeded, otherwise the captured stderr
pub fn run_ip(args: &[&str]) -> Result<(), String> {
    let output = std::process::Command::new("ip")
        .args(args)
        .output()
        .map_err(|e| format!("failed to run ip {:?}: {}", args, e))?;

    if output.status.success() {
        return Ok(());
    }
    Err(format!(
        "ip {} failed: {}",
        args.join(" "),
        String::from_utf8_lossy(&output.stderr).trim()
    ))
}

/// Enables IPv4 forwarding for the given sysctl path, ignoring missing knobs.
///
/// Traffic that is protected by IPsec leaves the eBPF datapath and is routed by
/// the kernel, which only happens when forwarding is switched on.
///
/// # Arguments
/// * `path` - The sysctl file below `/proc/sys` to write a `1` into
///
/// # Returns
/// None. Failures are ignored on purpose - the knob simply may not exist.
pub fn enable_forwarding(path: &str) {
    let _ = std::fs::write(path, b"1");
}

/// Parses a textual MAC address into its raw 6-byte representation.
///
/// # Arguments
/// * `mac` - A MAC address in the usual colon separated hex notation
///
/// # Returns
/// An `Option<[u8; 6]>` holding the parsed address, or None if malformed
pub fn parse_mac(mac: &str) -> Option<[u8; 6]> {
    let mut out = [0u8; 6];
    let mut seen = 0;
    for (i, part) in mac.trim().split(':').enumerate() {
        if i >= 6 {
            return None;
        }
        out[i] = u8::from_str_radix(part, 16).ok()?;
        seen += 1;
    }
    if seen != 6 {
        return None;
    }
    Some(out)
}
