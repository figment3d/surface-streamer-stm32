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
    saw_uart_traffic = False
    end_time = time.time() + 2.0

    print("STM32_STATUS test")
    print("  TX: STM32_STATUS")

    while time.time() < end_time:
        line = ser.readline()

        if not line:
            continue

        text = line.decode(errors="replace").strip()

        if not text:
            continue

        saw_uart_traffic = True
        print(f"  RX: {text}")

        if text.startswith("STM32_READY"):
            print("STM32 - NUCLEO-H753ZI ONLINE")
            found = True
            break

    if not found:
        if saw_uart_traffic:
            print("STM32 - NUCLEO-H753ZI STATUS RESPONSE TIMEOUT")
            print("  UART traffic was received, but STM32_READY was not.")
        else:
            print("STM32 - NUCLEO-H753ZI NOT DETECTED")
            print("  No UART traffic received.")
finally:
    ser.close()
      