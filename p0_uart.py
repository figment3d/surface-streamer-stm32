import time
import serial

PORT = "COM7"
BAUD = 115200
TIMEOUT_SECONDS = 4

print("UART waiting...")

try:
    ser = serial.Serial(PORT, BAUD, timeout=0.5)
except serial.SerialException:
    print("COM7 busy or unavailable - GUI/server may be using UART")
    raise SystemExit(0)

found = False
end_time = time.time() + TIMEOUT_SECONDS

try:
    while time.time() < end_time:
        line = ser.readline().decode(errors="ignore").strip()

        if line == "UART_READY":
            print("UART_READY COM7")
            found = True
            break
finally:
    ser.close()

if not found:
    print("UART timeout - no UART_READY received")