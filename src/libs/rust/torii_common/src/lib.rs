#![no_std]

#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub struct RouteTarget {
    pub action: u32,       // 0 = Local Interface, 1 = Encapsulate & Send
    pub ifindex: u32,      // Local interface index to redirect out of (e.g., eth0)
    pub encap_dst_ip: u32, // Target Gateway Underlay IP
    pub encap_dst_mac: [u8; 6],
    pub _pad1: [u8; 2],
    pub encap_src_ip: u32, // Our Gateway Underlay IP
    pub encap_src_mac: [u8; 6],
    pub _pad2: [u8; 2],
}
