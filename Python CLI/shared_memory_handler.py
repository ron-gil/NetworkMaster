import time
import mmap
import win32file
import win32event
import pywintypes
import ioctl_handler as ioctl
import packet_handler as ph
import ctypes
from ctypes import wintypes
import packet_display_window as window
import threading

event_handle = None
shared_memory = None

# Load Kernel32 DLL
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

SHARED_MEMORY_SIZE = 4096
PACKET_CAPTURE_EVENT_NAME = "Global\\PacketCaptureEvent"
REFRESH_WAIT_TIME = 15 * 60  # 15 minutes

EVENT_ALL_ACCESS = 0x1F0003
INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value

def open_shared_memory():
    # Call the IOCTL to keep the connection alive
    output_buffer, size_of_output = ioctl.send_ioctl(ioctl.IOCTL_INIT_PACKET_LOGGING)

    if not size_of_output and not output_buffer:
        print("Init packet logging ioctl failed")
        return False

    if size_of_output != ctypes.sizeof(ctypes.c_void_p):
        print("Invalid output size returned")
        return False

    # Convert the bytes-like object to an integer address
    pointer_value = int.from_bytes(output_buffer, byteorder="little")

    # Now cast the integer address to a c_void_p (pointer)
    shared_memory_pointer = ctypes.c_void_p(pointer_value)

    # Create a buffer pointing to that memory space
    global shared_memory
    shared_memory = (ctypes.c_char * SHARED_MEMORY_SIZE).from_address(
        shared_memory_pointer.value
    )

    print(
        "Shared Memory opened successfully.\nAddress:", hex(shared_memory_pointer.value)
    )
    return True


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


def wait_for_packet():
    if event_handle is None:
        print("Invalid event handle.")
        return

    while True:
        # Wait for the event to be signaled
        result = win32event.WaitForSingleObject(event_handle, win32event.INFINITE)
        if result == win32event.WAIT_OBJECT_0:

            packet_data = read_received_packet()

            ph.display_packet(packet_data)
        else:
            print(f"Unexpected result from event wait: {result}")


def read_received_packet():
    try:
        # Read packet data from shared memory
        packet_data = shared_memory[:SHARED_MEMORY_SIZE]
        return packet_data
    except Exception as e:
        print(f"Error reading packet: {e}")
        return None


def keep_connection_alive():
    while True:
        time.sleep(REFRESH_WAIT_TIME - 2)
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
