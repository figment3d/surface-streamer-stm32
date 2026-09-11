import time
import serial
import can

PORT = "COM7"
BAUD = 115200
CAN_ID = 0x123
CAN_DATA = [0x11, 0x22, 0x33, 0x44]

bus = None
ser = None

try:
    ser = serial.Serial(PORT, BAUD, timeout=0.2)

except serial.SerialException:
    print("CAN stage 2 of 5 - UART busy or unavailable")
    raise SystemExit(0)

try:
    bus = can.Bus(
        interface="gs_usb",
        channel=0,
        bitrate=500000
    )

except Exception:
    print("CAN stage 2 of 5 - USB-CAN adapter unavailable")
    ser.close()
    raise SystemExit(0)

try:
    # Clear stale UART input first.
    ser.reset_input_buffer()

    msg = can.Message(
        arbitration_id=CAN_ID,
        data=CAN_DATA,
        is_extended_id=False
    )

    found = False
    end_time = time.time() + 4.0

    while time.time() < end_time and not found:

        bus.send(msg)

        wait_until = time.time() + 0.5

        while time.time() < wait_until:
            line = ser.readline()

            if not line:
                continue

            text = line.decode(errors="replace").strip()

            if text.startswith("CAN_RX 0x123"):
                found = True
                break

    if found:
        print("CAN stage 2 of 5 - STM32 CAN receive path verified")
    else:
        print("CAN stage 2 of 5 - STM32 CAN receive path NOT DETECTED")

finally:
    bus.shutdown()
    ser.close()