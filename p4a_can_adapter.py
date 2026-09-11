import can

bus = None

try:
    bus = can.Bus(
        interface="gs_usb",
        channel=0,
        bitrate=500000
    )

    print("CAN stage 1 of 5 - USB-CAN adapter ready")

except Exception:
    print("CAN stage 1 of 5 - USB-CAN adapter OFFLINE")

finally:
    if bus:
        bus.shutdown()