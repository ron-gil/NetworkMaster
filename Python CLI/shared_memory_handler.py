import time
import mmap
import win32file
import win32event
import pywintypes
import ioctl_handler as ioctl
import packet_handler as ph
import ctypes
from ctypes import wintypes

# Load Kernel32 DLL
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

SHARED_MEMORY_SIZE = 4096
SHARED_MEMORY_NAME = "NetworkMasterPacketDump"
PACKET_CAPTURE_EVENT_NAME = "PacketCaptureEvent"

# Define constants
PAGE_READWRITE = 0x04
FILE_MAP_READ = 0x04
EVENT_ALL_ACCESS = 0x1F0003
INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value


def open_shared_memory():
    # Define function argument and return types for OpenFileMappingW
    kernel32.OpenFileMappingW.argtypes = [
        wintypes.DWORD,
        wintypes.BOOL,
        wintypes.LPCWSTR,
    ]
    kernel32.OpenFileMappingW.restype = wintypes.HANDLE

    kernel32.MapViewOfFile.argtypes = [
        wintypes.HANDLE,  # hFileMappingObject
        wintypes.DWORD,  # dwDesiredAccess
        wintypes.DWORD,  # dwFileOffsetHigh
        wintypes.DWORD,  # dwFileOffsetLow
        ctypes.c_size_t,  # dwNumberOfBytesToMap
    ]
    kernel32.MapViewOfFile.restype = wintypes.LPVOID

    # Open the existing shared memory object
    hFileMap = kernel32.OpenFileMappingW(
        FILE_MAP_READ,  # Desired access (read-only in this case)
        False,  # Inherit handle
        SHARED_MEMORY_NAME,  # Name of the shared memory object
    )

    if not hFileMap or hFileMap == INVALID_HANDLE_VALUE:
        print("Failed to create or open shared memory. Error:", ctypes.GetLastError())
        return None

    # Map view of the file to get a pointer to the shared memory
    pBuffer = kernel32.MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0)

    if not pBuffer:
        print("Failed to map view of shared memory. Error:", ctypes.GetLastError())
        kernel32.CloseHandle(hFileMap)
        return None

    return pBuffer


def open_event():
    # Define function argument and return types
    kernel32.OpenEventW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    kernel32.OpenEventW.restype = wintypes.HANDLE

    # Try to open the event created by the driver
    hEvent = kernel32.OpenEventW(EVENT_ALL_ACCESS, False, PACKET_CAPTURE_EVENT_NAME)

    if not hEvent or hEvent == INVALID_HANDLE_VALUE:
        print(f"Failed to open event. Error: {ctypes.get_last_error()}")
        return None

    print("Event opened successfully.")
    return hEvent


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
        packet_data = shared_mem.read(
            SHARED_MEMORY_SIZE
        )  # Adjust based on max packet size
        return packet_data
    except Exception as e:
        print(f"Error reading packet: {e}")
        return None


def keep_connection_alive(handle):
    while True:
        try:
            # Call the IOCTL to keep the connection alive
            ioctl.send_ioctl(handle, ioctl.IOCTL_INIT_PACKET_LOGGING)
            print("Sent IOCTL to init packet logging.")
        except Exception as e:
            print(f"Error in keep_connection_alive: {e}")

        time.sleep(15 * 60)  # Wait 15 minutes
