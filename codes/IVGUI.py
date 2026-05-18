import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import numpy as np
import scipy.optimize
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time

# Global variables
ser = None
mode = 'F'
voltages = []
currents = []

# Function to list available COM ports
def list_ports():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

# Toggle connection to Arduino
def toggle_connection():
    global ser
    if ser and ser.is_open:
        ser.close()
        connect_button.config(text="Connect", bg="lightblue")
        com_dropdown.config(state="readonly")
        print("[INFO] Disconnected from Arduino.")
    else:
        selected_port = com_port_var.get()
        if not selected_port:
            messagebox.showerror("Error", "Please select a COM port!")
            return
        try:
            ser = serial.Serial(selected_port, 115200, timeout=1)
            time.sleep(2)  # Allow Arduino to reset
            ser.flush()  # Clear buffer
            connect_button.config(text="Disconnect", bg="red")
            com_dropdown.config(state="disabled")
            print(f"[INFO] Connected to {selected_port}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to connect: {e}")
            print(f"[ERROR] Failed to connect: {e}")

# Set mode (Forward, Reverse, Instant)
def set_mode():
    global mode
    mode = mode_var.get()
    print(f"[INFO] Mode changed to: {mode}")
    if mode == "I":
        pwm_entry.config(state="normal")
    else:
        pwm_entry.config(state="disabled")

# Measure function with exponential fitting
def measure():
    global ser, voltages, currents
    if not ser or not ser.is_open:
        messagebox.showerror("Error", "Arduino is not connected!")
        print("[ERROR] Measurement failed - Arduino not connected.")
        return

    voltages.clear()
    currents.clear()

    print(f"[INFO] Measuring in mode: {mode}")

    if mode == "F":
        ser.write(f"{mode}*\n".encode())
        time.sleep(0.1)  # Allow Arduino to process mode
        threading.Thread(target=sweep_forward_bias, daemon=True).start()
    elif mode == "I":

        threading.Thread(target=read_arduino_data, daemon=True).start()
    elif mode == "R":
        print("[INFO] Reverse Bias mode - No action required.")

# Sweep forward bias and collect IV data until Arduino sends "*"
def sweep_forward_bias():
    global ser, voltages, currents

    if not ser or not ser.is_open:
        print("[ERROR] Serial connection lost.")
        return

    print("[INFO] Waiting for data from Arduino...")

    while True:
        line = ser.readline().decode().strip()  # Read Arduino response

        if not line:
            print("[WARNING] No data received, retrying...")
            continue

        if line == "*":  # Stop condition
            print("[INFO] Measurement complete. Stopping.")
            break

        try:
            voltage, current = map(float, line.split(","))
            voltages.append(voltage)
            currents.append(current)
            print(f"[INFO] Received V={voltage:.2f}V, I={current:.2f}mA")
        except ValueError:
            print(f"[ERROR] Unexpected data format: {line}")

    print("[INFO] All data received. Plotting...")
    update_graph()  # Plot after receiving all data

# Read instant data from Arduino
def read_arduino_data():
    global ser, voltages, currents, mode
    try:
        ser.flushInput()  # Clear buffer
        pwm_value = pwm_var.get()
        if not pwm_value.isdigit():
            messagebox.showerror("Error", "Enter a valid PWM value (integer).")
            return

        time.sleep(0.1)  # Allow Arduino to process command
        command = f"I,{pwm_value}\n"  # Send mode + PWM value
        ser.write(command.encode())
        time.sleep(0.1)

        line = ser.readline().decode().strip()
        if line:
            try:
                voltage, current = map(float, line.split(","))
                voltages.append(voltage)
                currents.append(current)
                print(f"[INFO] Received instant V={voltage:.2f}V, I={current:.2f}mA")
                
                # Update text fields
                voltage_var.set(f"{voltage:.2f} V")
                current_var.set(f"{current:.2f} mA")

                update_graph()
            except ValueError:
                print(f"[ERROR] Unexpected data format: {line}")
        else:
            print("[WARNING] No data received from Arduino.")
    except Exception as e:
        print(f"[ERROR] Exception while reading data: {e}")

# Exponential fitting function
def exp_func(v, A, B):
    return A * np.exp(B * v)

# Update the graph with both scatter and fitted exponential curve
def update_graph():
    ax.clear()
    
    # Scatter plot for original IV data
#     ax.scatter(voltages, currents, marker='o', color='b', label="Measured IV Data")
    ax.plot(voltages, currents, marker='o', linestyle='-', color='g', label="Measured IV Data ")

    # Fit exponential curve if enough data points exist
    if len(voltages) > 2:
        try:
            popt, _ = scipy.optimize.curve_fit(exp_func, voltages, currents, maxfev=5000)
            v_fit = np.linspace(min(voltages), max(voltages), 100)
            i_fit = exp_func(v_fit, *popt)
            ax.plot(v_fit, i_fit, color='r', linestyle='--', label="Exponential Fit")
        except RuntimeError:
            print("[WARNING] Curve fitting failed.")

    ax.set_xlabel("Voltage (V)")
    ax.set_ylabel("Current (mA)")
    ax.legend()
    canvas.draw()


# Tkinter GUI
root = tk.Tk()
root.title("Photovoltaic Diode Measuring System")
root.geometry("1024x768")  # Adjusted for better aspect ratio

# Title Label
tk.Label(root, text="Photovoltaic Diode Measuring System", font=("Arial", 16, "bold")).pack(pady=10)

# COM Port Selection
com_frame = tk.Frame(root)
com_frame.pack(anchor="w", padx=10, pady=5)

com_port_var = tk.StringVar()
ports = list_ports()
com_dropdown = ttk.Combobox(com_frame, textvariable=com_port_var, values=ports, width=15, state="readonly")
com_dropdown.pack(side=tk.LEFT, padx=5)
com_dropdown.set("Select COM Port")

connect_button = tk.Button(com_frame, text="Connect", command=toggle_connection, bg="lightblue")
connect_button.pack(side=tk.LEFT, padx=5)

# Mode Selection
mode_frame = tk.Frame(root)
mode_frame.pack(anchor="w", padx=10, pady=5)

mode_var = tk.StringVar(value="F")
ttk.Radiobutton(mode_frame, text="Forward Bias", variable=mode_var, value="F", command=set_mode).pack(side=tk.LEFT, padx=5)
ttk.Radiobutton(mode_frame, text="Reverse Bias", variable=mode_var, value="R", command=set_mode).pack(side=tk.LEFT, padx=5)
ttk.Radiobutton(mode_frame, text="Instant Value", variable=mode_var, value="I", command=set_mode).pack(side=tk.LEFT, padx=5)

# Frame for PWM, Voltage, and Current in one row
measurement_frame = tk.Frame(root)
measurement_frame.pack(anchor="w", padx=10, pady=5)

# PWM Input
pwm_var = tk.StringVar()
tk.Label(measurement_frame, text="PWM Value:").grid(row=0, column=0, sticky="w", padx=5, pady=2)
pwm_entry = tk.Entry(measurement_frame, textvariable=pwm_var, state="disabled", width=10)
pwm_entry.grid(row=0, column=1, padx=5, pady=2)

# Voltage Display
voltage_var = tk.StringVar(value="0.00 V")
tk.Label(measurement_frame, text="Voltage:").grid(row=0, column=2, sticky="w", padx=5, pady=2)
tk.Entry(measurement_frame, textvariable=voltage_var, state="readonly", width=10).grid(row=0, column=3, padx=5, pady=2)

# Current Display
current_var = tk.StringVar(value="0.00 mA")
tk.Label(measurement_frame, text="Current:").grid(row=0, column=4, sticky="w", padx=5, pady=2)
tk.Entry(measurement_frame, textvariable=current_var, state="readonly", width=10).grid(row=0, column=5, padx=5, pady=2)

# Measure Button
measure_button = tk.Button(root, text="Measure", command=measure, bg="lightgreen", font=("Arial", 12, "bold"))
measure_button.pack(pady=10)

# Matplotlib figure for graphs
fig, ax = plt.subplots(figsize=(6, 4))
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack()

root.mainloop()
