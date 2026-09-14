import time
import can

for i in range(1, 2001):
    bus = None

    try:
        bus = can.Bus(
            interface="gs_usb",
            channel=0,
            bitrate=500000
        )

        print(i, "OPEN")

    except Exception as e:
        print(i, "FAILED:", e)
        break

    finally:
        if bus is not None:
            bus.shutdown()

    time.sleep(1.0)