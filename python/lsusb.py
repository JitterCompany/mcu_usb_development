import usb.core
import usb.util

# find our device
dev = usb.core.find()

# was it found?
if dev is None:
    raise ValueError('Device not found')

print(dev)
# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration()

# get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(0, 0)]

desc = usb.util.find_descriptor(intf, find_all=True)
print(desc)

# ep = usb.util.find_descriptor(
#     intf,
#     # match the first OUT endpoint
#     custom_match = \
#     lambda e: \
#         usb.util.endpoint_direction(e.bEndpointAddress) == \
        # usb.util.ENDPOINT_OUT)