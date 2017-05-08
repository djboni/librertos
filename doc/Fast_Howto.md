# Fast Howto

1. Create a project.

2. Clone LibreRTOS repository into the project directory.

```
$ cd PROJECT/DIR
$ git clone https://github.com/djboni/librertos.git
```

3. Include LibreRTOS directory into the project.

4. Copy the the architecture `projdefs_*.h` file from the `librertos/port` directory alongside `main.c` file and rename it to projdefs.h.
  This file has the critical section management macros definitions and other macros to enable/disable LibreRTOS features.

5. Include the `projdefs.h` and `LibreRTOS.h` directories in the include path.

```
..
../librertos
```

6. The project should compile now. `:)`
