import time
import mmap
import win32file
import win32event
import pywintypes
import ioctl_handler as ioctl
import packet_handler as ph

SHARED_MEMORY_SIZE = 4096
SHARED_MEMORY_NAME = "NetworkMasterPacketDump"
PACKET_CAPTURE_EVENT_NAME = "PacketCaptureEvent"

def open_shared_memory():
    try:
        # Open a handle to the named shared memory object
        hFileMap = win32file.CreateFileMapping(
            win32file.INVALID_HANDLE_VALUE,
            None,
            win32file.PAGE_READWRITE,
            0,
            SHARED_MEMORY_SIZE,
            SHARED_MEMORY_NAME,
        )

        # Map the file into the memory space
        shared_mem = mmap.mmap(hFileMap, 0, access=mmap.ACCESS_READ)

        print("Shared memory opened successfully.")
        return shared_mem
    except pywintypes.error as e:
        print(f"Error opening shared memory: {e}")
        return None


def open_event():
    try:
        # Open the event created by the driver
        event_handle = win32event.OpenEvent(
            win32event.EVENT_ALL_ACCESS, False, f"Global\\{PACKET_CAPTURE_EVENT_NAME}"
        )
        print("Event opened successfully.")
        return event_handle
    except pywintypes.error as e:
        print(f"Error opening event: {e}")
        return None


def wait_for_packet(event_handle):
    if event_handle is None:
        print("Invalid event handle.")
        return

    while True:
        # Wait for the event to be signaled
        result = win32event.WaitForSingleObject(event_handle, win32event.INFINITE)
        if result == win32event.WAIT_OBJECT_0:
            print("Event signaled, packet ready to be read from shared memory.")
            # Call your function to read from shared memory
            packet_data = read_received_packet()
            ph.display_packet(packet_data)
        else:
            print(f"Unexpected result from event wait: {result}")


def read_received_packet(shared_mem):
    try:
        # Read packet data from shared memory
        shared_mem.seek(0)  # Move to the start
        packet_data = shared_mem.read(SHARED_MEMORY_SIZE)  # Adjust based on max packet size
        return packet_data
    except Exception as e:
        print(f"Error reading packet: {e}")
        return None


def keep_connection_alive():
    while True:
        try:
            # Call the IOCTL to keep the connection alive
            ioctl.send_ioctl(ioctl.IOCTL_INIT_PACKET_LOGGING)
            print("Sent IOCTL to init packet logging.")
        except Exception as e:
            print(f"Error in keep_connection_alive: {e}")

        time.sleep(15 * 60)  # Wait 15 minutes
