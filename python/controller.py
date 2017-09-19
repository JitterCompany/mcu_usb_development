import datetime
import ntpath
import array
import re
import time

from device_list import DeviceList
from tools import hash_serial
#from update_server import FirmwareUpdateServer
import usbthread

USB_VENDOR_ID = 0x3853
USB_PRODUCT_ID = 0x0021

PROTOCOL_EP = 1

DC_PREFIX = "Info: DC:"
TERMINAL_PREFIX = "Info: terminal:"
NUMFILES_PREFIX = TERMINAL_PREFIX + " num_files: "
FILES_PREFIX = TERMINAL_PREFIX + " files: "

READ_TIMEOUT = 1
READ_POLL_TIME = 6
WRITE_TIMEOUT = 300

# firmware update settings
FW_HOSTNAME = "localhost"
FW_PORT     = 3853

class Controller:

    def __init__(self):

        self.usbThread = usbthread.USBThread()
#        self.firmware_updates = FirmwareUpdateServer((FW_HOSTNAME, FW_PORT),
#                [], self)
#        self.firmware_updates.start()
        # TODO: stop() on exit
        self.n = 0
        self.selectedDevice = None
        self.deviceList = DeviceList(USB_VENDOR_ID, USB_PRODUCT_ID) 

    def enumerate_devices(self):
        self.hotplug_poll()

    def hotplug_poll(self):
        if self.deviceList.has_changed():
            
            removed, added = self.deviceList.update()
            
            for dev in removed:
                self.cancel_autoread_protocol_ep(dev)

            for dev in added:
                print(dev)
                self.autoread_protocol_ep(dev, READ_TIMEOUT)
            
            devices = self.deviceList.find_all()

            # update GUI with updated device list
            device_ids = []
            for dev in devices:
                device_ids.append(hash_serial(dev.serial_number))

            self.select_device(0);
            #self.firmware_updates.update_device_list(device_ids)


    def select_device(self, index):
        
        # stop plotting previous device
        #if self.selectedDevice is not None:
        #    self.cancel_autoread_plot_ep(self.selectedDevice)

        devices = self.deviceList.find_all()
        if (0 <= index < len(devices)):
            self.selectedDevice = devices[index]
        else:
            self.selectedDevice = None
        #self.debuglog.select_device(self.selectedDevice)
        if self.selectedDevice is None:
            print("Selecting device:\n", self.selectedDevice)
        else:
            print("Selecting device:\n", self.selectedDevice._str())

        # start plotting this device
        #if self.selectedDevice is not None:
        #    self.autoread_plots(self.selectedDevice, READ_TIMEOUT)


    def send_file(self, filePath):
        with open(filePath, 'r') as f:
            filedata = f.read()
            filename = ntpath.basename(filePath)
            self.send_file_data(filename, filedata)

    def send_file_data(self, filename, filedata):
        ext_index = filename.find('.')
        if ext_index >= 0 and (len(filename) - ext_index) > 4:
            print("ERROR: invalid 8.3 filename: extension too long")
        elif ext_index > 8:
            print("ERROR: invalid 8.3 filename: base name too long")
        elif len(filename) > 11:
            print("ERROR: invalid 8.3 filename: name too long")

        elif not self.selectedDevice:
            print("ERROR: no device selected for upload")

        else:
            header = "file " + filename + " " + str(len(filedata)) + " "
            self.usbThread.write(self.selectedDevice, 
                    PROTOCOL_EP, header, 15000)
            print("sending:\n",filename, " file size:", len(filedata))
            if len(filedata):
                self.usbThread.write(self.selectedDevice, 
                        PROTOCOL_EP, filedata, 15000)


    def update(self):
        """ High frequent updates """
        self.read_poll()

    def read_poll(self):
        task = self.usbThread.complete_read_task()
        if not task:
            return

        if task.ep == PROTOCOL_EP:
            if (len(task.data) > 0):
                text = ''.join([chr(c) for c in task.data])
                lines = text.split('\n')
                for l in lines:
                    self.process_line(l, task.device)
                    if ("Hello" in l):
                        self.send_ping()

    def send_ping(self):
        print("Send 1 messages..")
        #self.usbThread.write(self.selectedDevice, PROTOCOL_EP, "EENEENEEN", 100)
        #self.usbThread.write(self.selectedDevice, PROTOCOL_EP, "TWEETWEETWEE", 100)
        self.usbThread.write(self.selectedDevice, PROTOCOL_EP, "__MSG__" + str(self.n), 100)
        self.n += 1

    def autoread_protocol_ep(self, dev, timeout):
        self.usbThread.read(dev, PROTOCOL_EP, 512, timeout, repeat=True)

    def cancel_autoread_protocol_ep(self, dev):
        self.usbThread.cancel_autoreads(dev, [PROTOCOL_EP])

    def process_line(self, line, dev):
        """ Parse one line of text and send to correct destination"""
        if not line:
            return
        else:
            print(line);

    def slow_update(self):
        """ Low frequent updates  """
        
        #self.firmware_updates.poll()
        self.hotplug_poll()

       
        
        # call all write callbacks from the gui thread
        while self.usbThread.complete_write_task():
            pass

        if self.selectedDevice:
            self.send_ping()
    

    def send_cmd(self, cmd):
        print("send_cmd({})".format(cmd))
        self.usbThread.write(self.selectedDevice, 
            PROTOCOL_EP, cmd+"\0", 100)




        
if __name__ == '__main__':
    app = Controller();


    count = 0
    while True:
        app.read_poll();
        time.sleep(0.05);
        count += 1
        if count >= 20:
            app.slow_update();
            count = 0
