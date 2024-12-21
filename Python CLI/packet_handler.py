import struct

# import packet_display_window as window
from dataclasses import dataclass
import socket


@dataclass
class EthernetHeader:
    dst_mac: str
    src_mac: str
    ether_type: int


@dataclass
class ARPHeader:
    hw_type: int
    proto_type: int
    hw_size: int
    proto_size: int
    op_code: int
    src_mac: str
    src_ip: str
    dst_mac: str
    dst_ip: str


@dataclass
class IPHeader:
    version: int
    ihl: int  # header length in bytes
    dscp: int
    total_length: int
    identification: int
    flags: int
    fragment_offset: int
    ttl: int
    protocol: int
    header_checksum: int
    src_ip: str
    dst_ip: str


@dataclass
class TCPHeader:
    src_port: int
    dst_port: int
    seq_num: int
    ack_num: int
    data_offset: int
    flags: dict
    window_size: int
    checksum: int
    urgent_pointer: int
    options: bytes


@dataclass
class UDPHeader:
    src_port: int
    dst_port: int
    length: int
    checksum: int


# Function to parse the Ethernet header
def parse_ethernet_header(data):
    # Unpack the Ethernet header
    dst_mac, src_mac, ether_type = struct.unpack("!6s6sH", data[:14])

    # Convert MAC addresses from bytes to the standard string format (XX:XX:XX:XX:XX:XX)
    dst_mac = ":".join(f"{byte:02x}" for byte in dst_mac)
    src_mac = ":".join(f"{byte:02x}" for byte in src_mac)

    # Return as a dataclass instance
    return EthernetHeader(dst_mac=dst_mac, src_mac=src_mac, ether_type=ether_type)


def parse_arp_header(data):
    # Unpack the ARP header
    (
        hw_type,
        proto_type,
        hw_size,
        proto_size,
        op_code,
        src_mac,
        src_ip,
        dst_mac,
        dst_ip,
    ) = struct.unpack("!HHBBH6s4s6s4s", data[:28])

    # Convert MAC addresses from bytes to readable format
    src_mac = ":".join(format(x, "02x") for x in src_mac)
    dst_mac = ":".join(format(x, "02x") for x in dst_mac)

    # Convert IPs from binary to dotted decimal
    src_ip = socket.inet_ntoa(src_ip)
    dst_ip = socket.inet_ntoa(dst_ip)

    # Return as a dataclass instance
    return ARPHeader(
        hw_type=hw_type,
        proto_type=proto_type,
        hw_size=hw_size,
        proto_size=proto_size,
        op_code=op_code,
        src_mac=src_mac,
        src_ip=src_ip,
        dst_mac=dst_mac,
        dst_ip=dst_ip,
    )


def parse_ip_header(data):
    # Unpack the raw IP header
    (
        version_ihl,
        dscp_ecn,
        total_length,
        identification,
        flags_fragment_offset,
        ttl,
        protocol,
        header_checksum,
        src_ip,
        dst_ip,
    ) = struct.unpack("!BBHHHBBH4s4s", data[:20])

    # Process and separate fields
    version = version_ihl >> 4
    ihl = version_ihl & 0x0F
    dscp = dscp_ecn >> 2
    ecn = dscp_ecn & 0x03
    flags = flags_fragment_offset >> 13
    fragment_offset = flags_fragment_offset & 0x1FFF

    # Convert IPs from binary to dotted decimal
    src_ip = socket.inet_ntoa(src_ip)
    dst_ip = socket.inet_ntoa(dst_ip)

    # Return as a dataclass instance
    return IPHeader(
        version=version,
        ihl=ihl * 4,  # Convert IHL from 32-bit words to bytes
        dscp=dscp,
        total_length=total_length,
        identification=identification,
        flags=flags,
        fragment_offset=fragment_offset,
        ttl=ttl,
        protocol=protocol,
        header_checksum=header_checksum,
        src_ip=src_ip,
        dst_ip=dst_ip,
    )


def parse_tcp_header(data):
    # Unpack the TCP header
    (
        src_port,
        dst_port,
        seq_num,
        ack_num,
        data_offset_flags,
        window_size,
        checksum,
        urgent_pointer,
    ) = struct.unpack("!HHLLHHHH", data[:20])

    # Extract the data offset (which tells us the size of the TCP header in 32-bit words)
    data_offset = data_offset_flags >> 12
    flags = data_offset_flags & 0xFFF

    # TCP flags
    tcp_flags = {
        "FIN": bool(flags & 0x01),
        "SYN": bool(flags & 0x02),
        "RST": bool(flags & 0x04),
        "PSH": bool(flags & 0x08),
        "ACK": bool(flags & 0x10),
        "URG": bool(flags & 0x20),
    }

    # If the TCP header has options, they come after the basic header (20 bytes)
    options = data[20 : data_offset * 4] if data_offset > 5 else b""

    # Return as a dataclass instance
    return TCPHeader(
        src_port=src_port,
        dst_port=dst_port,
        seq_num=seq_num,
        ack_num=ack_num,
        data_offset=data_offset * 4,  # Convert from 32-bit words to bytes
        flags=tcp_flags,
        window_size=window_size,
        checksum=checksum,
        urgent_pointer=urgent_pointer,
        options=options,
    )


def parse_udp_header(data):
    # Unpack the UDP header
    src_port, dst_port, length, checksum = struct.unpack("!HHHH", data[:8])

    # Return as a dataclass instance
    return UDPHeader(
        src_port=src_port, dst_port=dst_port, length=length, checksum=checksum
    )


# Main function to parse the packet
def parse_packet(data):
    ether_type = None
    # Check if the packet starts with an Ethernet header (Ethertype)
    if len(data) > 14 and struct.unpack("!H", data[12:14])[0] in {
        0x0800,
        0x0806,
        0x86DD,
    }:  # Check if it's Ethernet frame
        ethernet_header = parse_ethernet_header(data)
        print(f"Ethernet Header:\n{ethernet_header}\n")
        ether_type = ethernet_header.ether_type
        data = data[14:]  # Remove Ethernet header before processing further

    else:
        # If no Ethernet header, print an empty one
        print(
            "Ethernet Header:\nEthernet header is absent (packet starts with IPv4 or ARP)\n"
        )

    # Check if the Ethertype indicates IPv4 (0x0800)
    if ether_type == 0x0800 or (data[0] >> 4) == 4:
        # Parse the IP header
        ip_header = parse_ip_header(data)
        print(f"IP Header:\n{ip_header}\n")
        data = data[ip_header.ihl :]

        # Parse the protocol (TCP/UDP) based on the IP protocol field
        if ip_header.protocol == 6:  # TCP protocol number
            tcp_header = parse_tcp_header(data)
            print(f"TCP Header:\n{tcp_header}\n")
        elif ip_header.protocol == 17:  # UDP protocol number
            udp_header = parse_udp_header(data)
            print(f"UDP Header:\n{udp_header}\n")

    # Check if the Ethertype indicates ARP (0x0806)
    elif ether_type == 0x0806 or struct.unpack("!H", data[2:4])[0] == 0x0800:
        # Parse the ARP header
        arp_header = parse_arp_header(data)
        print(f"ARP Header:\n{arp_header}\n")
    else:
        raise Exception("no known header encountered!")


def display_packet(packet_data):
    try:
        parse_packet(packet_data)
    except Exception as e:
        print("Exception in displaying the packet:", e)
        print(f"packet_data:\n{packet_data}")

    # window.write_data(packet_data[:540])


if __name__ == "__main__":
    packet_data = b"\x00\x01\x08\x00\x06\x04\x00\x02RT\x00\x125\x02\n\x00\x02\x02\x08\x00'<(\xa9\n\x00\x02\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03com\x00\x00\x1c\x00\x01\xc0\x0c\x00\x06\x00\x01\x00\x00\x03\x84\x00K\x0bns-cloud-c1\rgoogledomains\xc0\x12\x11awsdns-hostmaster\x06amazon\xc0\x12\x00\x00\x00\x01\x00\x00\x1c \x00\x00\x03\x84\x00\x12u\x00\x00\x01Q\x80\x00\x00"
    display_packet(packet_data)
