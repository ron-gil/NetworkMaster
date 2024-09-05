import ctypes
from ctypes import wintypes

# Define constants
GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3

# Load Kernel32 DLL
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)


def main():
    # Define the CreateFile function
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

    # Open the device
    handle = kernel32.CreateFileW(
        r"\\.\NetworkMaster",  # Symbolic link to your device
        GENERIC_READ | GENERIC_WRITE,
        0,  # No sharing
        None,  # Default security
        OPEN_EXISTING,
        0,  # No flags
        None,  # No template file
    )

    if handle == -1:
        raise ctypes.WinError(ctypes.get_last_error())

    # Define DeviceIoControl
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

    # Define an IOCTL code (this needs to match the IOCTL code in your driver)
    IOCTL_CREATE_FILTER = 0x80002000  # Example value

    # Send IOCTL
    in_buffer = ctypes.create_string_buffer(b"Data to send to the driver")
    out_buffer = ctypes.create_string_buffer(1024)  # Adjust size as needed
    bytes_returned = wintypes.DWORD()

    success = kernel32.DeviceIoControl(
        handle,
        IOCTL_CREATE_FILTER,
        ctypes.byref(in_buffer),
        ctypes.sizeof(in_buffer),
        ctypes.byref(out_buffer),
        ctypes.sizeof(out_buffer),
        ctypes.byref(bytes_returned),
        None,
    )

    if not success:
        raise ctypes.WinError(ctypes.get_last_error())
    else:
        print("IOCTL succeeded, bytes returned:", bytes_returned.value)

    kernel32.CloseHandle(handle)


if __name__ == "__main__":
    main()
