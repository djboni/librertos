#!/usr/bin/python3
import unittest
import os
from unit_test_c_with_python import load, FFIMocks

################################################################################

module_name = 'fifo_'

source_files = [
  '../source/fifo.c',
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

class FifoInit(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Mocks
        Mocks.ResetMocks()

    def testInit(self):
        # Ini   tialize fifoue
        length = 8
        buff = ffi.new("uint8_t [%d]" % length)
        module.FifoInit(self.pfifo, buff, length)
        # Values
        self.assertEqual(self.fifo.length, length)
        self.assertEqual(self.fifo.free, length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, buff)
        self.assertEqual(self.fifo.tail_ptr, buff)
        self.assertEqual(self.fifo.buff_ptr, buff)
        self.assertEqual(self.fifo.buff_end_ptr, buff + (length - 1))
        Mocks.OSEventRwInit.assert_called_once_with(self.fifo.event)

class FifoWrite(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Mocks
        Mocks.ResetMocks()

    def testWriteOnce(self):
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWrite(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length-1)
        self.assertEqual(self.fifo.used, 1)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff + 1)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
        for i in range(len(data)):
            self.assertEqual(data[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteMultiple(self):
        data_total = b''
        # Writes and succeeds self.length times
        for num in range(1, self.length + 1):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Values
            self.assertEqual(ret, True)
            self.assertEqual(self.fifo.length, self.length)
            self.assertEqual(self.fifo.free, self.length - num)
            self.assertEqual(self.fifo.used, num)
            self.assertEqual(self.fifo.w_lock, 0)
            self.assertEqual(self.fifo.r_lock, 0)
            self.assertEqual(self.fifo.head_ptr, self.buff)
            if num < self.length:
                self.assertEqual(self.fifo.tail_ptr, self.buff + num)
            else:
                # Write wraps
                self.assertEqual(self.fifo.tail_ptr, self.buff)
            self.assertEqual(self.fifo.buff_ptr, self.buff)
            self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
            for i in range(len(data_total)):
                self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Writes and fails
        data = os.urandom(1)
        ret = module.FifoWrite(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, 0)
        self.assertEqual(self.fifo.used, self.length)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteDoesNotUnblockTaskIfNotEnoughData(self):
        # Mimick a task waiting to read
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 2 # Number of bytes desired by task
        self.fifo.event.list_read.length = 1
        self.fifo.event.list_read.tail_ptr = pnode
        self.fifo.event.list_read.head_ptr = pnode
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWrite(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, True)
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteUnblocksTask(self):
        # Mimick a task waiting to read
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 1 # Number of bytes desired by task
        self.fifo.event.list_read.length = 1
        self.fifo.event.list_read.tail_ptr = pnode
        self.fifo.event.list_read.head_ptr = pnode
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWrite(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, True)
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.fifo.event.list_read)

    def testWriteUnblocksTask(self):
        # Mimick a task waiting to read
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 1 # Number of bytes desired by task
        self.fifo.event.list_read.length = 1
        self.fifo.event.list_read.tail_ptr = pnode
        self.fifo.event.list_read.head_ptr = pnode
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWrite(self.pfifo, data, 1)
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.fifo.event.list_read)

    def testWriteMultipleBytesAndWrapBuffer(self):
        n1 = self.length // 2
        n2 = self.length

        # Writes and succeeds
        data_total = os.urandom(n1)
        ret = module.FifoWrite(self.pfifo, data_total, n1)
        # Values
        self.assertEqual(ret, n1)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])

        # Reads
        data = b"\x00" * n1
        ret = module.FifoRead(self.pfifo, data_total, n1)
        # Values
        self.assertEqual(ret, n1)

        # Writes and succeeds
        data_total = os.urandom(n2)
        ret = module.FifoWrite(self.pfifo, data_total, n2)
        # Values
        self.assertEqual(ret, n2)
        for i in range(len(data_total)):
            buff_pos = (i + n1) % self.length
            self.assertEqual(data_total[i], self.fifo.buff_ptr[buff_pos])

class FifoRead(unittest.TestCase):

    def setUp(self):
      # Fifo
      self.pfifo = ffi.new("struct fifo_t[1]")
      self.fifo = self.pfifo[0]
      # Initialize fifo
      self.length = 8
      self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
      module.FifoInit(self.pfifo, self.buff, self.length)
      # Mocks
      Mocks.ResetMocks()

    def writeOnFifo(self, num):
      data_total = b''
      # Writes and succeeds self.length times
      for i in range(num):
        # Writes and succeeds
        data = os.urandom(1)
        data_total += data
        ret = module.FifoWrite(self.pfifo, data, 1)
        # Value
        self.assertEqual(ret, True)
      return data_total

    def testReadFail(self):
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoRead(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
        for i in range(len(data)):
            self.assertEqual(data[i], 0)
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadOnce(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(1)
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoRead(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff + 1)
        self.assertEqual(self.fifo.tail_ptr, self.buff + 1)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
        for i in range(len(data)):
            self.assertEqual(data[i], data_total[i])
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadMultiple(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(self.length)
        data_read = b""
        # Reads and succeeds self.length times
        for num in range(self.length-1, -1, -1):
            # Reads and succeeds
            data = b"\x00" * 1
            ret = module.FifoRead(self.pfifo, data, 1)
            data_read += data_read
            # Values
            self.assertEqual(ret, True)
            self.assertEqual(self.fifo.length, self.length)
            self.assertEqual(self.fifo.free, self.length - num)
            self.assertEqual(self.fifo.used, num)
            self.assertEqual(self.fifo.w_lock, 0)
            self.assertEqual(self.fifo.r_lock, 0)
            if num > 0:
                self.assertEqual(self.fifo.head_ptr, self.buff + 1 * (self.length - num))
            else:
            # Read wraps
                self.assertEqual(self.fifo.head_ptr, self.buff)
                self.assertEqual(self.fifo.tail_ptr, self.buff)
                self.assertEqual(self.fifo.buff_ptr, self.buff)
                self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
            for i in range(len(data_read)):
                self.assertEqual(data_total[i], data_read[i])
        # Reads and fails
        data = b"\x00" * 1
        ret = module.FifoRead(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
        for i in range(len(data_read)):
            self.assertEqual(data_total[i], data_read[i])
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteDoesNotUnblockTaskIfNotEnoughData(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(self.length)
        # Mimick a task waiting to write
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 2 # Number of bytes desired by task
        self.fifo.event.list_write.length = 1
        self.fifo.event.list_write.tail_ptr = pnode
        self.fifo.event.list_write.head_ptr = pnode
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoRead(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, True)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadUnblocksTask(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(self.length)
        # Mimick a task waiting to write
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 1 # Number of bytes desired by task
        self.fifo.event.list_write.length = 1
        self.fifo.event.list_write.tail_ptr = pnode
        self.fifo.event.list_write.head_ptr = pnode
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoRead(self.pfifo, data, 1)
        # Values
        self.assertEqual(ret, True)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.fifo.event.list_write)

    def testReadMultipleBytesAndWrapBuffer(self):
        n1 = self.length // 2
        n2 = self.length

        # Writes and succeeds
        data_total = self.writeOnFifo(n1)
        # Reads and succeeds
        data = b"\x00" * n1
        ret = module.FifoRead(self.pfifo, data, n1)
        # Values
        self.assertEqual(ret, n1)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])

        # Writes wrapping buffer and succeeds
        data_total = self.writeOnFifo(n2)
        # Reads and succeeds
        data = b"\x00" * n2
        ret = module.FifoRead(self.pfifo, data, n2)
        # Values
        self.assertEqual(ret, n2)
        for i in range(len(data_total)):
            buff_pos = (i + n1) % self.length
            self.assertEqual(data_total[i], self.fifo.buff_ptr[buff_pos])

class FifoWriteByte(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Mocks
        Mocks.ResetMocks()

    def testWriteByteOnce(self):
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWriteByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length-1)
        self.assertEqual(self.fifo.used, 1)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff + 1)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
        for i in range(len(data)):
            self.assertEqual(data[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteMultiple(self):
        data_total = b''
        # Writes and succeeds self.length times
        for num in range(1, self.length + 1):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWriteByte(self.pfifo, data)
            # Values
            self.assertEqual(ret, True)
            self.assertEqual(self.fifo.length, self.length)
            self.assertEqual(self.fifo.free, self.length - num)
            self.assertEqual(self.fifo.used, num)
            self.assertEqual(self.fifo.w_lock, 0)
            self.assertEqual(self.fifo.r_lock, 0)
            self.assertEqual(self.fifo.head_ptr, self.buff)
            if num < self.length:
                self.assertEqual(self.fifo.tail_ptr, self.buff + num)
            else:
                # Write wraps
                self.assertEqual(self.fifo.tail_ptr, self.buff)
            self.assertEqual(self.fifo.buff_ptr, self.buff)
            self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
            for i in range(len(data_total)):
                self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Writes and fails
        data = os.urandom(1)
        ret = module.FifoWriteByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, 0)
        self.assertEqual(self.fifo.used, self.length)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteByteDoesNotUnblockTaskIfNotEnoughData(self):
        # Mimick a task waiting to read
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 2 # Number of bytes desired by task
        self.fifo.event.list_read.length = 1
        self.fifo.event.list_read.tail_ptr = pnode
        self.fifo.event.list_read.head_ptr = pnode
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWriteByte(self.pfifo, data)
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testWriteByteUnblocksTask(self):
        # Mimick a task waiting to read
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 1 # Number of bytes desired by task
        self.fifo.event.list_read.length = 1
        self.fifo.event.list_read.tail_ptr = pnode
        self.fifo.event.list_read.head_ptr = pnode
        # Writes and succeeds
        data = os.urandom(1)
        ret = module.FifoWriteByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, True)
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.fifo.event.list_read)

class FifoReadByte(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testReadByteFail(self):
        # Reads and succeeds
        # TODO For some reason b"\x00" get garbage.
        data = b"\x01"
        ret = module.FifoReadByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff + (self.length - 1))
        for i in range(len(data)):
            self.assertEqual(data[i], 1)
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadByteOnce(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(1)
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoReadByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, True)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff + 1)
        self.assertEqual(self.fifo.tail_ptr, self.buff + 1)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
        for i in range(len(data)):
            self.assertEqual(data[i], data_total[i])
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadByteMultiple(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(self.length)
        data_read = b""
        # Reads and succeeds self.length times
        for num in range(self.length-1, -1, -1):
            # Reads and succeeds
            data = b"\x00" * 1
            ret = module.FifoReadByte(self.pfifo, data)
            data_read += data_read
            # Values
            self.assertEqual(ret, True)
            self.assertEqual(self.fifo.length, self.length)
            self.assertEqual(self.fifo.free, self.length - num)
            self.assertEqual(self.fifo.used, num)
            self.assertEqual(self.fifo.w_lock, 0)
            self.assertEqual(self.fifo.r_lock, 0)
            if num > 0:
                self.assertEqual(self.fifo.head_ptr, self.buff + 1 * (self.length - num))
            else:
            # Read wraps
                self.assertEqual(self.fifo.head_ptr, self.buff)
                self.assertEqual(self.fifo.tail_ptr, self.buff)
                self.assertEqual(self.fifo.buff_ptr, self.buff)
                self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
            for i in range(len(data_read)):
                self.assertEqual(data_total[i], data_read[i])
        # Reads and fails
        data = b"\x00" * 1
        ret = module.FifoReadByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, False)
        self.assertEqual(self.fifo.length, self.length)
        self.assertEqual(self.fifo.free, self.length)
        self.assertEqual(self.fifo.used, 0)
        self.assertEqual(self.fifo.w_lock, 0)
        self.assertEqual(self.fifo.r_lock, 0)
        self.assertEqual(self.fifo.head_ptr, self.buff)
        self.assertEqual(self.fifo.tail_ptr, self.buff)
        self.assertEqual(self.fifo.buff_ptr, self.buff)
        self.assertEqual(self.fifo.buff_end_ptr, self.buff+(self.length - 1))
        for i in range(len(data_read)):
            self.assertEqual(data_total[i], data_read[i])
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadByteDoesNotUnblockTaskIfNotEnoughData(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(self.length)
        # Mimick a task waiting to write
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 2 # Number of bytes desired by task
        self.fifo.event.list_write.length = 1
        self.fifo.event.list_write.tail_ptr = pnode
        self.fifo.event.list_write.head_ptr = pnode
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoReadByte(self.pfifo, data)
        # Values
        self.assertEqual(ret, True)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_not_called()

    def testReadByteUnblocksTask(self):
        # Writes and succeeds
        data_total = self.writeOnFifo(self.length)
        # Mimick a task waiting to write
        pnode = ffi.new("struct task_list_node_t[1]")
        node = pnode[0]
        node.value = 1 # Number of bytes desired by task
        self.fifo.event.list_write.length = 1
        self.fifo.event.list_write.tail_ptr = pnode
        self.fifo.event.list_write.head_ptr = pnode
        # Reads and succeeds
        data = b"\x00" * 1
        ret = module.FifoReadByte(self.pfifo, data)
        for i in range(len(data_total)):
            self.assertEqual(data_total[i], self.fifo.buff_ptr[i])
        # Calls
        Mocks.OSEventUnblockTasks.assert_called_once_with(self.fifo.event.list_write)

class FifoReadPend(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.new("struct task_t[1]")
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testReadPendEmptyZeroWait(self):
        # Reads and fails
        wait = 0
        data = b"\x00" * 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.FifoReadPend(self.pfifo, data, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testReadPendEmpty(self):
        # Reads and pends
        wait = 1
        data = b"\x00" * 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.FifoReadPend(self.pfifo, data, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.fifo.event.list_read, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.fifo.event.list_read, self.ptask1, wait)

    def testReadPendNonEmpty(self):
        # Writes and succeeds
        self.writeOnFifo(1)
        # Reads and pends
        wait = 1
        data = b"\x00" * 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.FifoReadPend(self.pfifo, data, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class FifoWritePend(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.new("struct task_t[1]")
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testWritePendFullZeroWait(self):
        # Writes and succeeds
        self.writeOnFifo(self.length)
        # Reads and fails
        wait = 0
        data = b"\x00" * 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.FifoWritePend(self.pfifo, data, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testWritePendFull(self):
        # Writes and succeeds
        self.writeOnFifo(self.length)
        # Reads and fails
        wait = 1
        data = b"\x00" * 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.FifoWritePend(self.pfifo, data, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, False)
        # Calls
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.fifo.event.list_write, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.fifo.event.list_write, self.ptask1, wait)

    def testWritePendNonFull(self):
        # Reads and fails
        wait = 1
        data = b"\x00" * 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        ret = module.FifoWritePend(self.pfifo, data, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        self.assertEqual(ret, True)
        # Calls
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class FifoPendRead(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.new("struct task_t[1]")
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testFifoEmptyZeroWait(self):
        # Task 1 pends
        wait = 0
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.FifoPendRead(self.pfifo, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testFifoEmpty(self):
        # Task 1 pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.FifoPendRead(self.pfifo, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.fifo.event.list_read, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.fifo.event.list_read, self.ptask1, wait)

    def testFifoNonEmpty(self):
        # Writes and succeeds
        self.writeOnFifo(1)
        # Task 1 pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.FifoPendRead(self.pfifo, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class FifoPendWrite(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Tasks
        self.ptask0 = ffi.cast("struct task_t*", 0)
        self.ptask1 = ffi.new("struct task_t[1]")
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Value
            self.assertEqual(ret, True)
        return data_total

    def testFifoFullZeroWait(self):
        # Writes and succeeds
        self.writeOnFifo(self.length)
        # Task 1 pends
        wait = 0
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.FifoPendWrite(self.pfifo, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

    def testFifoFull(self):
        # Writes and succeeds
        self.writeOnFifo(self.length)
        # Task 1 pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.FifoPendWrite(self.pfifo, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_called_once_with()
        Mocks.OSEventPrePendTask.assert_called_once_with(self.fifo.event.list_write, self.ptask1)
        Mocks.OSEventPendTask.assert_called_once_with(self.fifo.event.list_write, self.ptask1, wait)

    def testFifoNonFull(self):
        # Task 1 pends
        wait = 1
        # Simulate Task1 calling
        Mocks.GetCurrentTask.return_value = self.ptask1
        module.FifoPendWrite(self.pfifo, 1, wait)
        Mocks.GetCurrentTask.return_value = self.ptask0
        # Values
        Mocks.GetCurrentTask.assert_not_called()
        Mocks.OSEventPrePendTask.assert_not_called()
        Mocks.OSEventPendTask.assert_not_called()

class FifoUtils(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num, succeeds=True):
        data_total = b''
        # Writes and succeeds self.length times
        for i in range(num):
            # Writes and succeeds
            data = os.urandom(1)
            data_total += data
            ret = module.FifoWrite(self.pfifo, data, 1)
            # Value
            self.assertEqual(ret, succeeds)
        return data_total

    def testFifoUsed(self):
        self.assertEqual(module.FifoUsed(self.pfifo), 0)
        for i in range(1, self.length + 1):
            self.writeOnFifo(1)
            self.assertEqual(module.FifoUsed(self.pfifo), i)
        self.writeOnFifo(1, succeeds=False)
        self.assertEqual(module.FifoUsed(self.pfifo), self.length)

    def testFifoFree(self):
        self.assertEqual(module.FifoFree(self.pfifo), self.length)
        for i in range(1, self.length + 1):
            self.writeOnFifo(1)
            self.assertEqual(module.FifoFree(self.pfifo), self.length-i)
        self.writeOnFifo(1, succeeds=False)
        self.assertEqual(module.FifoFree(self.pfifo), 0)

    def testFifoLength(self):
        self.assertEqual(module.FifoLength(self.pfifo), self.length)

    def testFifoEmpty(self):
        self.assertEqual(module.FifoEmpty(self.pfifo), True)
        self.writeOnFifo(1)
        self.assertEqual(module.FifoEmpty(self.pfifo), False)

    def testFifoFull(self):
        self.assertEqual(module.FifoFull(self.pfifo), False)
        self.writeOnFifo(self.length)
        self.assertEqual(module.FifoFull(self.pfifo), True)

class FifoConcurrentAccess(unittest.TestCase):

    def setUp(self):
        # Fifo
        self.pfifo = ffi.new("struct fifo_t[1]")
        self.fifo = self.pfifo[0]
        # Initialize fifo
        self.length = 8
        self.buff = ffi.new("uint8_t [%d]" % (self.length, ))
        module.FifoInit(self.pfifo, self.buff, self.length)
        # Mocks
        Mocks.ResetMocks()

    def writeOnFifo(self, num, values=None, succeeds=True):
        data_total = b''
        if values == None:
            # Writes and succeeds self.length times
            for i in range(num):
                # Writes and succeeds
                data = os.urandom(1)
                data_total += data
                ret = module.FifoWrite(self.pfifo, data, 1)
                # Value
                self.assertEqual(ret, succeeds)
        else:
            data_total += values
            ret = module.FifoWrite(self.pfifo, values, num)
        return data_total

    def testFifoWrite(self):
        # Write and concurrent write
        def side_effect_write_fifo():
            Mocks.LibrertosTestConcurrentAccess.side_effect = None
            self.writeOnFifo(1, b"b")
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_write_fifo
        self.writeOnFifo(1, b"a")

        # Read from first and second (concurrent) write
        read_val = b"\x00" * 2
        ret = module.FifoRead(self.pfifo, read_val, 2)
        self.assertEqual(ret, 2)
        self.assertEqual(read_val, b"ab")

    def testFifoRead(self):
        # Write twice
        self.writeOnFifo(1, b"a")
        self.writeOnFifo(1, b"b")

        # Read and concurrent read

        def side_effect_read_fifo():
            Mocks.LibrertosTestConcurrentAccess.side_effect = None
            # Second read (concurrent)
            read_val = b"\x02" * 1
            ret = module.FifoRead(self.pfifo, read_val, 1)
            self.assertEqual(ret, 1)
            self.assertEqual(read_val, b"b")
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_read_fifo

        # First read
        read_val = b"\x03" * 1
        ret = module.FifoRead(self.pfifo, read_val, 1)
        self.assertEqual(ret, 1)
        self.assertEqual(read_val, b"a")

class FifoMultipleBytesReadWrite(unittest.TestCase):
    # TODO Read/Write multiple bytes to FIFO
    #
    # FifoRead() and FifoWrite() were tested with third argument equal only to
    # one.
    pass

################################################################################

if __name__ == '__main__':
  unittest.main()
