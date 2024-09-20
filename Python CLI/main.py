import threading
import ioctl_handler as ioctl
import shared_memory_handler as smh


def init_driver_connection():
    # Open shared memory
    shared_mem = smh.open_shared_memory()
    if not shared_mem:
        print("Error:", "Failed to init driver connection")
        return

    # Open the kernel event to know when a new packet arrived
    event_handle = smh.open_event()
    if event_handle:
        # Start a thread to wait for packets
        packet_thread = threading.Thread(
            target=smh.wait_for_packet, args=(event_handle,)
        )
        connection_thread.daemon = True
        packet_thread.start()

    # Start the thread to keep the connection alive
    connection_thread = threading.Thread(target=smh.keep_connection_alive)
    connection_thread.daemon = True
    connection_thread.start()

    print("Driver connection initialized and threads started.")


def main():
    handle = ioctl.open_device()

    if handle is None:
        return

    init_driver_connection()

    while True:
        print("\nMenu:")
        print("1) Stop inbound traffic")
        print("2) Start inbound traffic")
        print("3) Exit")

        choice = input("Enter your choice (1/2/3): ").strip()

        if choice == "1":
            ioctl.send_ioctl(handle, ioctl.IOCTL_STOP_INBOUND_TRAFFIC)
        elif choice == "2":
            ioctl.send_ioctl(handle, ioctl.IOCTL_START_INBOUND_TRAFFIC)
        elif choice == "3":
            break
        else:
            print("Invalid choice, please enter 1, 2, or 3.")

    ioctl.close_device(handle)


if __name__ == "__main__":
    main()

"""
use send_ioctl to call the init_packet_logging_ioctl,
make a call to this ioctl in an infinite loop on a seperate thread every 15 minutes.
wait for a kernel event and read the buffer every time there's one in an infinite loop on a seperate thread.
display the packet read to the user.
"""
