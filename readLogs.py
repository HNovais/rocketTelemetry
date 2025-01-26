import tkinter as tk
from tkinter import ttk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import csv

# Log file path
LOG_FILE = "data.csv"

# Lists to store data
altitudes = []
temperatures = []
timestamps = []

# Read the log file and extract data
def read_log_file():
    global altitudes, temperatures, timestamps
    try:
        with open(LOG_FILE, "r") as f:
            reader = csv.reader(f)
            next(reader)  # Skip header row
            for row in reader:
                if len(row) == 4:
                    sample, timestamp, temperature, altitude = row
                    timestamps.append(float(timestamp))
                    temperatures.append(float(temperature))
                    altitudes.append(float(altitude))
    except Exception as e:
        print(f"Error reading log file: {e}")

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

# Create the main window
root = tk.Tk()
root.title("Rocket Data Visualization - Log File")

# Read the log file and extract data
read_log_file()

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
