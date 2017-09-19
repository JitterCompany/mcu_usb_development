import queue
import time
import threading
import traceback

import usb.core
import usb.backend.libusb1 as libusb

import error

class USBTask:

    def __init__(self, ep, timeout, device, onComplete, repeat):
        self.ep = ep
        self.timeout = timeout
        self.device = device
        self.onComplete = onComplete
        self.repeat = repeat

    def __lt__(self, other):
        return self.priority < other.priority


class USBReadTask(USBTask):

    def __init__(self, device, ep, length, timeout=10, onComplete=None,
            repeat=False):
        super().__init__(ep, timeout, device, onComplete, repeat)
        self.length = length
        self.data = []

class USBWriteTask(USBTask):

    def __init__(self, device, ep, data, timeout=10, onComplete=None):
        super().__init__(ep, timeout, device, onComplete, repeat=False)
        self.data = data
        self.length = len(data)

class repeatTasks:

    def __init__(self):
        self.tasks = []

    def should_repeat(self, task):
        """ Check if the given task should be repeated """
        if not task.repeat:
            return False

        index = self.find(task.device, task.ep)
        if index is None:
            task.repeat = False
        return task.repeat

    def add(self, task):
        """ Mark all tasks with task.device+task.ep as repeating """
        task.repeat = True

        if self.find(task.device, task.ep) is None:
            self.tasks.append(task)

    def cancel(self, device, ep):
        """ Stop repeating all tasks for this device+ep """
        index = self.find(device, ep)
        if index is not None:
            del self.tasks[index]

    def find(self, device, ep):
        for index, task in enumerate(self.tasks):
            if task.device == device and task.ep == ep:
                return index
        return None


class USBThread:

    def __init__(self):
        self.writeQueue = queue.Queue()
        self.priorityWriteQueue = queue.Queue()
        self.readQueue = queue.Queue()
        self.readCompleteQueue = queue.Queue()
        self.writeCompleteQueue = queue.Queue()
        self.repeatReader = repeatTasks()
       
        timerThread = threading.Thread(target=self.poll)
        timerThread.deamon = True
        timerThread.start();

    def clear_writes(self):
        self.priorityWriteQueue.queue.clear()
        self.writeQueue.queue.clear()

    def complete_task(self, queue):
        if not queue.empty():
            task = queue.get()
            if task.onComplete:
                task.onComplete(task.length)
            return task
        else:
            return

    def complete_read_task(self):
        return self.complete_task(self.readCompleteQueue)

    def complete_write_task(self):
        return self.complete_task(self.writeCompleteQueue)

    def read(self, device, ep, length, timeout=10, onComplete=None, repeat=False):
        if device is None:
            return
        
        task = USBReadTask(device, ep, length, timeout, onComplete, repeat)

        if repeat:
            self.repeatReader.add(task)

        self.addReadTask(task)
        return task

    def cancel_autoreads(self, device, ep_list):
        for ep in ep_list:
            self.repeatReader.cancel(device, ep)
    
    def write(self, device, ep, data, timeout=10, onComplete=None):
        if device is None:
            return

        task = USBWriteTask(device, ep, data, timeout, onComplete)
        self.addWriteTask(task)
        return task

    def addReadTask(self, task):
        self.readQueue.put(task)

    def addWriteTask(self, task):
        self.writeQueue.put(task)
    
    def poll(self):
        while(True):
            self.handleWriteTask()
            self.handleReadTask()
            self.handleReadTask()
            self.handleReadTask()
            self.handleReadTask()
            self.handleReadTask()
            time.sleep(0.001)

    def handleReadTask(self):
        try:
            task = self.readQueue.get(block=False)

            task.data = task.device.read(task.ep | 0x80, task.length, task.timeout)
            if task:
 #               print("read task for ep:", task.ep)
                self.readCompleteQueue.put(task)
            
            if self.repeatReader.should_repeat(task):

                # Note: copy task to avoid re-using the buffer
                self.addReadTask(USBReadTask(task.device, task.ep,
                    task.length, task.timeout, task.onComplete, task.repeat))

        except queue.Empty:
            pass
        except usb.core.USBError as err:
            if (err.backend_error_code == libusb.LIBUSB_ERROR_TIMEOUT
                    or err.backend_error_code == libusb.LIBUSB_ERROR_IO):

                if task.repeat:
                    
                    # Note: copy task to avoid re-using the buffer
                    self.addReadTask(USBReadTask(task.device, task.ep,
                        task.length, task.timeout, task.onComplete, task.repeat))

                if err.backend_error_code == libusb.LIBUSB_ERROR_IO:
                    print("Warning: USB IO error on read")

            elif err.backend_error_code == libusb.LIBUSB_ERROR_NO_DEVICE:
                error.print_error("No Such Device:" + task.device._str())
            else:
                error.print_error("(unexpected) " + traceback.format_exc())

        except Exception:
            error.print_error(traceback.format_exc())

    def handleWriteTask(self):
        q = self.writeQueue
        if not self.priorityWriteQueue.empty():
            q = self.priorityWriteQueue
        try:
            task = q.get(block=False)

            #task.device.clear_halt(task.ep)
            l = task.device.write(task.ep, task.data, task.timeout)
            if l == len(task.data):
                self.writeCompleteQueue.put(task)
            else:
                task.data = task.data[l:]
                self.priorityWriteQueue.put(task)

        except queue.Empty:
            pass
        except usb.core.USBError as err:
            if err.backend_error_code == libusb.LIBUSB_ERROR_TIMEOUT:
                print("Warning: USB Timeout, retrying task")
                self.priorityWriteQueue.put(task)

            elif err.backend_error_code == libusb.LIBUSB_ERROR_IO:
                print("Warning: USB IO error on write")
                self.priorityWriteQueue.put(task)

            elif err.backend_error_code == libusb.LIBUSB_ERROR_NO_DEVICE:
                error.print_error("No Such Device:" + task.device._str())
            else:
                print(traceback.format_exc())
        except Exception:
            print(traceback.format_exc())

