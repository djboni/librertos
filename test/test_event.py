#!/usr/bin/python3
import unittest
import os, random
from unit_test_c_with_python import load, FFIMocks

################################################################################
# Casted

def CastedFunc(*args, **kwargs):
    args = list(args)
    func = kwargs['func']
    del kwargs['func']
    for i in range(len(args)):
        cdata = str(args[i]).split("'", 3)
        if cdata[0] == '<cdata ':
            args[i] = ffi.cast(cdata[1], args[i])
    for key in kwargs:
        cdata = str(kwargs[key]).split("'", 3)
        if cdata[0] == '<cdata ':
            kwargs[key] = ffi.cast(cdata[1], kwargs[key])
    func(*args, **kwargs)

def Casted(func):
    return lambda *args, **kwargs: CastedFunc(*args, **kwargs, func=func)

################################################################################

LIBRERTOS_MAX_PRIORITY = 3
LIBRERTOS_PREEMPTION = 1
LIBRERTOS_PREEMPT_LIMIT = 0
LIBRERTOS_SOFTWARETIMERS = 1
LIBRERTOS_STATISTICS = 0
LIBRERTOS_STATE_GUARDS = 0

module_name = 'librertos_event_'

source_files = [
    '../source/librertos_event.c',
    '../source/librertos_state.c',
]

include_paths = [
    '.',
    '../include',
]

compiler_options = [
    '-std=c90',
    '-pedantic',
    '-DLIBRERTOS_MAX_PRIORITY=%d' % LIBRERTOS_MAX_PRIORITY,
    '-DLIBRERTOS_PREEMPTION=%d' % LIBRERTOS_PREEMPTION,
    '-DLIBRERTOS_PREEMPT_LIMIT=%d' % LIBRERTOS_PREEMPT_LIMIT,
    '-DLIBRERTOS_SOFTWARETIMERS=%d' % LIBRERTOS_SOFTWARETIMERS,
    '-DLIBRERTOS_STATISTICS=%d' % LIBRERTOS_STATISTICS,
    '-DLIBRERTOS_STATE_GUARDS=%d' % LIBRERTOS_STATE_GUARDS,
]

module, ffi = load(source_files, include_paths, compiler_options, module_name=module_name, en_code_coverage=True)

################################################################################
# Test imports

import test_list

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
Mocks.CreateMock(ffi, 'TaskDelay', return_value=None)

# librertos_list.c
Mocks.CreateMock(ffi, 'OSListHeadInit', wraps=test_list.Casted(test_list.module.OSListHeadInit))
Mocks.CreateMock(ffi, 'OSListNodeInit', wraps=test_list.Casted(test_list.module.OSListNodeInit))
Mocks.CreateMock(ffi, 'OSListInsert', wraps=test_list.Casted(test_list.module.OSListInsert))
Mocks.CreateMock(ffi, 'OSListInsertAfter', wraps=test_list.Casted(test_list.module.OSListInsertAfter))
Mocks.CreateMock(ffi, 'OSListRemove', wraps=test_list.Casted(test_list.module.OSListRemove))

################################################################################

class OSEventRInit(unittest.TestCase):

    def setUp(self):
        # Event
        self.pevent = ffi.new("struct event_r_t[1]")
        self.event = self.pevent[0]
        self.pevent_read = ffi.addressof(self.event.list_read)
        # Mocks
        Mocks.ResetMocks()

    def testCall(self):
        module.OSEventRInit(self.pevent)
        # Calls
        Mocks.OSListHeadInit.assert_called_once_with(self.pevent_read)
        # Values
        self.assertEqual(self.event.list_read.head_ptr, self.pevent_read)
        self.assertEqual(self.event.list_read.tail_ptr, self.pevent_read)
        self.assertEqual(self.event.list_read.length, 0)

class OSEventRwInit(unittest.TestCase):

    def setUp(self):
        # Event
        self.pevent = ffi.new("struct event_rw_t[1]")
        self.event = self.pevent[0]
        self.pevent_read = ffi.addressof(self.event.list_read)
        self.pevent_write = ffi.addressof(self.event.list_write)
        # Mocks
        Mocks.ResetMocks()

    def testCall(self):
        module.OSEventRwInit(self.pevent)
        # Calls
        called_mock = Mocks.OSListHeadInit
        expected_calls = [
            Mocks.call(self.pevent_read),
            Mocks.call(self.pevent_write),
        ]
        called_mock.assert_has_calls(expected_calls)
        self.assertEqual(called_mock.call_count, len(expected_calls))
        # Values
        self.assertEqual(self.event.list_read.head_ptr, self.pevent_read)
        self.assertEqual(self.event.list_read.tail_ptr, self.pevent_read)
        self.assertEqual(self.event.list_read.length, 0)
        self.assertEqual(self.event.list_write.head_ptr, self.pevent_write)
        self.assertEqual(self.event.list_write.tail_ptr, self.pevent_write)
        self.assertEqual(self.event.list_write.length, 0)

class OSEventPrePendTask(unittest.TestCase):

    def setUp(self):
        # List
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        module.OSListHeadInit(self.plist)

        # Task 1
        self.ptask1 = ffi.new("struct task_t[1]")
        self.task1 = self.ptask1[0]
        # Nodes
        self.ptask1_node_delay = ffi.addressof(self.task1.node_delay)
        self.ptask1_node_event = ffi.addressof(self.task1.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask1_node_delay, self.ptask1)
        module.OSListNodeInit(self.ptask1_node_event, self.ptask1)
        self.task1.priority = 1

        # Task 2
        self.ptask2 = ffi.new("struct task_t[1]")
        self.task2 = self.ptask2[0]
        # Nodes
        self.ptask2_node_delay = ffi.addressof(self.task2.node_delay)
        self.ptask2_node_event = ffi.addressof(self.task2.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask2_node_delay, self.ptask2)
        module.OSListNodeInit(self.ptask2_node_event, self.ptask2)
        self.task2.priority = 2

        Mocks.ResetMocks()

    def testInsertFirst(self):
        module.OSEventPrePendTask(self.plist, self.ptask1)
        # Values
        # List
        self.assertEqual(self.list_.head_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.tail_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.length, 1)
        # Task1
        self.assertEqual(self.task1.node_event.next_ptr, self.plist)
        self.assertEqual(self.task1.node_event.prev_ptr, self.plist)
        self.assertEqual(self.task1.node_event.list_ptr, self.plist)
        self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
        #self.assertEqual(self.task1.node_event.value, ?)

    def testInsertSecondInHead(self):
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPrePendTask(self.plist, self.ptask2)
        # Values
        # List
        self.assertEqual(self.list_.head_ptr, self.ptask2_node_event)
        self.assertEqual(self.list_.tail_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.length, 2)
        # Task1
        self.assertEqual(self.task1.node_event.next_ptr, self.plist)
        self.assertEqual(self.task1.node_event.prev_ptr, self.ptask2_node_event)
        self.assertEqual(self.task1.node_event.list_ptr, self.plist)
        self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
        #self.assertEqual(self.task1.node_event.value, ?)
        # Task2
        self.assertEqual(self.task2.node_event.next_ptr, self.ptask1_node_event)
        self.assertEqual(self.task2.node_event.prev_ptr, self.plist)
        self.assertEqual(self.task2.node_event.list_ptr, self.plist)
        self.assertEqual(self.task2.node_event.owner_ptr, self.ptask2)
        #self.assertEqual(self.task2.node_event.value, ?)

class OSEventPendTask(unittest.TestCase):

    def setUp(self):
        if __name__ != '__main__':
            module.LibrertosInit()
            module.LibrertosStart()

        # List
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        module.OSListHeadInit(self.plist)

        # Task 1
        self.ptask1 = ffi.new("struct task_t[1]")
        self.task1 = self.ptask1[0]
        # Nodes
        self.ptask1_node_delay = ffi.addressof(self.task1.node_delay)
        self.ptask1_node_event = ffi.addressof(self.task1.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask1_node_delay, self.ptask1)
        module.OSListNodeInit(self.ptask1_node_event, self.ptask1)
        self.task1.priority = 1

        # Task 2
        self.ptask2 = ffi.new("struct task_t[1]")
        self.task2 = self.ptask2[0]
        # Nodes
        self.ptask2_node_delay = ffi.addressof(self.task2.node_delay)
        self.ptask2_node_event = ffi.addressof(self.task2.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask2_node_delay, self.ptask2)
        module.OSListNodeInit(self.ptask2_node_event, self.ptask2)
        self.task2.priority = 2

        # Task 3
        self.ptask3 = ffi.new("struct task_t[1]")
        self.task3 = self.ptask3[0]
        # Nodes
        self.ptask3_node_delay = ffi.addressof(self.task3.node_delay)
        self.ptask3_node_event = ffi.addressof(self.task3.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask3_node_delay, self.ptask3)
        module.OSListNodeInit(self.ptask3_node_event, self.ptask3)
        self.task3.priority = 3

        # Task 4
        self.ptask4 = ffi.new("struct task_t[1]")
        self.task4 = self.ptask4[0]
        # Nodes
        self.ptask4_node_delay = ffi.addressof(self.task4.node_delay)
        self.ptask4_node_event = ffi.addressof(self.task4.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask4_node_delay, self.ptask4)
        module.OSListNodeInit(self.ptask4_node_event, self.ptask4)
        self.task4.priority = 4

        Mocks.ResetMocks()

    def testInsertFirstSuspend(self):
        ticks_to_wait = ffi.cast("tick_t", -1)
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
        # Values
        # List
        self.assertEqual(self.list_.head_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.tail_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.length, 1)
        # Task1
        self.assertEqual(self.task1.node_event.next_ptr, self.plist)
        self.assertEqual(self.task1.node_event.prev_ptr, self.plist)
        self.assertEqual(self.task1.node_event.list_ptr, self.plist)
        self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
        #self.assertEqual(self.task1.node_event.value, ?)
        # Calls
        if __name__ == '__main__':
            Mocks.TaskDelay.assert_not_called()

    def testInsertFirstDelay(self):
        ticks_to_wait = 1
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
        # Values
        # List
        self.assertEqual(self.list_.head_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.tail_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.length, 1)
        # Task1
        self.assertEqual(self.task1.node_event.next_ptr, self.plist)
        self.assertEqual(self.task1.node_event.prev_ptr, self.plist)
        self.assertEqual(self.task1.node_event.list_ptr, self.plist)
        self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
        #self.assertEqual(self.task1.node_event.value, ?)
        # Calls
        if __name__ == '__main__':
            Mocks.TaskDelay.assert_called_once_with(ticks_to_wait)

    def testInsertSecond(self):
        ticks_to_wait = ffi.cast("tick_t", -1)
        for task_order in ( # Priority
                (1, 2),     # low, high
                (2, 1),     # high, low
        ):
            self.setUp()
            for task_num in task_order:
                if task_num == 1:
                    module.OSEventPrePendTask(self.plist, self.ptask1)
                    module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
                elif task_num == 2:
                    module.OSEventPrePendTask(self.plist, self.ptask2)
                    module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)
                else:
                    raise Exception("Invalid task_order")
        # Values
        # List
        self.assertEqual(self.list_.head_ptr, self.ptask1_node_event)
        self.assertEqual(self.list_.tail_ptr, self.ptask2_node_event)
        self.assertEqual(self.list_.length, 2)
        # Task1
        self.assertEqual(self.task1.node_event.next_ptr, self.ptask2_node_event)
        self.assertEqual(self.task1.node_event.prev_ptr, self.plist)
        self.assertEqual(self.task1.node_event.list_ptr, self.plist)
        self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
        #self.assertEqual(self.task1.node_event.value, ?)
        # Task2
        self.assertEqual(self.task2.node_event.next_ptr, self.plist)
        self.assertEqual(self.task2.node_event.prev_ptr, self.ptask1_node_event)
        self.assertEqual(self.task2.node_event.list_ptr, self.plist)
        self.assertEqual(self.task2.node_event.owner_ptr, self.ptask2)
        #self.assertEqual(self.task2.node_event.value, ?)

    def testInsertThird(self):
        ticks_to_wait = ffi.cast("tick_t", -1)
        for task_order in ( # Priority
                (1, 2, 3),  # low, medium, high
                (1, 3, 2),  # low, high, medium
                (2, 1, 3),  # medium, low, high
                (2, 3, 1),  # medium, high, low
                (3, 1, 2),  # high, low, medium
                (3, 2, 1),  # high, medium, low
        ):
            self.setUp()
            for task_num in task_order:
                if task_num == 1:
                    module.OSEventPrePendTask(self.plist, self.ptask1)
                    module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
                elif task_num == 2:
                    module.OSEventPrePendTask(self.plist, self.ptask2)
                    module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)
                elif task_num == 3:
                    module.OSEventPrePendTask(self.plist, self.ptask3)
                    module.OSEventPendTask(self.plist, self.ptask3, ticks_to_wait)
                else:
                    raise Exception("Invalid task_order")
            # Values
            # List
            self.assertEqual(self.list_.head_ptr, self.ptask1_node_event)
            self.assertEqual(self.list_.tail_ptr, self.ptask3_node_event)
            self.assertEqual(self.list_.length, 3)
            # Task1
            self.assertEqual(self.task1.node_event.next_ptr, self.ptask2_node_event)
            self.assertEqual(self.task1.node_event.prev_ptr, self.plist)
            self.assertEqual(self.task1.node_event.list_ptr, self.plist)
            self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
            #self.assertEqual(self.task1.node_event.value, ?)
            # Task2
            self.assertEqual(self.task2.node_event.next_ptr, self.ptask3_node_event)
            self.assertEqual(self.task2.node_event.prev_ptr, self.ptask1_node_event)
            self.assertEqual(self.task2.node_event.list_ptr, self.plist)
            self.assertEqual(self.task2.node_event.owner_ptr, self.ptask2)
            #self.assertEqual(self.task2.node_event.value, ?)
            # Task3
            self.assertEqual(self.task3.node_event.next_ptr, self.plist)
            self.assertEqual(self.task3.node_event.prev_ptr, self.ptask2_node_event)
            self.assertEqual(self.task3.node_event.list_ptr, self.plist)
            self.assertEqual(self.task3.node_event.owner_ptr, self.ptask3)
            #self.assertEqual(self.task3.node_event.value, ?)

    def testInsertFourth(self):
        ticks_to_wait = ffi.cast("tick_t", -1)
        for task_order in ( # Priority
                (1, 2, 3, 4),
                (1, 2, 4, 3),
                (1, 3, 2, 4),
                (1, 3, 4, 2),
                (1, 4, 2, 3),
                (1, 4, 3, 2),

                (2, 1, 3, 4),
                (2, 1, 4, 3),
                (2, 3, 1, 4),
                (2, 3, 4, 1),
                (2, 4, 1, 3),
                (2, 4, 3, 1),

                (3, 1, 2, 4),
                (3, 1, 4, 2),
                (3, 2, 1, 4),
                (3, 2, 4, 1),
                (3, 4, 1, 2),
                (3, 4, 2, 1),

                (4, 1, 2, 3),
                (4, 1, 3, 2),
                (4, 2, 1, 3),
                (4, 2, 3, 1),
                (4, 3, 1, 2),
                (4, 3, 2, 1),
        ):
            self.setUp()
            for task_num in task_order:
                if task_num == 1:
                    module.OSEventPrePendTask(self.plist, self.ptask1)
                    module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
                elif task_num == 2:
                    module.OSEventPrePendTask(self.plist, self.ptask2)
                    module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)
                elif task_num == 3:
                    module.OSEventPrePendTask(self.plist, self.ptask3)
                    module.OSEventPendTask(self.plist, self.ptask3, ticks_to_wait)
                elif task_num == 4:
                    module.OSEventPrePendTask(self.plist, self.ptask4)
                    module.OSEventPendTask(self.plist, self.ptask4, ticks_to_wait)
                else:
                    raise Exception("Invalid task_order")
            # Values
            # List
            self.assertEqual(self.list_.head_ptr, self.ptask1_node_event)
            self.assertEqual(self.list_.tail_ptr, self.ptask4_node_event)
            self.assertEqual(self.list_.length, 4)
            # Task1
            self.assertEqual(self.task1.node_event.next_ptr, self.ptask2_node_event)
            self.assertEqual(self.task1.node_event.prev_ptr, self.plist)
            self.assertEqual(self.task1.node_event.list_ptr, self.plist)
            self.assertEqual(self.task1.node_event.owner_ptr, self.ptask1)
            #self.assertEqual(self.task1.node_event.value, ?)
            # Task2
            self.assertEqual(self.task2.node_event.next_ptr, self.ptask3_node_event)
            self.assertEqual(self.task2.node_event.prev_ptr, self.ptask1_node_event)
            self.assertEqual(self.task2.node_event.list_ptr, self.plist)
            self.assertEqual(self.task2.node_event.owner_ptr, self.ptask2)
            #self.assertEqual(self.task2.node_event.value, ?)
            # Task3
            self.assertEqual(self.task3.node_event.next_ptr, self.ptask4_node_event)
            self.assertEqual(self.task3.node_event.prev_ptr, self.ptask2_node_event)
            self.assertEqual(self.task3.node_event.list_ptr, self.plist)
            self.assertEqual(self.task3.node_event.owner_ptr, self.ptask3)
            #self.assertEqual(self.task3.node_event.value, ?)
            # Task4
            self.assertEqual(self.task4.node_event.next_ptr, self.plist)
            self.assertEqual(self.task4.node_event.prev_ptr, self.ptask3_node_event)
            self.assertEqual(self.task4.node_event.list_ptr, self.plist)
            self.assertEqual(self.task4.node_event.owner_ptr, self.ptask4)
            #self.assertEqual(self.task4.node_event.value, ?)

class OSEventUnblockTasks(unittest.TestCase):

    def setUp(self):
        # Simulate LibrertosInit()
        self.ppending_ready_task_list = ffi.addressof(module.OS_State.pending_ready_task_list)
        self.pending_ready_task_list = self.ppending_ready_task_list[0]
        module.OSListHeadInit(self.ppending_ready_task_list)
        # List
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        module.OSListHeadInit(self.plist)

        # Task 1
        self.ptask1 = ffi.new("struct task_t[1]")
        self.task1 = self.ptask1[0]
        # Nodes
        self.ptask1_node_delay = ffi.addressof(self.task1.node_delay)
        self.ptask1_node_event = ffi.addressof(self.task1.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask1_node_delay, self.ptask1)
        module.OSListNodeInit(self.ptask1_node_event, self.ptask1)
        self.task1.priority = 1

        # Task 2
        self.ptask2 = ffi.new("struct task_t[1]")
        self.task2 = self.ptask2[0]
        # Nodes
        self.ptask2_node_delay = ffi.addressof(self.task2.node_delay)
        self.ptask2_node_event = ffi.addressof(self.task2.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask2_node_delay, self.ptask2)
        module.OSListNodeInit(self.ptask2_node_event, self.ptask2)
        self.task2.priority = 2

        Mocks.ResetMocks()

    def testNoTaskWaiting(self):
        module.OSEventUnblockTasks(self.plist)
        # Values
        # List
        self.assertEqual(self.list_.head_ptr, self.plist)
        self.assertEqual(self.list_.tail_ptr, self.plist)
        self.assertEqual(self.list_.length, 0)

    def testOneTaskWaiting(self):
        ticks_to_wait = ffi.cast("tick_t", -1)
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
        module.OSEventUnblockTasks(self.plist)
        # Values
        # List
        self.assertEqual(self.list_.length, 0)
        self.assertEqual(self.list_.head_ptr, self.plist)
        self.assertEqual(self.list_.tail_ptr, self.plist)
        self.assertEqual(self.pending_ready_task_list.length, 1)
        self.assertEqual(self.pending_ready_task_list.head_ptr, self.task1.node_event)
        self.assertEqual(self.pending_ready_task_list.tail_ptr, self.task1.node_event)

    def testTwoTaskWaiting(self):
        ticks_to_wait = ffi.cast("tick_t", -1)
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)
        module.OSEventPrePendTask(self.plist, self.ptask2)
        module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)
        module.OSEventUnblockTasks(self.plist)
        # Values
        # List
        self.assertEqual(self.list_.length, 1)
        self.assertEqual(self.list_.head_ptr, self.task1.node_event)
        self.assertEqual(self.list_.tail_ptr, self.task1.node_event)
        self.assertEqual(self.pending_ready_task_list.length, 1)
        self.assertEqual(self.pending_ready_task_list.head_ptr, self.task2.node_event)
        self.assertEqual(self.pending_ready_task_list.tail_ptr, self.task2.node_event)
        #
        module.OSEventUnblockTasks(self.plist)
        # Values
        # List
        self.assertEqual(self.list_.length, 0)
        self.assertEqual(self.list_.head_ptr, self.plist)
        self.assertEqual(self.list_.tail_ptr, self.plist)
        self.assertEqual(self.pending_ready_task_list.length, 2)
        self.assertEqual(self.pending_ready_task_list.head_ptr, self.task2.node_event)
        self.assertEqual(self.pending_ready_task_list.tail_ptr, self.task1.node_event)

class EventConcurrentAccess(unittest.TestCase):

    def setUp(self):
        # Simulate LibrertosInit()
        self.ppending_ready_task_list = ffi.addressof(module.OS_State.pending_ready_task_list)
        self.pending_ready_task_list = self.ppending_ready_task_list[0]
        module.OSListHeadInit(self.ppending_ready_task_list)
        # List
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        module.OSListHeadInit(self.plist)

        # Task 1
        self.ptask1 = ffi.new("struct task_t[1]")
        self.task1 = self.ptask1[0]
        # Nodes
        self.ptask1_node_delay = ffi.addressof(self.task1.node_delay)
        self.ptask1_node_event = ffi.addressof(self.task1.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask1_node_delay, self.ptask1)
        module.OSListNodeInit(self.ptask1_node_event, self.ptask1)
        self.task1.priority = 1

        # Task 2
        self.ptask2 = ffi.new("struct task_t[1]")
        self.task2 = self.ptask2[0]
        # Nodes
        self.ptask2_node_delay = ffi.addressof(self.task2.node_delay)
        self.ptask2_node_event = ffi.addressof(self.task2.node_event)
        # Simulate LibrertosTaskCreate()
        module.OSListNodeInit(self.ptask2_node_delay, self.ptask2)
        module.OSListNodeInit(self.ptask2_node_event, self.ptask2)
        self.task2.priority = 2

        Mocks.ResetMocks()

    def testRemoveCurrentTaskFromList(self):
        ticks_to_wait = ffi.cast("tick_t", -1)

        # Task2
        module.OSEventPrePendTask(self.plist, self.ptask2)
        module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)

        # Concurrent access removes task from list
        def side_effect_remove_task():
            if Mocks.LibrertosTestConcurrentAccess.call_count == 1:
                Mocks.LibrertosTestConcurrentAccess.side_effect = None
                #module.OSEventUnblockTasks(self.plist)
                module.OSListRemove(self.list_.head_ptr)
        Mocks.LibrertosTestConcurrentAccess.call_count = 0
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_remove_task

        # Task1 with concurrent access
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)

        # Values
        self.assertEqual(self.list_.length, 1)
        self.assertEqual(self.list_.head_ptr, self.task2.node_event)
        self.assertEqual(self.list_.tail_ptr, self.task2.node_event)

    def testRemoveCurrentPositionFromList(self):
        ticks_to_wait = ffi.cast("tick_t", -1)

        # Task2
        module.OSEventPrePendTask(self.plist, self.ptask2)
        module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)

        # Concurrent access removes task from list
        def side_effect_remove_task():
            if Mocks.LibrertosTestConcurrentAccess.call_count == 1:
                Mocks.LibrertosTestConcurrentAccess.side_effect = None
                #module.OSEventUnblockTasks(self.plist)
                module.OSListRemove(self.list_.tail_ptr)
        Mocks.LibrertosTestConcurrentAccess.call_count = 0
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_remove_task

        # Task2 with concurrent access
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)

        # Values
        self.assertEqual(self.list_.length, 1)
        self.assertEqual(self.list_.head_ptr, self.task1.node_event)
        self.assertEqual(self.list_.tail_ptr, self.task1.node_event)

    def testRemoveAllFromList(self):
        ticks_to_wait = ffi.cast("tick_t", -1)

        # Task2
        module.OSEventPrePendTask(self.plist, self.ptask2)
        module.OSEventPendTask(self.plist, self.ptask2, ticks_to_wait)

        # Concurrent access removes task from list
        def side_effect_remove_task():
            if Mocks.LibrertosTestConcurrentAccess.call_count == 1:
                Mocks.LibrertosTestConcurrentAccess.side_effect = None
                #module.OSEventUnblockTasks(self.plist)
                module.OSListRemove(self.list_.tail_ptr)
                module.OSListRemove(self.list_.tail_ptr)
        Mocks.LibrertosTestConcurrentAccess.call_count = 0
        Mocks.LibrertosTestConcurrentAccess.side_effect = side_effect_remove_task

        # Task1 with concurrent access
        module.OSEventPrePendTask(self.plist, self.ptask1)
        module.OSEventPendTask(self.plist, self.ptask1, ticks_to_wait)

        # Values
        self.assertEqual(self.list_.length, 0)
        self.assertEqual(self.list_.head_ptr, self.plist)
        self.assertEqual(self.list_.tail_ptr, self.plist)

################################################################################

if __name__ == '__main__':
    unittest.main()
