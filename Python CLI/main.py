import threading
import ioctl_handler as ioctl
import shared_memory_handler as smh
import ctypes
from ctypes import wintypes
import signal
import sys
import packet_display_window as window


def signal_handler(sig, frame):
    print("Signal received, exiting...")
    ioctl.send_ioctl(ioctl.IOCTL_END_PACKET_LOGGING)
    smh.close_event_handle()  # Ensure the event handle is closed
    ioctl.close_device()
    sys.exit(0)  # Exit the program


# Register the signal handler for Ctrl+C
signal.signal(signal.SIGINT, signal_handler)


def init_driver_connection():
    # Open the shared memory between the driver and cli
    if not smh.open_shared_memory():
        return False

    # Open the kernel event to know when a new packet arrived
    if not smh.open_event():
        return False

    # Start a thread to wait for packets
    packet_thread = threading.Thread(target=smh.wait_for_packet)
    packet_thread.daemon = True
    packet_thread.start()

    # Start the thread to keep the connection alive
    connection_thread = threading.Thread(target=smh.keep_connection_alive)
    connection_thread.daemon = True
    connection_thread.start()

    print("Driver connection initialized and threads started.")
    window.root.mainloop()
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
        ioctl.send_ioctl(ioctl.IOCTL_END_PACKET_LOGGING)
        smh.close_event_handle()
        ioctl.close_device()

    return 0


if __name__ == "__main__":
    main()
