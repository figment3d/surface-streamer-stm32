import can
import time

BITRATE = 500_000
TX_ID = 0x123
TX_DATA = [0x11, 0x22, 0x33, 0x44]

print("CAN Test")
print("--------")
print("Interface : gs_usb")
print(f"Bitrate   : {BITRATE}")
print(f"TX ID     : 0x{TX_ID:03X}")
print()

bus = None

try:
    bus = can.Bus(
        interface="gs_usb",
        channel=0,
        bitrate=BITRATE,
    )

except Exception as e:
    print("CAN adapter not available.")
    print()
    print(f"{type(e).__name__}: {e}")
    print()
    print("This is expected if the USB-CAN adapter is not connected.")
    raise SystemExit(1)

print("CAN adapter opened successfully.")
print("Press Ctrl+C to stop.")
print()

try:
    while True:
        msg = can.Message(
            arbitration_id=TX_ID,
            data=TX_DATA,
            is_extended_id=False,
        )

        try:
            bus.send(msg, timeout=0.2)
            print(
                f"TX  0x{TX_ID:03X}  "
                + " ".join(f"{b:02X}" for b in TX_DATA)
            )
        except can.CanError as e:
            print(f"TX ERROR: {e}")

        while True:
            rx = bus.recv(timeout=0.05)

            if rx is None:
                break

            print(
                f"RX  0x{rx.arbitration_id:03X}  "
                + " ".join(f"{b:02X}" for b in rx.data)
            )

        time.sleep(0.5)

except KeyboardInterrupt:
    print("\nStopping CAN test.")

finally:
    if bus is not None:
        bus.shutdown()