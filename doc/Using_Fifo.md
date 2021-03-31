# How to use LibreRTOS FIFO

LibreRTOS FIFOs are similar to queues for the type uint8_t (size of the items equal to one).

Everything is pretty much the same. However, FIFOs have two additional functions that write one byte.

```
FifoWriteByte(ptr, buff_ptr)
FifoReadByte(ptr, buff_ptr)
```
