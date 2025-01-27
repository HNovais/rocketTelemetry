import tkinter as tk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import csv

# Log file path
LOG_FILE = "data.csv"

# Lists to store data
altitudes = []
temperatures = []
pressures = []
rates = []
timestamps = []

# Read the log file and extract data
def read_log_file():
    global altitudes, temperatures, timestamps, pressures, rates
    try:
        with open(LOG_FILE, "r") as f:
            reader = csv.reader(f)
            next(reader)  # Skip header row
            for row in reader:
                if len(row) == 6:
                    sample, timestamp, temperature, pressure, altitude, rateOfClimb = row
                    timestamps.append(float(timestamp) / 1000)
                    temperatures.append(float(temperature))
                    pressures.append(float(pressure))
                    altitudes.append(float(altitude))
                    rates.append(float(rateOfClimb))
    except Exception as e:
        print(f"Error reading log file: {e}")

# Function to update the GUI plots
def update_plot():
    global altitudes, temperatures, timestamps, pressures, rates
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

        # Update pressure plot
        ax3.clear()
        ax3.plot(timestamps, pressures, label="Pressure (hPa)", color="green")
        ax3.set_title("Pressure")
        ax3.set_xlabel("Time (ms)")
        ax3.set_ylabel("Pressure (hPa)")
        ax3.legend()

        # Update rate of climb plot
        ax4.clear()
        ax4.plot(timestamps, rates, label="Rate of Climb (m/s)", color="purple")
        ax4.set_title("Rate of Climb")
        ax4.set_xlabel("Time (ms)")
        ax4.set_ylabel("Rate of Climb (m/s)")
        ax4.legend()

        # Refresh the canvas
        canvas.draw()

# Create the main window
root = tk.Tk()
root.title("Rocket Data Visualization - Log File")

# Read the log file and extract data
read_log_file()

# Create a Matplotlib figure
fig = Figure(figsize=(10, 10), dpi=100)

# Create subplots
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
