import serial
import time

PORT = "COM7"
BAUD = 115200

ser = None

try:
    ser = serial.Serial(PORT, BAUD, timeout=1.0)

except serial.SerialException:
    print("STM32 - UART busy or unavailable")
    raise SystemExit(0)

try:
    time.sleep(0.2)

    ser.reset_input_buffer()

    ser.write(b"STM32_STATUS\r\n")
    ser.flush()

    found = False
    end_time = time.time() + 2.0

    while time.time() < end_time:
        line = ser.readline()

        if not line:
            continue

        text = line.decode(errors="replace").strip()

        if text.startswith("STM32_READY"):
            print("STM32 - NUCLEO-H753ZI ONLINE")
            found = True
            break

    if not found:
        print("STM32 - NUCLEO-H753ZI NOT DETECTED")

finally:
    ser.close()