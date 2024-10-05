import struct
import packet_display_window as window


def display_packet(packet_data):
    window.write_data(packet_data[:180])
    return
    # Assuming packet_data contains raw bytes from a packet
    print(f"Received Packet: {len(packet_data)} bytes")

    # Example to interpret the first few bytes of the packet (IP/TCP/UDP headers):
    if len(packet_data) >= 20:  # Minimum size for an IPv4 header
        ip_header = struct.unpack(
            "!BBHHHBBH4s4s", packet_data[:20]
        )  # Extract IP header details
        src_ip = ".".join(map(str, ip_header[8]))
        dest_ip = ".".join(map(str, ip_header[9]))
        print(f"Source IP: {src_ip}")
        print(f"Destination IP: {dest_ip}")
        # Continue interpreting packet based on your protocol (TCP/UDP)
    else:
        print("Packet too small to interpret headers.")

    # Display raw bytes (if necessary)
    print(packet_data.hex())


"""
ask saba gapeto to create all the structs in packetHeaders.h in python,
and then in display_packet function, parse the bytes and display the packet according to the structs.
add an ioctl end_packet_logging to tell the driver that cli has exited, and make sure then to test everything.
"""
