import ctypes
from ctypes import wintypes

# ioctl codes
IOCTL_STOP_INBOUND_TRAFFIC = 0x222008  # CTL_CODE(0x00000022, 0x802, 0x0, 0x0)
IOCTL_START_INBOUND_TRAFFIC = 0x222010  # CTL_CODE(0x00000022, 0x804, 0x0, 0x0)

INVALID_HANDLE_VALUE = -1

# Define constants
GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3

# Load Kernel32 DLL
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)


def CTL_CODE(DeviceType, Function, Method, Access):
    return hex(((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))


def open_device():
    kernel32.CreateFileW.argtypes = [
        wintypes.LPCWSTR,  # lpFileName
        wintypes.DWORD,  # dwDesiredAccess
        wintypes.DWORD,  # dwShareMode
        wintypes.LPVOID,  # lpSecurityAttributes
        wintypes.DWORD,  # dwCreationDisposition
        wintypes.DWORD,  # dwFlagsAndAttributes
        wintypes.HANDLE,  # hTemplateFile
    ]
    kernel32.CreateFileW.restype = wintypes.HANDLE

    handle = kernel32.CreateFileW(
        r"\\.\NetworkMaster",
        GENERIC_READ | GENERIC_WRITE,
        0,
        None,
        OPEN_EXISTING,
        0,
        None,
    )

    if handle == INVALID_HANDLE_VALUE:
        print("Failed to open device. Error:", ctypes.GetLastError())
        return None
    return handle


def send_ioctl(handle, ioctl_code):
    kernel32.DeviceIoControl.argtypes = [
        wintypes.HANDLE,  # hDevice
        wintypes.DWORD,  # dwIoControlCode
        wintypes.LPVOID,  # lpInBuffer
        wintypes.DWORD,  # nInBufferSize
        wintypes.LPVOID,  # lpOutBuffer
        wintypes.DWORD,  # nOutBufferSize
        ctypes.POINTER(wintypes.DWORD),  # lpBytesReturned
        wintypes.LPVOID,  # lpOverlapped
    ]
    kernel32.DeviceIoControl.restype = wintypes.BOOL

    # Create buffers
    in_buffer = ctypes.create_string_buffer(0)  # Empty buffer with size 0
    out_buffer = ctypes.create_string_buffer(1024)  # Adjust size as needed
    bytes_returned = wintypes.DWORD()

    # Send IOCTL
    success = kernel32.DeviceIoControl(
        handle,
        ioctl_code,
        ctypes.byref(in_buffer),
        ctypes.sizeof(in_buffer),
        ctypes.byref(out_buffer),
        ctypes.sizeof(out_buffer),
        ctypes.byref(bytes_returned),
        None,
    )

    if not success:
        print("IOCTL failed. Error:", ctypes.GetLastError())
    else:
        print("IOCTL succeeded, bytes returned:", bytes_returned.value)


def main():
    handle = open_device()

    if handle is None:
        return

    while True:
        print("\nMenu:")
        print("1) Stop inbound traffic")
        print("2) Start inbound traffic")
        print("3) Exit")

        choice = input("Enter your choice (1/2/3): ").strip()

        if choice == "1":
            send_ioctl(handle, IOCTL_STOP_INBOUND_TRAFFIC)
        elif choice == "2":
            send_ioctl(handle, IOCTL_START_INBOUND_TRAFFIC)
        elif choice == "3":
            break
        else:
            print("Invalid choice, please enter 1, 2, or 3.")

    # Ensure handle is cast to the correct type before closing
    if handle and handle != INVALID_HANDLE_VALUE:
        kernel32.CloseHandle(ctypes.c_void_p(handle))


if __name__ == "__main__":
    main()
