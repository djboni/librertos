#!/usr/bin/python3
import unittest
import os
from unit_test_c_with_python import load, FFIMocks

################################################################################

module_name = 'queue_'

source_files = [
  '../source/queue.c',
  '../source/librertos_state.c',
]

include_paths = [
  '.',
  '../include',
]

compiler_options = [
  '-std=c90',
  '-pedantic',
  '-DLIBRERTOS_PREEMPTION=0',
  '-DLIBRERTOS_PREEMPT_LIMIT=0'
]

module, ffi = load(source_files, include_paths, compiler_options, module_name=module_name, en_code_coverage=True)

################################################################################
# Undefinded functions

Mocks = FFIMocks()

# projdefs.h
Mocks.CreateMock(ffi, 'InterruptsEnable', return_value=None)
Mocks.CreateMock(ffi, 'InterruptsDisable', return_value=None)
Mocks.CreateMock(ffi, 'CriticalVal', return_value=None)
Mocks.CreateMock(ffi, 'CriticalEnter', return_value=None)
Mocks.CreateMock(ffi, 'CriticalExit', return_value=None)
Mocks.CreateMock(ffi, 'LibrertosTestConcurrentAccess', return_value=None)

# librertos.c
Mocks.CreateMock(ffi, 'SchedulerLock', return_value=None)
Mocks.CreateMock(ffi, 'SchedulerUnlock', return_value=None)
Mocks.CreateMock(ffi, 'GetCurrentTask', return_value=ffi.cast("struct task_t*", 0))

# librertos_event.c
Mocks.CreateMock(ffi, 'OSEventRInit', return_value=None)
Mocks.CreateMock(ffi, 'OSEventRwInit', return_value=None)
Mocks.CreateMock(ffi, 'OSEventPrePendTask', return_value=None)
Mocks.CreateMock(ffi, 'OSEventPendTask', return_value=None)
Mocks.CreateMock(ffi, 'OSEventUnblockTasks', return_value=None)

################################################################################

class QueueInit(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Mocks
        Mocks.ResetMocks()

    def testInit(self):
        # Initialize queue
        length, item_size = 8, 4
        buff = ffi.new("uint8_t [%d]" % (length * item_size, ))
        module.QueueInit(self.pque, buff, length, item_size)
        # Values
        self.assertEqual(self.que.item_size, item_size)
        self.assertEqual(self.que.free, length)
        self.assertEqual(self.que.used, 0)
        self.assertEqual(self.que.w_lock, 0)
        self.assertEqual(self.que.r_lock, 0)
        self.assertEqual(self.que.head_ptr, buff)
        self.assertEqual(self.que.tail_ptr, buff)
        self.assertEqual(self.que.buff_ptr, buff)
        self.assertEqual(self.que.buff_end_ptr, buff+(length - 1) * item_size)
        Mocks.OSEventRwInit.assert_called_once_with(self.que.event)

class QueueWrite(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Mocks
        Mocks.ResetMocks()

    def testWriteOnce(self):
        # Writes and succeeds
        data = os.urandom(self.item_size)
        ret = module.QueueWrite(self.pque, data)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.que.item_size, self.item_size)
        self.assertEqual(self.que.free, self.length-1)
        self.assertEqual(self.que.used, 1)
        self.assertEqual(self.que.w_lock, 0)
        self.assertEqual(self.que.r_lock, 0)
        self.assertEqual(self.que.head_ptr, self.buff)
        self.assertEqual(self.que.tail_ptr, self.buff + self.item_size)
        self.assertEqual(self.que.buff_ptr, self.buff)
        self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
        for i in range(len(data)):
            self.assertEqual(data[i], self.que.buff_ptr[i])

    def testWriteMultiple(self):
        data_total = b''
        # Writes and succeeds self.length times
        for num in range(1, self.length + 1):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Values
            self.assertEqual(ret, True)
            self.assertEqual(self.que.item_size, self.item_size)
            self.assertEqual(self.que.free, self.length - num)
            self.assertEqual(self.que.used, num)
            self.assertEqual(self.que.w_lock, 0)
            self.assertEqual(self.que.r_lock, 0)
            self.assertEqual(self.que.head_ptr, self.buff)
            if num < self.length:
                self.assertEqual(self.que.tail_ptr, self.buff + self.item_size * num)
            else:
                # Write wraps
                self.assertEqual(self.que.tail_ptr, self.buff)
            self.assertEqual(self.que.buff_ptr, self.buff)
            self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
            for i in range(len(data_total)):
                self.assertEqual(data_total[i], self.que.buff_ptr[i])
        # Writes and fails
        data = os.urandom(self.item_size)
        ret = module.QueueWrite(self.pque, data)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.que.item_size, self.item_size)
        self.assertEqual(self.que.free, 0)
        self.assertEqual(self.que.used, self.length)
        self.assertEqual(self.que.w_lock, 0)
        self.assertEqual(self.que.r_lock, 0)
        self.assertEqual(self.que.head_ptr, self.buff)
        self.assertEqual(self.que.tail_ptr, self.buff)
        self.assertEqual(self.que.buff_ptr, self.buff)
        self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.que.buff_ptr[i])

    def testWriteUnblocksTask(self):
        # Mimick a task waiting to read
        self.que.event.list_read.length = 1
        # Writes and succeeds
        data = os.urandom(self.item_size)
        ret = module.QueueWrite(self.pque, data)
        # Values
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.que.event.list_read)

class QueueRead(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testReadFail(self):
        # Reads and succeeds
        data = b"\x01" * self.item_size
        ret = module.QueueRead(self.pque, data)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.que.item_size, self.item_size)
        self.assertEqual(self.que.free, self.length)
        self.assertEqual(self.que.used, 0)
        self.assertEqual(self.que.w_lock, 0)
        self.assertEqual(self.que.r_lock, 0)
        self.assertEqual(self.que.head_ptr, self.buff)
        self.assertEqual(self.que.tail_ptr, self.buff)
        self.assertEqual(self.que.buff_ptr, self.buff)
        self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
        for i in range(len(data)):
            self.assertEqual(data[i], 1)

    def testReadOnce(self):
        # Writes and succeeds
        data_total = self.writeOnQueue(1)
        # Reads and succeeds
        data = b"\x00" * self.item_size
        ret = module.QueueRead(self.pque, data)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.que.item_size, self.item_size)
        self.assertEqual(self.que.free, self.length)
        self.assertEqual(self.que.used, 0)
        self.assertEqual(self.que.w_lock, 0)
        self.assertEqual(self.que.r_lock, 0)
        self.assertEqual(self.que.head_ptr, self.buff + self.item_size)
        self.assertEqual(self.que.tail_ptr, self.buff + self.item_size)
        self.assertEqual(self.que.buff_ptr, self.buff)
        self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
        for i in range(len(data)):
            self.assertEqual(data[i], data_total[i])

    def testReadMultiple(self):
        # Writes and succeeds
        data_total = self.writeOnQueue(self.length)
        data_read = b""
        # Reads and succeeds self.length times
        for num in range(self.length-1, -1, -1):
            # Reads and succeeds
            data = b"\x00" * self.item_size
            ret = module.QueueRead(self.pque, data)
            data_read += data_read
            # Values
            self.assertEqual(ret, True)
            self.assertEqual(self.que.item_size, self.item_size)
            self.assertEqual(self.que.free, self.length - num)
            self.assertEqual(self.que.used, num)
            self.assertEqual(self.que.w_lock, 0)
            self.assertEqual(self.que.r_lock, 0)
            if num > 0:
                self.assertEqual(self.que.head_ptr, self.buff + self.item_size * (self.length - num))
            else:
                # Read wraps
                self.assertEqual(self.que.head_ptr, self.buff)
            self.assertEqual(self.que.tail_ptr, self.buff)
            self.assertEqual(self.que.buff_ptr, self.buff)
            self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
            for i in range(len(data_read)):
                self.assertEqual(data_total[i], data_read[i])
        # Reads and fails
        data = b"\x00" * self.item_size
        ret = module.QueueRead(self.pque, data)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.que.item_size, self.item_size)
        self.assertEqual(self.que.free, self.length)
        self.assertEqual(self.que.used, 0)
        self.assertEqual(self.que.w_lock, 0)
        self.assertEqual(self.que.r_lock, 0)
        self.assertEqual(self.que.head_ptr, self.buff)
        self.assertEqual(self.que.tail_ptr, self.buff)
        self.assertEqual(self.que.buff_ptr, self.buff)
        self.assertEqual(self.que.buff_end_ptr, self.buff+(self.length - 1) * self.item_size)
        for i in range(len(data_read)):
            self.assertEqual(data_total[i], data_read[i])

    def testReadUnblocksTask(self):
        # Writes and succeeds
        data_total = self.writeOnQueue(self.length)
        # Mimick a task waiting to write
        self.que.event.list_write.length = 1
        # Reads and succeeds
        data = b"\x00" * self.item_size
        ret = module.QueueRead(self.pque, data)
        # Values
        self.assertEqual(ret, True)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.que.buff_ptr[i])
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.que.event.list_write)

class QueueReadPend(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testReadPendEmptyZeroWait(self):
        # Reads and fails
        wait = 0
        data = b"\x00" * self.item_size
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.QueueReadPend(self.pque, data, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testReadPendEmpty(self):
        # Reads and fails
        wait = 1
        data = b"\x00" * self.item_size
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.QueueReadPend(self.pque, data, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.que.event.list_read, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.que.event.list_read, self.ptask1, wait)

    def testReadPendNonEmpty(self):
        # Writes and succeeds
        self.writeOnQueue(1)
        # Reads and succeeds
        wait = 0
        data = b"\x00" * self.item_size
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.QueueReadPend(self.pque, data, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class QueueWritePend(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testWritePendFullZeroWait(self):
        # Writes and succeeds
        self.writeOnQueue(self.length)
        # Writes and fails
        wait = 0
        data = b"\x00" * self.item_size
        ret = module.QueueWritePend(self.pque, data, wait)
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.QueueWritePend(self.pque, data, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testWritePendFull(self):
        # Writes and succeeds
        self.writeOnQueue(self.length)
        # Writes and fails
        wait = 1
        data = b"\x00" * self.item_size
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.QueueWritePend(self.pque, data, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.que.event.list_write, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.que.event.list_write, self.ptask1, wait)

    def testWritePendNonFull(self):
        # Writes and succeeds
        wait = 0
        data = b"\x00" * self.item_size
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.QueueWritePend(self.pque, data, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class QueuePendRead(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testQueueZeroWait(self):
        # Task1 does not pend
        wait = 0
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.QueuePendRead(self.pque, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testQueueEmpty(self):
        # Task 1 pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.QueuePendRead(self.pque, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.que.event.list_read, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.que.event.list_read, self.ptask1, wait)

    def testQueueNonEmpty(self):
        # Writes and succeeds
        self.writeOnQueue(1)
        # Task1 does not pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.QueuePendRead(self.pque, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class QueuePendWrite(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.cast("struct task_t*", 1)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testQueueFullZeroWait(self):
        # Writes and succeeds
        self.writeOnQueue(self.length)
        # Task1 does not pend
        wait = 0
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.QueuePendWrite(self.pque, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testQueueFull(self):
        # Writes and succeeds
        self.writeOnQueue(self.length)
        # Task 1 pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.QueuePendWrite(self.pque, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.que.event.list_write, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.que.event.list_write, self.ptask1, wait)

    def testQueueNonFull(self):
        # Task 1 does not pend
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.QueuePendWrite(self.pque, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class QueueUtils(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num, succeeds=True):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(self.item_size)
            data_total += data
            ret = module.QueueWrite(self.pque, data)
            # Value
            self.assertEqual(ret, succeeds)
        return data_total

    def testQueueUsed(self):
        self.assertEqual(module.QueueUsed(self.pque), 0)
        for i in range(1, self.length + 1):
            self.writeOnQueue(1)
            self.assertEqual(module.QueueUsed(self.pque), i)
        self.writeOnQueue(1, succeeds=False)
        self.assertEqual(module.QueueUsed(self.pque), self.length)

    def testQueueFree(self):
        self.assertEqual(module.QueueFree(self.pque), self.length)
        for i in range(1, self.length + 1):
            self.writeOnQueue(1)
            self.assertEqual(module.QueueFree(self.pque), self.length-i)
        self.writeOnQueue(1, succeeds=False)
        self.assertEqual(module.QueueFree(self.pque), 0)

    def testQueueLength(self):
        self.assertEqual(module.QueueLength(self.pque), self.length)

    def testQueueItemLength(self):
        self.assertEqual(module.QueueItemSize(self.pque), self.item_size)

    def testQueueEmpty(self):
        self.assertEqual(module.QueueEmpty(self.pque), True)
        self.writeOnQueue(1)
        self.assertEqual(module.QueueEmpty(self.pque), False)

    def testQueueFull(self):
        self.assertEqual(module.QueueFull(self.pque), False)
        self.writeOnQueue(self.length)
        self.assertEqual(module.QueueFull(self.pque), True)

class QueueConcurrentAccess(unittest.TestCase):

    def setUp(self):
        # Queue
        self.pque = ffi.new("struct queue_t[1]")
        self.que = self.pque[0]
        # Initialize queue
        self.length, self.item_size = 8, 4
        self.buff = ffi.new("uint8_t [%d]" % (self.length * self.item_size, ))
        module.QueueInit(self.pque, self.buff, self.length, self.item_size)
        # Mocks
        Mocks.ResetMocks()

    def writeOnQueue(self, num, filler=None, succeeds=True):
        data_total = b''
        if filler == None:
            # Writes and succeeds self.length times
            for i in range(num):
                # Writes and succeeds
                data = os.urandom(self.item_size)
                data_total += data
                ret = module.QueueWrite(self.pque, data)
                # Value
                self.assertEqual(ret, succeeds)
        else:
            # Writes and succeeds self.length times
            for val in filler:
                # Writes and succeeds
                val = bytes([val])
                data = val * self.item_size
                data_total += data
                ret = module.QueueWrite(self.pque, data)
                # Value
                self.assertEqual(ret, succeeds)

        return data_total

    def testQueueWrite(self):
        # Write and concurrent write
        def side_effect_write_queue():
            Mocks.LibrertosTestConcurrentAccess.side_effect = None
            self.writeOnQueue(1, b"b")
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_write_queue
        self.writeOnQueue(1, b"a")

        # Read from first write
        read_val = b"\x00" * self.item_size
        ret = module.QueueRead(self.pque, read_val)
        self.assertEqual(ret, True)
        self.assertEqual(read_val, b"a" * self.item_size)

        # Read from second write (concurrent)
        read_val = b"\x00" * self.item_size
        ret = module.QueueRead(self.pque, read_val)
        self.assertEqual(ret, True)
        self.assertEqual(read_val, b"b" * self.item_size)

    def testQueueRead(self):
        # Write twice
        self.writeOnQueue(1, b"a")
        self.writeOnQueue(1, b"b")

        # Read and concurrent read

        def side_effect_read_queue():
            Mocks.LibrertosTestConcurrentAccess.side_effect = None
            # Second read (concurrent)
            read_val = b"\x00" * self.item_size
            ret = module.QueueRead(self.pque, read_val)
            self.assertEqual(ret, True)
            self.assertEqual(read_val, b"b" * self.item_size)
        Mocks.LibrertosTestConcurrentAccess.call_count = 0
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_read_queue

        # First read
        read_val = b"\x00" * self.item_size
        ret = module.QueueRead(self.pque, read_val)
        self.assertEqual(ret, True)
        self.assertEqual(read_val, b"a" * self.item_size)

        self.assertEqual(module.QueueEmpty(self.pque), True)

################################################################################

if __name__ == "__main__":
    unittest.main()
