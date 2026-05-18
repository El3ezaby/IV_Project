import serial
import time
import matplotlib.pyplot as plt

# Initialize Serial connection (Update COM port if needed)
arduino = serial.Serial(port='COM4', baudrate=9600, timeout=1)
time.sleep(2)  # Wait for Arduino to initialize

voltages = []
currents = []

# Function to read response from Arduino
def read_arduino_response():
    while arduino.in_waiting < 5:  # Wait for at least 5 bytes
        pass
    response = arduino.read(5)  # Read 5 bytes

    if response[0] == 255:  # Ensure correct start byte
        v0 = (response[1] * 256 + response[2]) * (5.0 / 1024.0)  # Convert ADC to voltage
#         print(response[1] * 256)
        v1 = (response[3] * 256 + response[4]) * (5.0 / 1024.0)  # Convert ADC to voltage
        i_diode = (v0 - v1) / 110.0  # Current in mA (100Ω resistor)
        return v1, i_diode
    return None, None

print("\n📌 **IV Data from Arduino**\n")
print(" PWM  | Voltage (V) | Current (mA) ")
print("-----------------------------------")

# Sending PWM values from 255 to 1
for val in range(255, 0, -1):
    nrep = 10  # Take 10 samples for better averaging
    arduino.write(bytes([255, val, nrep]))  # Send PWM value
    time.sleep(0.05)  # Allow Arduino to process
    
    voltage, current = read_arduino_response()
    if voltage is not None and current is not None:
        voltages.append(voltage)
        currents.append(current)
        print(f" {val:3}  | {voltage:.3f} V   | {current:.3f} mA")  # Print formatted values

# Close serial connection
arduino.close()

# Plot IV characteristics
plt.figure(figsize=(8, 6))
plt.plot(voltages, currents, marker='o', linestyle='-', color='b', label='IV Curve')
plt.xlabel("Voltage (V)")
plt.ylabel("Current (mA)")
plt.title("IV Characteristics of the Diode")
plt.grid(True)
plt.legend()
plt.show()
