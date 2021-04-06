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

module_name = 'librertos_list_'

source_files = [
    '../source/librertos_list.c',
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

####################################################################################

####################################################################################
# Undefinded functions

Mocks = FFIMocks()

# projdefs.h
Mocks.CreateMock(ffi, 'InterruptsEnable', return_value=None)
Mocks.CreateMock(ffi, 'InterruptsDisable', return_value=None)
Mocks.CreateMock(ffi, 'CriticalVal', return_value=None)
Mocks.CreateMock(ffi, 'CriticalEnter', return_value=None)
Mocks.CreateMock(ffi, 'CriticalExit', return_value=None)
Mocks.CreateMock(ffi, 'LibrertosTestConcurrentAccess', return_value=None)

####################################################################################

class OSListHeadInit(unittest.TestCase):

    def testInit(self):
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        module.OSListHeadInit(self.plist)

        self.assertEqual(self.list_.head_ptr, self.plist)
        self.assertEqual(self.list_.tail_ptr, self.plist)
        self.assertEqual(self.list_.length, 0)

class OSListNodeInit(unittest.TestCase):

    def testInit(self):
        # Task
        self.ptask = ffi.cast("struct task_t*", 1)
        # List
        self.plist0 = ffi.cast("struct task_head_list_t*", 0)
        # Node
        self.pnode0 = ffi.cast("struct task_list_node_t*", 0)
        self.pnode = ffi.new("struct task_list_node_t[1]")
        self.node = self.pnode[0]

        module.OSListNodeInit(self.pnode, self.ptask)

        self.assertEqual(self.node.next_ptr, self.pnode0)
        self.assertEqual(self.node.prev_ptr, self.pnode0)
        self.assertEqual(self.node.value, 0)
        self.assertEqual(self.node.list_ptr, self.plist0)
        self.assertEqual(self.node.owner_ptr, self.ptask)

class OSListInsert(unittest.TestCase):

    def setUp(self):
        # List
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        module.OSListHeadInit(self.plist)
        # Tasks
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        # Nodes
        self.pnode1 = ffi.new("struct task_list_node_t[1]")
        self.pnode2 = ffi.new("struct task_list_node_t[1]")
        self.node1 = self.pnode1[0]
        self.node2 = self.pnode2[0]
        module.OSListNodeInit(self.pnode1, self.ptask1)
        module.OSListNodeInit(self.pnode2, self.ptask2)

    def testInsertFirst(self):
        value1 = 100
        module.OSListInsert(self.plist, self.pnode1, value1)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.plist)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.value, value1)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode1)
        self.assertEqual(self.list_.length, 1)

    def testInsertAfterFirstIfLess(self):
        value1 = 100
        value2 = 110
        module.OSListInsert(self.plist, self.pnode1, value1)
        module.OSListInsert(self.plist, self.pnode2, value2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode2)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.value, value1)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.plist)
        self.assertEqual(self.node2.prev_ptr, self.pnode1)
        self.assertEqual(self.node2.value, value2)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode2)
        self.assertEqual(self.list_.length, 2)

    def testInsertAfterFirstIfEqual(self):
        value1 = 100
        value2 = 100
        module.OSListInsert(self.plist, self.pnode1, value1)
        module.OSListInsert(self.plist, self.pnode2, value2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode2)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.value, value1)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.plist)
        self.assertEqual(self.node2.prev_ptr, self.pnode1)
        self.assertEqual(self.node2.value, value2)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode2)
        self.assertEqual(self.list_.length, 2)

    def testInsertBeforeFirstIfGreater(self):
        value1 = 100
        value2 = 90
        module.OSListInsert(self.plist, self.pnode1, value1)
        module.OSListInsert(self.plist, self.pnode2, value2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.plist)
        self.assertEqual(self.node1.prev_ptr, self.pnode2)
        self.assertEqual(self.node1.value, value1)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode1)
        self.assertEqual(self.node2.prev_ptr, self.plist)
        self.assertEqual(self.node2.value, value2)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode2)
        self.assertEqual(self.list_.tail_ptr, self.pnode1)
        self.assertEqual(self.list_.length, 2)

    def testInsertRandom(self):
        max_rand = 256 # time_t
        num_nodes = 255 # uint8_t
        pnode = ffi.new("struct task_list_node_t[%d]" % num_nodes)

        # Create nodes with random values
        for i in range(num_nodes):
            value = random.randint(0, max_rand)
            ptask = ffi.cast("struct task_t*", i+1)
            module.OSListNodeInit(pnode + i, ptask)
            module.OSListInsert(self.plist, pnode + i, value)

        # The linked list must be organized
        prev_value = max_rand
        for i in range(num_nodes):
            curr_node = pnode[i]
            self.assertGreaterEqual(prev_value, curr_node.value)
            self.assertEqual(curr_node.next_ptr[0].prev_ptr, curr_node)
            self.assertEqual(curr_node.prev_ptr[0].next_ptr, curr_node)

        # List
        self.assertEqual(self.list_.length, num_nodes)

class OSListInsertAfter(unittest.TestCase):

    def setUp(self):
        # List
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        self.plistnode = ffi.cast("struct task_list_node_t*", self.plist)
        module.OSListHeadInit(self.plist)
        # Tasks
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        self.ptask3 = ffi.cast("struct task_t*", 3)
        # Nodes
        self.pnode1 = ffi.new("struct task_list_node_t[1]")
        self.pnode2 = ffi.new("struct task_list_node_t[1]")
        self.pnode3 = ffi.new("struct task_list_node_t[1]")
        self.node1 = self.pnode1[0]
        self.node2 = self.pnode2[0]
        self.node3 = self.pnode3[0]
        module.OSListNodeInit(self.pnode1, self.ptask1)
        module.OSListNodeInit(self.pnode2, self.ptask2)
        module.OSListNodeInit(self.pnode3, self.ptask3)

    def testInsertFirst(self):
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode1)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.plist)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode1)
        self.assertEqual(self.list_.length, 1)

    def testInsertAfterFirst(self):
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode1)
        module.OSListInsertAfter(self.plist, self.pnode1, self.pnode2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode2)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.plist)
        self.assertEqual(self.node2.prev_ptr, self.pnode1)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode2)
        self.assertEqual(self.list_.length, 2)

    def testInsertBeforeFirst(self):
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode1)
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.plist)
        self.assertEqual(self.node1.prev_ptr, self.pnode2)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode1)
        self.assertEqual(self.node2.prev_ptr, self.plist)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode2)
        self.assertEqual(self.list_.tail_ptr, self.pnode1)
        self.assertEqual(self.list_.length, 2)

    def testInsertBetweenFirstAndSecond(self):
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode1)
        module.OSListInsertAfter(self.plist, self.pnode1, self.pnode2)
        module.OSListInsertAfter(self.plist, self.pnode1, self.pnode3)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode3)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.plist)
        self.assertEqual(self.node2.prev_ptr, self.pnode3)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.pnode2)
        self.assertEqual(self.node3.prev_ptr, self.pnode1)
        self.assertEqual(self.node3.list_ptr, self.plist)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode2)
        self.assertEqual(self.list_.length, 3)

    def testInsertAfterSecond(self):
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode1)
        module.OSListInsertAfter(self.plist, self.pnode1, self.pnode2)
        module.OSListInsertAfter(self.plist, self.pnode2, self.pnode3)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode2)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode3)
        self.assertEqual(self.node2.prev_ptr, self.pnode1)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.plist)
        self.assertEqual(self.node3.prev_ptr, self.pnode2)
        self.assertEqual(self.node3.list_ptr, self.plist)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode3)
        self.assertEqual(self.list_.length, 3)

class OSListRemove(unittest.TestCase):

    def setUp(self):
        # List
        self.plist0 = ffi.cast("struct task_head_list_t*", 0)
        self.plist = ffi.new("struct task_head_list_t[1]")
        self.list_ = self.plist[0]
        self.plistnode = ffi.cast("struct task_list_node_t*", self.plist)
        module.OSListHeadInit(self.plist)
        # Tasks
        self.ptask1 = ffi.cast("struct task_t*", 1)
        self.ptask2 = ffi.cast("struct task_t*", 2)
        self.ptask3 = ffi.cast("struct task_t*", 3)
        # Nodes
        self.pnode0 = ffi.cast("struct task_list_node_t*", 0)
        self.pnode1 = ffi.new("struct task_list_node_t[1]")
        self.pnode2 = ffi.new("struct task_list_node_t[1]")
        self.pnode3 = ffi.new("struct task_list_node_t[1]")
        self.node1 = self.pnode1[0]
        self.node2 = self.pnode2[0]
        self.node3 = self.pnode3[0]
        module.OSListNodeInit(self.pnode1, self.ptask1)
        module.OSListNodeInit(self.pnode2, self.ptask2)
        module.OSListNodeInit(self.pnode3, self.ptask3)
        # Insert H -> N1 -> N2 -> N3 -> T
        module.OSListInsertAfter(self.plist, self.plistnode, self.pnode1)
        module.OSListInsertAfter(self.plist, self.pnode1, self.pnode2)
        module.OSListInsertAfter(self.plist, self.pnode2, self.pnode3)

    def testNotRemoving(self):
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode2)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode3)
        self.assertEqual(self.node2.prev_ptr, self.pnode1)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.plist)
        self.assertEqual(self.node3.prev_ptr, self.pnode2)
        self.assertEqual(self.node3.list_ptr, self.plist)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode3)
        self.assertEqual(self.list_.length, 3)

    def testRemoveFirstOfTrhee(self):
        module.OSListRemove(self.pnode1)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode0)
        self.assertEqual(self.node1.prev_ptr, self.pnode0)
        self.assertEqual(self.node1.list_ptr, self.plist0)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode3)
        self.assertEqual(self.node2.prev_ptr, self.plist)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.plist)
        self.assertEqual(self.node3.prev_ptr, self.pnode2)
        self.assertEqual(self.node3.list_ptr, self.plist)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode2)
        self.assertEqual(self.list_.tail_ptr, self.pnode3)
        self.assertEqual(self.list_.length, 2)

    def testRemoveSecondOfThree(self):
        module.OSListRemove(self.pnode2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode3)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode0)
        self.assertEqual(self.node2.prev_ptr, self.pnode0)
        self.assertEqual(self.node2.list_ptr, self.plist0)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.plist)
        self.assertEqual(self.node3.prev_ptr, self.pnode1)
        self.assertEqual(self.node3.list_ptr, self.plist)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode3)
        self.assertEqual(self.list_.length, 2)

    def testRemoveThirdOfThree(self):
        module.OSListRemove(self.pnode3)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode2)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.plist)
        self.assertEqual(self.node2.prev_ptr, self.pnode1)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.pnode0)
        self.assertEqual(self.node3.prev_ptr, self.pnode0)
        self.assertEqual(self.node3.list_ptr, self.plist0)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode2)
        self.assertEqual(self.list_.length, 2)

    def testRemoveFirstAndSecondOfThree(self):
        module.OSListRemove(self.pnode1)
        module.OSListRemove(self.pnode2)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode0)
        self.assertEqual(self.node1.prev_ptr, self.pnode0)
        self.assertEqual(self.node1.list_ptr, self.plist0)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode0)
        self.assertEqual(self.node2.prev_ptr, self.pnode0)
        self.assertEqual(self.node2.list_ptr, self.plist0)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.plist)
        self.assertEqual(self.node3.prev_ptr, self.plist)
        self.assertEqual(self.node3.list_ptr, self.plist)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode3)
        self.assertEqual(self.list_.tail_ptr, self.pnode3)
        self.assertEqual(self.list_.length, 1)

    def testRemoveSecondAndThirdOfThree(self):
        module.OSListRemove(self.pnode2)
        module.OSListRemove(self.pnode3)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.plist)
        self.assertEqual(self.node1.prev_ptr, self.plist)
        self.assertEqual(self.node1.list_ptr, self.plist)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode0)
        self.assertEqual(self.node2.prev_ptr, self.pnode0)
        self.assertEqual(self.node2.list_ptr, self.plist0)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.pnode0)
        self.assertEqual(self.node3.prev_ptr, self.pnode0)
        self.assertEqual(self.node3.list_ptr, self.plist0)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode1)
        self.assertEqual(self.list_.tail_ptr, self.pnode1)
        self.assertEqual(self.list_.length, 1)

    def testRemoveFirstAndThirdOfThree(self):
        module.OSListRemove(self.pnode1)
        module.OSListRemove(self.pnode3)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode0)
        self.assertEqual(self.node1.prev_ptr, self.pnode0)
        self.assertEqual(self.node1.list_ptr, self.plist0)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.plist)
        self.assertEqual(self.node2.prev_ptr, self.plist)
        self.assertEqual(self.node2.list_ptr, self.plist)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.pnode0)
        self.assertEqual(self.node3.prev_ptr, self.pnode0)
        self.assertEqual(self.node3.list_ptr, self.plist0)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.pnode2)
        self.assertEqual(self.list_.tail_ptr, self.pnode2)
        self.assertEqual(self.list_.length, 1)

    def testRemoveAllThree(self):
        module.OSListRemove(self.pnode1)
        module.OSListRemove(self.pnode2)
        module.OSListRemove(self.pnode3)
        # Node1
        self.assertEqual(self.node1.next_ptr, self.pnode0)
        self.assertEqual(self.node1.prev_ptr, self.pnode0)
        self.assertEqual(self.node1.list_ptr, self.plist0)
        self.assertEqual(self.node1.value, 0)
        self.assertEqual(self.node1.owner_ptr, self.ptask1)
        # Node2
        self.assertEqual(self.node2.next_ptr, self.pnode0)
        self.assertEqual(self.node2.prev_ptr, self.pnode0)
        self.assertEqual(self.node2.list_ptr, self.plist0)
        self.assertEqual(self.node2.value, 0)
        self.assertEqual(self.node2.owner_ptr, self.ptask2)
        # Node3
        self.assertEqual(self.node3.next_ptr, self.pnode0)
        self.assertEqual(self.node3.prev_ptr, self.pnode0)
        self.assertEqual(self.node3.list_ptr, self.plist0)
        self.assertEqual(self.node3.value, 0)
        self.assertEqual(self.node3.owner_ptr, self.ptask3)
        # List
        self.assertEqual(self.list_.head_ptr, self.plist)
        self.assertEqual(self.list_.tail_ptr, self.plist)
        self.assertEqual(self.list_.length, 0)

if __name__ == '__main__':
    unittest.main()
