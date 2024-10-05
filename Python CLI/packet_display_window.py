from tkinter import *
import datetime
import threading
import time

# Create the main application window
root = Tk()
root.title("Packets Received")
root.geometry("800x600")  # Set the window size to 800x600 pixels

# Make the rows and columns expand when the window is resized
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=0)  # Row for the label
root.rowconfigure(1, weight=1)  # Row for the text area


def write_data(data):  # runs in background thread
    # Insert new data at the end of the text widget with a newline
    txt.insert(
        END, "********new packet********\n" + str(data) + "\n\n"
    )  # Append new data


# Create and place the label at the top of the window using grid
lbl = Label(root, text="Start")  # Label in main thread
lbl.grid(row=0, column=0, sticky=NSEW)  # Fill the entire cell

# Create a Text widget to hold the received packets
txt = Text(root, wrap="word")  # Text widget for multi-line input
txt.grid(row=1, column=0, sticky=NSEW, padx=10, pady=10)  # Fill the entire cell
