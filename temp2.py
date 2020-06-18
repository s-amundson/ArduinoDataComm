import time

import usb1

# VENDOR_ID = 0x03EB
# PRODUCT_ID = 0x2040
# Arduino
VENDOR_ID = 0x2341
PRODUCT_ID = 0x0001
VENDOR_IN_EPADDR = 83
VENDOR_OUT_EPADDR = 4
VENDOR_IO_EPSIZE = 64

data = 'blackBight\n'.encode()

with usb1.USBContext() as context:

    handle = context.openByVendorIDAndProductID(
        VENDOR_ID,
        PRODUCT_ID,
        skip_on_error=True,
    )
    if handle is None:
        exit()
    with handle.claimInterface(0):
        sent = handle.bulkWrite(VENDOR_OUT_EPADDR, data, 2)
        print(f"sent {sent}")
        time.sleep(.5)
        try:
            receive = handle.bulkRead(VENDOR_IN_EPADDR, 64, 10)
            print(f"receive {receive}")

        except usb1.USBErrorTimeout as e:
            print(f"error {e.recieved}")

        # Do stuff with endpoints on claimed interface.