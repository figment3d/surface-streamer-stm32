import time
import can

bus = can.Bus(
    interface="gs_usb",
    channel=0,
    bitrate=500000
)

msg = can.Message(
    arbitration_id=0x123,
    data=[0x11, 0x22, 0x33, 0x44],
    is_extended_id=False
)

try:
    for i in range(1, 2001):
        print(i)

        bus.send(msg)

        rx = bus.recv(timeout=0.1)

        if rx is None:
            print("NO RX")
        else:
            print(
                f"RX 0x{rx.arbitration_id:X} "
                + " ".join(f"{b:02X}" for b in rx.data)
            )

        time.sleep(0.1)

finally:
    bus.shutdown()