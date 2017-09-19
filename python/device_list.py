import threading
import queue
import time

import usb.core
import usb.util

#NOTE: hotplug support is avaliable in pyusb jitter-1.1
if usb.__version__.startswith('jitter'):
    import usb.hotplug as hotplug
else:
    try:
        import usb.hotplug as hotplug
    except:
        print('\033[93m' + "WARNING: no hotplug support!")
        print('\033[93m' + "WARNING: this requires pyusb > jitter-1.1"
                + ", see JitterCompany/pyusb.git")
        print('\033[0m')
    hotplug = None

def _device_in_list(dev, dev_list):
    for d in dev_list:
        if (dev.bus == d.bus
                and dev.address == d.address
                and dev.idVendor == d.idVendor
                and dev.idProduct == d.idProduct):
            return True
    return False

class DeviceList:

    def __init__(self, vendor_id, product_id):
        
        self.devices = []
        self.usb_VID = vendor_id
        self.usb_PID = product_id

        self.hotplugEventQueue = queue.Queue() 
        
        if hotplug is not None:
            event_mask = (hotplug.LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED
                    | hotplug.LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
            flags = hotplug.LIBUSB_HOTPLUG_NO_FLAGS
            dev_class = hotplug.LIBUSB_HOTPLUG_MATCH_ANY

            # NOTE: we MUST keep a reference to hotplug_handle to avoid segfaults
            # in pyusb
            self.hotplug_handle = hotplug.register_callback(event_mask, flags,
                    self.usb_VID, self.usb_PID, dev_class, self._hotplug_cb, 0)
            self.hotplug_iterator = hotplug.loop()

            self.usbEventThread = threading.Thread(target=self._usb_handle_events)
            self.usbEventThread.daemon = True
            self.usbEventThread.start()

        self.first_time = True

    def has_changed(self):
        """ Returns True if device list has changed """
        
        if not self.first_time:
            if self.hotplugEventQueue.empty():
                return False

        self.first_time = False
        
        # empty event queue
        while not self.hotplugEventQueue.empty():
            try:
                self.hotplugEventQueue.get(False)
            except queue.Empty:
                continue

        return True

    def find_all(self):
        """ Returns all devices. Call update() first to update the list """
        return self.devices

    def update(self):
        """ returns (obsolete[], new[]) devices since last update """
        obsolete = []
        new = []

        prev_devices = self.devices
        found_devices = list(self._find_devices())
        self.devices = []

        # close obsolete devices
        for dev in prev_devices:
            if not _device_in_list(dev, found_devices):
                # obsolete: close

                print("==== RM device: ====");
                print(dev._str());
                obsolete.append(dev)
                self._close_device(dev)
            else:
                # still active: keep
                self.devices.append(dev)
      
        # configure new devices
        for dev in found_devices:
            if not _device_in_list(dev, prev_devices):
                # new: add

                print("==== NEW device: ====");
                print(dev._str());
                
                new.append(dev)
                self.devices.append(dev)
                dev.set_configuration()
        
        return (obsolete, new)

    def _usb_handle_events(self):
        while True:
            #hotplugging
            next(self.hotplug_iterator)
            time.sleep(0.1)

    def _hotplug_cb(self, device, event, dummy_ctx):
        print("==== Hotplug ====", device._str(), event)

        self.hotplugEventQueue.put(event)
        return 0

    def _find_devices(self):
        return usb.core.find(idVendor=self.usb_VID, idProduct=self.usb_PID, 
                find_all=True)

    def _close_device(self, device):
            # try to close the device (non-public api)
            #device._ctx.managed_close()

            # try to close the device (public api (but is it right?))
            usb.util.dispose_resources(device)
       

