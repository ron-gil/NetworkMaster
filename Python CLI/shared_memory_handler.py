import time
import mmap
import win32file
import win32event
import pywintypes
import ioctl_handler as ioctl
import packet_handler as ph
import ctypes
from ctypes import wintypes

event_handle = None

# Load Kernel32 DLL
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

SHARED_MEMORY_SIZE = 4096
SHARED_MEMORY_NAME = "NetworkMasterPacketDump"
PACKET_CAPTURE_EVENT_NAME = "Global\\PacketCaptureEvent"

# Define constants
PAGE_READWRITE = 0x04
FILE_MAP_READ = 0x04
EVENT_ALL_ACCESS = 0x1F0003
INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value


# def open_shared_memory():
#     # Define function argument and return types for OpenFileMappingW
#     kernel32.OpenFileMappingW.argtypes = [
#         wintypes.DWORD,
#         wintypes.BOOL,
#         wintypes.LPCWSTR,
#     ]
#     kernel32.OpenFileMappingW.restype = wintypes.HANDLE

#     kernel32.MapViewOfFile.argtypes = [
#         wintypes.HANDLE,  # hFileMappingObject
#         wintypes.DWORD,  # dwDesiredAccess
#         wintypes.DWORD,  # dwFileOffsetHigh
#         wintypes.DWORD,  # dwFileOffsetLow
#         ctypes.c_size_t,  # dwNumberOfBytesToMap
#     ]
#     kernel32.MapViewOfFile.restype = wintypes.LPVOID

#     # Open the existing shared memory object
#     hFileMap = kernel32.OpenFileMappingW(
#         FILE_MAP_READ,  # Desired access (read-only in this case)
#         False,  # Inherit handle
#         SHARED_MEMORY_NAME,  # Name of the shared memory object
#     )

#     if not hFileMap or hFileMap == INVALID_HANDLE_VALUE:
#         print("Failed to open shared memory. Error:", ctypes.GetLastError())
#         return None

#     # Map view of the file to get a pointer to the shared memory
#     pBuffer = kernel32.MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0)

#     if not pBuffer:
#         print("Failed to map view of shared memory. Error:", ctypes.GetLastError())
#         kernel32.CloseHandle(hFileMap)
#         return None

#     return pBuffer


def open_event():
    # Define function argument and return types
    kernel32.OpenEventA.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCSTR]
    kernel32.OpenEventA.restype = wintypes.HANDLE

    event_name = ctypes.create_string_buffer(PACKET_CAPTURE_EVENT_NAME.encode("utf-8"))

    # Try to open the event created by the driver
    global event_handle
    event_handle = kernel32.OpenEventA(EVENT_ALL_ACCESS, False, event_name)

    # global event_handle
    # event_handle = kernel32.OpenEventA(
    #     EVENT_ALL_ACCESS,  # Desired access
    #     False,  # Inherit handle
    #     PACKET_CAPTURE_EVENT_NAME,  # Event name
    # )

    if not event_handle or event_handle == INVALID_HANDLE_VALUE:
        print(f"Failed to open event. Error: {ctypes.get_last_error()}")
        return False

    print("Event opened successfully.")
    return True


def wait_for_packet(shared_memory):
    if event_handle is None:
        print("Invalid event handle.")
        return

    while True:
        # Wait for the event to be signaled
        result = win32event.WaitForSingleObject(event_handle, win32event.INFINITE)
        if result == win32event.WAIT_OBJECT_0:
            print("Event signaled, packet ready to be read from shared memory.")

            packet_data = read_received_packet(shared_memory)

            ph.display_packet(packet_data)
        else:
            print(f"Unexpected result from event wait: {result}")


def read_received_packet(shared_mem):
    try:
        # Read packet data from shared memory
        # packet_data = shared_mem.read(SHARED_MEMORY_SIZE)
        packet_data = shared_mem[:SHARED_MEMORY_SIZE]
        return packet_data
    except Exception as e:
        print(f"Error reading packet: {e}")
        return None


def keep_connection_alive():
    while True:
        time.sleep(15 * 60)  # Wait 15 minutes
        try:
            # Call the IOCTL to keep the connection alive
            ioctl.send_ioctl(ioctl.IOCTL_INIT_PACKET_LOGGING)
            print("Sent IOCTL to init packet logging.")
        except Exception as e:
            print(f"Error in keep_connection_alive: {e}")


def close_event_handle():
    if event_handle and event_handle != INVALID_HANDLE_VALUE:
        kernel32.CloseHandle(event_handle)
        print("Event handle closed.")
