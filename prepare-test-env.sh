#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (e.g., sudo ./setup-host.sh)"
  exit 1
fi

echo "Cleaning up previous state..."
ip link delete veth-host > /dev/null 2>&1 || true
docker compose down --remove-orphans

# Always rebuild first: starting with stale images silently runs a different
# datapath than the one in this working tree.
echo "Building images..."
docker compose build

echo "Starting Docker containers..."
docker compose up -d gateway-node vmm-gateway

echo "Waiting for gateway-node to initialize..."
while ! docker inspect -f '{{.State.Pid}}' gateway-node > /dev/null 2>&1 || [ "$(docker inspect -f '{{.State.Pid}}' gateway-node)" == "0" ]; do
    sleep 1
done

PID=$(docker inspect -f '{{.State.Pid}}' gateway-node)

echo "Injecting veth-gw into container (PID: $PID)..."
ip link add veth-host type veth peer name veth-gw

# NEW: The host routes to VMs via the 10.0.0.0/24 Subnet only!
ip addr add 10.0.0.1/24 dev veth-host

# Maintain the Gateway IPs as /32 strictly so the host can still answer pings
# to the VM gateway addresses, but prevents the Host OS from reaching
# 192.168.100.0/24 natively. VM ARP requests never arrive here anymore: they are
# answered by the eBPF ARP responder inside the vmm-gateway containers.
ip addr add 192.168.100.1/32 dev veth-host 
ip addr add 193.100.100.1/32 dev veth-host

ip link set veth-host up
ethtool -K veth-host tx off rx off
ip link set veth-gw netns $PID
nsenter -t $PID -n ip link set veth-gw up
nsenter -t $PID -n ethtool -K veth-gw tx off rx off

echo "Enabling IP Forwarding and NAT for VM Internet Access..."
sysctl -w net.ipv4.ip_forward=1 > /dev/null

# Identify the primary interface with the default route out to the internet
DEFAULT_IF=$(ip route show default | awk '/default/ {print $5}' | head -n 1)

if [ -n "$DEFAULT_IF" ]; then
    echo "Configuring iptables MASQUERADE on $DEFAULT_IF..."
    
    # We now masquerade the FIP subnet since eBPF does SNAT to 10.0.0.X
    iptables -t nat -D POSTROUTING -s 10.0.0.0/24 -o "$DEFAULT_IF" -j MASQUERADE 2>/dev/null || true
    iptables -D FORWARD -s 10.0.0.0/24 -j ACCEPT 2>/dev/null || true
    iptables -D FORWARD -d 10.0.0.0/24 -j ACCEPT 2>/dev/null || true
    
    iptables -t nat -A POSTROUTING -s 10.0.0.0/24 -o "$DEFAULT_IF" -j MASQUERADE
    iptables -A FORWARD -s 10.0.0.0/24 -j ACCEPT
    iptables -A FORWARD -d 10.0.0.0/24 -j ACCEPT
else
    echo "WARNING: No default route found on host. NAT will not be configured."
fi

echo "Host setup complete!"
echo "Wait ~30 seconds for both VMs to boot, then you can run:"
echo "ssh ubuntu@10.0.0.2  (VM 1 via FIP NAT)"
echo "ssh ubuntu@10.0.0.3  (VM 2 via FIP NAT, other host)"
echo "ssh ubuntu@10.0.0.4  (VM 4 via FIP NAT, same subnet AND same host as VM 1)"
echo "ssh ubuntu@10.0.0.42 (VM 3 via FIP NAT, second subnet on the host of VM 1)"
