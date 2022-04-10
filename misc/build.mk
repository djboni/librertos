# Copyright (c) 2022 Djones A. Boni - MIT License

EXEC = build/main.elf
ARCHIVE = build/main.a
OBJ_DIR = build/obj
INPUTS = $(wildcard *.c) $(wildcard *.cpp)

CC = gcc
CFLAGS = -g -O0 -std=c90 -pedantic -Wall -Wextra -Werror
CXX = g++
CXXFLAGS = -g -O0 -std=c++98 -pedantic -Wall -Wextra -Werror
CPPFLAGS =
LD = gcc
LDFLAGS =
LDLIBS =

AR = ar
ARFLAGS = -cq

INPUTS_NO_PARENT = $(subst ../,,$(INPUTS))
OBJECTS_STEP = $(patsubst %.c,$(OBJ_DIR)/%.o,$(INPUTS_NO_PARENT))
OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(OBJECTS_STEP))

all: $(EXEC)

$(OBJ_DIR)/%.o: %.c
	@X="$@"; if [ "$${X%/*}" != "$$X" ]; then mkdir -p "$${X%/*}"; fi
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(OBJ_DIR)/%.o: %.cpp
	@X="$@"; if [ "$${X%/*}" != "$$X" ]; then mkdir -p "$${X%/*}"; fi
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -c -o $@

$(EXEC): $(OBJECTS)
	@X="$@"; if [ "$${X%/*}" != "$$X" ]; then mkdir -p "$${X%/*}"; fi
	$(LD) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(ARCHIVE): $(OBJECTS)
	@X="$@"; if [ "$${X%/*}" != "$$X" ]; then mkdir -p "$${X%/*}"; fi
	$(AR) $(ARFLAGS) $@ $^
