import serial

PORT = "COM7"
BAUD = 115200

print("CAN UART Monitor")
print("----------------")
print(f"Port     : {PORT}")
print(f"Baudrate : {BAUD}")
print()
print("Listening for STM32 UART output...")
print("Press Ctrl+C to stop.")
print()

try:
    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        while True:
            line = ser.readline()

            if line:
                text = line.decode(errors="replace").strip()

                if text.startswith("CAN_"):
                    print(text)

except serial.SerialException as e:
    print(f"Serial error: {e}")

except KeyboardInterrupt:
    print("\nStopped.")