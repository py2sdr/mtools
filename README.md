# Multicast UDP Tools

A pair of simple command-line utilities for sending and receiving binary data over UDP multicast.

## Author

Edson Pereira, PY2SDR

## Overview

This package contains two programs:

- **msend** - Multicast sender that reads binary data from stdin and transmits it via UDP multicast
- **mrecv** - Multicast receiver that listens for UDP multicast packets and writes them to stdout

These tools are useful for streaming binary data (such as audio samples, sensor data, or SDR I/Q data) across a network to multiple receivers simultaneously. Receivers can be added and removed transparently, enabling scenarios like live spactrum monitoring, data distribution, and collaborative signal processing.

## Building

Compile both programs using gcc:

```bash
gcc -o msend msend.c
gcc -o mrecv mrecv.c
```

## Installation

Copy the compiled binaries to your system path:

```bash
sudo cp msend mrecv /usr/local/bin
```

## Usage

### Multicast Sender (msend)

```bash
msend <multicast_group> <port> [ttl]
```

**Parameters:**
- `multicast_group` - Multicast IP address (e.g., 239.0.0.11)
- `port` - UDP port number (e.g., 15004)
- `ttl` - Time-to-live (optional, default: 1, range: 1-255)

**Examples:**

```bash
# Send binary file to multicast group
./msend 239.0.0.11 15004 < audio_samples.bin

# Send with TTL=2 (reaches devices 2 hops away)
./msend 239.0.0.11 15004 2 < data.bin

# Pipe data from another program
./generate_data | ./msend 239.0.0.11 15004
```

### Multicast Receiver (mrecv)

```bash
mrecv <multicast_group> <port>
```

**Parameters:**
- `multicast_group` - Multicast IP address (must match sender)
- `port` - UDP port number (must match sender)

**Examples:**

```bash
# Receive and save to file
./mrecv 239.0.0.11 15004 > received_data.bin

# Receive and pipe to another program
./mrecv 239.0.0.11 15004 | ./process_data

# View binary data (use with caution on text terminals)
./mrecv 239.0.0.11 15004 | hexdump -C
```

## Technical Details

### Data Format

- Both programs handle **binary data** (IQ, audio, etc)
- The sender reads data in chunks of 512 int16_t values (1024 bytes)
- Maximum packet size is 2048 bytes
- Data is transmitted exactly as received with no encoding or transformation

### Multicast Groups

- Uses IPv4 multicast addresses in the range 224.0.0.0 to 239.255.255.255
- The example address 239.0.0.11 is in the organization-local scope (239.0.0.0/8)
- Common multicast scopes:
  - 224.0.0.0/24 - Local network control
  - 239.0.0.0/8 - Organization-local scope

## Example Use Cases

### SDR/Radio Applications

```bash
# Transmitter side - send I/Q samples
rtl_sdr -f 100M -s 2048000 -g 40 - | ./msend 239.0.0.11 15004

# Receiver side - receive and process
./mrecv 239.0.0.11 15004 | csdr convert_u8_f | ...
```

### Audio Streaming

```bash
# Sender
arecord -f S16_LE -r 48000 -c 2 | ./msend 239.0.0.11 15004

# Receiver
./mrecv 239.0.0.11 15004 | aplay -f S16_LE -r 48000 -c 2
```

### Data Distribution

```bash
# One sender
./data_generator | ./msend 239.0.0.11 15004

# Multiple receivers (can run simultaneously)
./mrecv 239.0.0.11 15004 > /dev/null  # Receiver 1
./mrecv 239.0.0.11 15004 > output.bin # Receiver 2
./mrecv 239.0.0.11 15004 | ./analyze  # Receiver 3
```

## Troubleshooting

### No data received

1. Check that sender and receiver use the same multicast group and port
2. Verify firewall settings allow UDP traffic on the specified port
3. Ensure multicast routing is enabled on your network
4. Try increasing TTL if devices are on different subnets

### Permission errors

Some systems require root privileges for multicast operations:

```bash
sudo ./mrecv 239.0.0.11 15004
```

### Network interface selection

If your system has multiple network interfaces, you may need to specify which interface to use for multicast. This can be done by modifying the code to set `imr_interface` to a specific interface address instead of `INADDR_ANY`.

## Local Dummy Network Setup

For testing or to prevent multicast data from being transmitted over WiFi networks, you can set up a private dummy network interface on your local machine. This keeps all multicast traffic isolated to your system.

On Linux, create a dummy network interface:

```bash
ip link add dummy0 type dummy
ip addr add 192.168.10.1/24 dev dummy0
ip link set dummy0 multicast on
ip link set dummy0 up
ip route add 239.0.0.0/8 dev dummy0
```

Once configured, multicast traffic will use the dummy interface instead of your physical network interfaces. This is particularly useful for:

- Testing multicast applications locally
- Preventing multicast traffic on shared WiFi networks
- Running sender and receiver on the same machine
- Development and debugging

To remove the dummy interface when done:

```bash
ip link delete dummy0
```

**Note:** These commands require root privileges. The dummy interface configuration will not persist across reboots unless added to your network configuration files.

## Limitations

- Maximum packet size: 2048 bytes
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
