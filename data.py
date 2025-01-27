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
pressures = []
rates = []
timestamps = []
startTime = None  # Set to None initially

# Initialize log file
with open(LOG_FILE, "w") as f:
    f.write("Sample,Timestamp,Temperature,Pressure,Altitude,RateOfClimb\n")

# Open the serial port once at the beginning
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Serial port {SERIAL_PORT} opened successfully.")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    ser = None

# Function to read serial data
def read_serial_data():
    global altitudes, temperatures, timestamps, pressures, rates, startTime
    if not ser or not ser.is_open:
        print("Serial port not open.")
        return
    
    while True:
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
                print(f"Received line: {line}")  # Debug print
                
                # Assuming the format is: "sample_number,timestamp,temperature,pressure,altitude,rateOfClimb"
                parts = line.split(',')
                if len(parts) == 6:
                    sample, timestamp, temperature, pressure, altitude, rateOfClimb = parts
                    
                    timestamp = float(timestamp)
                    if startTime is None:
                        startTime = timestamp  # Set the start timestamp when the first data arrives
                    relativeTime = timestamp - startTime  # Calculate relative timestamp

                    # Ignore data from the first second
                    if relativeTime < 1000:
                        continue

                    altitudes.append(float(altitude))
                    temperatures.append(float(temperature))
                    timestamps.append(relativeTime / 1000)  # Append the relative time in seconds
                    pressures.append(float(pressure))
                    rates.append(float(rateOfClimb))

                    with open(LOG_FILE, "a") as f:
                        f.write(f"{sample},{relativeTime},{temperature},{pressure},{altitude},{rateOfClimb}\n")

                    # Keep only the last 100 samples
                    if len(altitudes) > 100:
                        altitudes.pop(0)
                        temperatures.pop(0)
                        timestamps.pop(0)
                        pressures.pop(0)
                        rates.pop(0)
                else:
                    print(f"Invalid data format: {line}")
            except Exception as e:
                print(f"Error parsing data: {e}")
                continue

# Function to update the GUI plots
def update_plot():
    global altitudes, temperatures, timestamps, pressures, rates
    if timestamps:
        # Update altitude plot
        ax1.clear()
        ax1.plot(timestamps, altitudes, label="Altitude (m)", color="blue")
        ax1.set_title("Altitude")
        ax1.set_xlabel("Time (s)")
        ax1.set_ylabel("Altitude (m)")
        ax1.legend()

        # Update temperature plot
        ax2.clear()
        ax2.plot(timestamps, temperatures, label="Temperature (°C)", color="red")
        ax2.set_title("Temperature")
        ax2.set_xlabel("Time (s)")
        ax2.set_ylabel("Temperature (°C)")
        ax2.legend()

        # Update pressure plot
        ax3.clear()
        ax3.plot(timestamps, pressures, label="Pressure (Pa)", color="green")
        ax3.set_title("Pressure")
        ax3.set_xlabel("Time (s)")
        ax3.set_ylabel("Pressure (Pa)")
        ax3.legend()

        # Update rate of climb plot
        ax4.clear()
        ax4.plot(timestamps, rates, label="Rate of Climb (m/s)", color="purple")
        ax4.set_title("Rate of Climb")
        ax4.set_xlabel("Time (s)")
        ax4.set_ylabel("Rate of Climb (m/s)")
        ax4.legend()

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
fig = Figure(figsize=(10, 10), dpi=100)
ax1 = fig.add_subplot(411)  # Altitude subplot
ax2 = fig.add_subplot(412)  # Temperature subplot
ax3 = fig.add_subplot(413)  # Pressure subplot
ax4 = fig.add_subplot(414)  # Rate of Climb subplot

# Embed the Matplotlib figure into the Tkinter GUI
canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas_widget.pack(fill=tk.BOTH, expand=True)

# Start updating the plot
update_plot()

# Start the Tkinter main loop
root.mainloop()
