# Multicast UDP Tools

A pair of simple command-line utilities for sending and receiving binary data over UDP multicast.

## Author

Edson Pereira, PY2SDR

## Overview

This package contains two programs:

- **msend** - Multicast sender that reads binary data from stdin and transmits it via UDP multicast
- **mrecv** - Multicast receiver that listens for UDP multicast packets and writes them to stdout

These tools are useful for streaming binary data (such as audio samples, sensor data, or SDR I/Q data) across a network to multiple receivers simultaneously. Receivers can be added and removed transparently, enabling scenarios like live spectrum monitoring, instrumentation, data distribution, and parallel signal processing and demodulators.

## Building

Compile both programs using gcc:

```bash
gcc -o msend msend.c
gcc -o mrecv mrecv.c
```

## Installation

```bash
sudo cp msend mrecv /usr/local/bin
```

## Usage

### Multicast Sender (msend)

```bash
msend -a <multicast_group> -p <port> -i <interface> [-t <ttl>]
```

**Required arguments:**
- `-a <multicast_group>` - Multicast IP address (e.g., 239.0.0.11)
- `-p <port>` - UDP port number (e.g., 15004)
- `-i <interface>` - Network interface name (e.g., dummy0, eth0)

**Optional arguments:**
- `-t <ttl>` - Time-to-live (default: 1, range: 1-255)

**Examples:**

```bash
# Send binary file to multicast group
msend -a 239.0.0.11 -p 15004 -i dummy0 < audio_samples.bin

# Send with TTL=2 (reaches devices 2 hops away)
msend -a 239.0.0.11 -p 15004 -i dummy0 -t 2 < data.bin

# Pipe data from another program
another_program | msend -a 239.0.0.11 -p 15004 -i dummy0
```

### Multicast Receiver (mrecv)

```bash
mrecv -a <multicast_group> -p <port> -i <interface>
```

**Required arguments:**
- `-a <multicast_group>` - Multicast IP address (must match sender)
- `-p <port>` - UDP port number (must match sender)
- `-i <interface>` - Network interface name (e.g., dummy0, eth0)

**Examples:**

```bash
# Receive and save to file
mrecv -a 239.0.0.11 -p 15004 -i dummy0 > received_data.bin

# Receive and pipe to another program
mrecv -a 239.0.0.11 -p 15004 -i dummy0 | another_program
```

## Technical Details

### Data Format

- Both programs handle **binary data** (IQ, audio, etc)
- The sender reads data in chunks of 512 int16_t values (1024 bytes)
- Data is transmitted exactly as received with no encoding or transformation

### Multicast Groups

- Uses IPv4 multicast addresses in the range 224.0.0.0 to 239.255.255.255
- The example address 239.0.0.11 is in the organization-local scope (239.0.0.0/8)

## Example Use Cases

### RTL-SDR

```bash
# Transmitter side - send I/Q samples
rtl_sdr -f 145M -s 2048000 -g 40 - | msend -a 239.0.0.11 -p 15004 -i dummy0

# Receiver side - receive and process
mrecv -a 239.0.0.11 -p 15004 -i dummy0 | csdr convert_u8_f | ...
```

### QSD-SDR

```bash
# Sender
arecord -f S16_LE -r 96000 -c 2 | msend -a 239.0.0.11 -p 15004 -i dummy0

# Receiver
mrecv -a 239.0.0.11 -p 15004 -i dummy0 | csdr convert_u16_f | ... | aplay -f S16_LE -r 48000 -c 2
```

### Data Distribution

```bash
# One sender
data_generator | msend -a 239.0.0.11 -p 15004 -i dummy0

# Multiple receivers (can run simultaneously)
mrecv -a 239.0.0.11 -p 15004 -i dummy0 > /dev/null  # Receiver 1
mrecv -a 239.0.0.11 -p 15004 -i dummy0 > output.bin # Receiver 2
mrecv -a 239.0.0.11 -p 15004 -i dummy0 | analyzer   # Receiver 3
```

## Local Dummy Network Setup

To prevent multicast data from being transmitted over WiFi or other physical networks, you can set up a private dummy network interface on your local machine. By specifying the dummy interface with the `-i` flag, all multicast traffic stays isolated to your system.

On Linux, create a dummy network interface:

```bash
ip link add dummy0 type dummy
ip addr add 192.168.10.1/24 dev dummy0
ip link set dummy0 multicast on
ip link set dummy0 up
```

Since both msend and mrecv explicitly specify the interface via the `-i` flag, no additional routing configuration is needed. This is particularly useful for:

- Testing multicast applications locally
- Preventing multicast traffic on WiFi networks (WiFi has issues with IP Multicast)
- Running sender and receiver on the same machine
- Development and debugging without network interference

### Verifying the Setup

Verify no multicast leaks to your physical interfaces using tcpdump:

```bash
# Monitor WiFi interface (replace wlan0 by your WiFi interface) - should see NO multicast traffic
sudo tcpdump -i wlan0 multicast

# Monitor dummy interface - should see your multicast packets
sudo tcpdump -i dummy0 multicast
```

### Cleanup

To remove the dummy interface when done:

```bash
ip link delete dummy0
```

**Note:** These commands require root privileges. The dummy interface configuration will not persist across reboots unless added to your network configuration files. On my system, I have placed the command in /etc/rc.local and enabled rc.local in systemd.

## Troubleshooting

### No data received

1. Check that sender and receiver use the same multicast group, port, and interface
2. Verify firewall settings allow UDP traffic on the specified port
3. Ensure multicast routing is enabled on your network
4. Try increasing TTL if devices are on different subnets

## Limitations

- UDP provides no guarantee of delivery or ordering
- No built-in error correction or retransmission
- Receiver must be running before sender starts (or it will miss initial packets)
- No encryption or authentication

## Platform Support

These programs should compile and run on:

- Linux
- macOS
- BSD variants
- Other POSIX-compliant systems

Note: `SO_REUSEPORT` may not be available on older systems.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3).
