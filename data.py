import serial
import tkinter as tk
from tkinter import ttk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import os

# Serial port configuration (adjust according to your setup)
SERIAL_PORT = "COM5"  # Replace with your LoRa receiver's serial port
BAUD_RATE = 9600      # Match the baud rate of your LoRa transmitter

# Log file configuration
LOG_FILE = "data.csv"

# Global variables for data
altitudes = []
temperatures = []
timestamps = []

with open(LOG_FILE, "w") as f:
    f.write("Sample,Timestamp,Temperature,Altitude\n")

# Open the serial port once at the beginning
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Serial port {SERIAL_PORT} opened successfully.")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    ser = None

# Function to read serial data
def read_serial_data():
    global altitudes, temperatures, timestamps
    if not ser or not ser.is_open:
        print("Serial port not open.")
        return
    
    while True:
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
                print(f"Received line: {line}")  # Debug print
                
                # Assuming the format is: "sample_number,timestamp,temperature,altitude"
                parts = line.split(',')
                if len(parts) == 4:
                    sample, timestamp, temperature, altitude = parts
                    altitudes.append(float(altitude))
                    temperatures.append(float(temperature))
                    timestamps.append(float(timestamp))

                    with open(LOG_FILE, "a") as f:
                        f.write(f"{sample},{timestamp},{temperature},{altitude}\n")

                    if len(altitudes) > 100:  # Keep only the last 100 samples
                        altitudes.pop(0)
                        temperatures.pop(0)
                        timestamps.pop(0)
                else:
                    print(f"Invalid data format: {line}")
            except Exception as e:
                print(f"Error parsing data: {e}")
                continue

# Function to update the GUI plots
def update_plot():
    global altitudes, temperatures, timestamps
    if timestamps:
        # Update altitude plot
        ax1.clear()
        ax1.plot(timestamps, altitudes, label="Altitude (m)", color="blue")
        ax1.set_title("Altitude")
        ax1.set_xlabel("Time (ms)")
        ax1.set_ylabel("Altitude (m)")
        ax1.legend()

        # Update temperature plot
        ax2.clear()
        ax2.plot(timestamps, temperatures, label="Temperature (°C)", color="red")
        ax2.set_title("Temperature")
        ax2.set_xlabel("Time (ms)")
        ax2.set_ylabel("Temperature (°C)")
        ax2.legend()

        # Refresh the canvas
        canvas.draw()

    # Schedule the next update
    root.after(1000, update_plot)

# Create the main window
root = tk.Tk()
root.title("Rocket Data Visualization")

# Start the serial reader in a separate thread
thread = threading.Thread(target=read_serial_data, daemon=True)
thread.start()

# Create a Matplotlib figure
fig = Figure(figsize=(8, 6), dpi=100)
ax1 = fig.add_subplot(211)  # Altitude subplot
ax2 = fig.add_subplot(212)  # Temperature subplot

# Embed the Matplotlib figure into the Tkinter GUI
canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas_widget.pack(fill=tk.BOTH, expand=True)

# Start updating the plot
update_plot()

# Start the Tkinter main loop
root.mainloop()
