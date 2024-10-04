import threading
import ioctl_handler as ioctl
import shared_memory_handler as smh
import ctypes
from ctypes import wintypes
import signal
import sys


def signal_handler(sig, frame):
    print("Signal received, exiting...")
    smh.close_event_handle()  # Ensure the event handle is closed
    ioctl.close_device()
    sys.exit(0)  # Exit the program


# Register the signal handler for Ctrl+C
signal.signal(signal.SIGINT, signal_handler)


def init_driver_connection():
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

    print("Shared Memory Address:", hex(shared_memory_pointer.value))

    # Create a buffer pointing to that memory space
    shared_memory = (ctypes.c_char * smh.SHARED_MEMORY_SIZE).from_address(
        shared_memory_pointer.value
    )

    # Open shared memory
    # shared_mem = smh.open_shared_memory()
    # if not shared_mem:
    #     return False

    # Open the kernel event to know when a new packet arrived
    if not smh.open_event():
        return False

    # Start a thread to wait for packets
    packet_thread = threading.Thread(target=smh.wait_for_packet, args=(shared_memory,))
    packet_thread.daemon = True
    packet_thread.start()

    # Start the thread to keep the connection alive
    connection_thread = threading.Thread(target=smh.keep_connection_alive)
    connection_thread.daemon = True
    connection_thread.start()

    print("Driver connection initialized and threads started.")
    return True


def main():
    try:
        if not ioctl.open_device():
            return 1

        if not init_driver_connection():
            print("Error occurred initializing driver connection")
            return 1

        while True:
            print("\nMenu:")
            print("1) Stop inbound traffic")
            print("2) Start inbound traffic")
            print("3) Exit")

            choice = input("Enter your choice (1/2/3): ").strip()

            if choice == "1":
                ioctl.send_ioctl(ioctl.IOCTL_STOP_INBOUND_TRAFFIC)
            elif choice == "2":
                ioctl.send_ioctl(ioctl.IOCTL_START_INBOUND_TRAFFIC)
            elif choice == "3":
                break
            else:
                print("Invalid choice, please enter 1, 2, or 3.")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        smh.close_event_handle()
        ioctl.close_device()

    return 0


if __name__ == "__main__":
    main()
