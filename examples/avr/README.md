# LibreRTOS on AVR

- Board: Arduino Mega 2560
- Microcontroller: AVR ATmega2560
- Compiler: GCC
- Tick: 16 bits

1. Install the dependencies (Ubuntu 22.04):

   ```sh
   sudo apt install make gcc-avr avr-libc avrdude
   ```

2. Compile:

   ```sh
   make
   ```

3. Write flash:

   ```sh
   make flash
   ```

4. Read the serial:

- Cutecom:

  ```sh
  sudo apt install cutecom
  cutecom
  ```

- Terminal

  ```sh
  stty -F /dev/ttyACM0 speed 115200
  cat /dev/ttyACM0
  ```

  Note that to read the serial (/dev/ttyACM0) the user must be in the dialout
  group. To add the current user to the group use the command below and
  login again or restart the computer.

  ```sh
  sudo adduser $USER dialout
  ```

## RAM Usage

| Element   | Data Size          |
| --------- | ------------------ |
| LibreRTOS | 25 + 5\*priorities |
| Task      | 25                 |
| Timer     | 33                 |
| Semaphore | 7                  |
| Mutex     | 8                  |
| Queue     | 17 + buff          |

- Size in bytes.

## Flash Usage

| Kernel Mode | Optimization | Everything | Only Tasks |
| ----------- | ------------ | ---------- | ---------- |
| Preemptive  | -Os (size)   | 4923       | 2059       |
| Preemptive  | -O1          | 5121       | 2267       |

- Size in bytes.
- The column "Everything" includes code for tasks, timers, semaphores, mutexes,
  and queues.
- The column "Only Tasks" includes only the code for tasks.
