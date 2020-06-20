import codecs
import time

import usb.core
import usb.util
# USB BULK2
VID = 0x03EB
PID = 0x2040

# Arduino
# VID = 0x2341
# PID = 0x0001


VENDOR_IN_EPADDR = 0x83
VENDOR_OUT_EPADDR = 0x04
VENDOR_IO_EPSIZE = 64
CONTROL_SEND_SIZE = 0x10
CONTROL_SEND_USART = 0x11
CMD_DATA_AVAILABLE = 0x31
CMD_REQUEST_DATA = 0x32

msg = 'a\n'.encode()
pkg = len(msg).to_bytes(2, 'little') + msg
print(msg)
# pkg = msg

dev = usb.core.find(idVendor=VID, idProduct=PID)
if not dev:
    print("Could not find dev")
    exit(1)

if dev.is_kernel_driver_active(0):
    dev.detach_kernel_driver(0)
    dev.reset()


print("found dev")
# dev.set_configuration()
print(dev.ctrl_transfer(0x40, CONTROL_SEND_SIZE, 0, 0, len(pkg)))
# print(dev.ctrl_transfer(0x40, CONTROL_SEND_USART, 0, 0, pkg))
dev.set_configuration()
dev.write(VENDOR_OUT_EPADDR, pkg, 500)
time.sleep(1)
available_data = 0
count = 0
while available_data == 0:
    print(f'count = {count}')
    a = dev.ctrl_transfer(0xC0, CMD_DATA_AVAILABLE, 0, 0, 2)
    available_data = (a[1] << 4) + a[0]
    print(f"available_data = {available_data}") \

    time.sleep(1)
    count += 1
    if count > 10:
        print('break')
        break
if available_data:
    # pass
    print(dev.ctrl_transfer(0xC0, CMD_REQUEST_DATA, 0, 0, available_data))
    # ret = dev.read(VENDOR_IN_EPADDR, available_data, 1000)
    # print(ret)
    # s = ""
    # index = 0
    # for c in ret:
    #     if 1 < index < len(msg) + 2:
    #         s += chr(c)
    #     index += 1
    #     # print(chr(c))
    # print(s)
    # ctrl_transfer(self, bmRequestType, bRequest, wValue=0, wIndex=0,
    #             data_or_wLength = None, timeout = None)
    # print(dev.ctrl_transfer(0xC0, CMD_REQUEST_DATA, 0, 0, available_data))
    # print(dev.read(VENDOR_IN_EPADDR, 1, 1000))
    # dev.ctrl_transfer(bmRequestType, bmRequest, wValue, wIndex ,msg or read len)
    # dev.ctrl_transfer(0x40, bmRequest, 0, 0, msg)


