CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
COMMON := ../common
TARGET := build/test_agri_control
SOURCES := test_agri_control.c $(COMMON)/agri_control.c $(COMMON)/agri_sensor_convert.c
.PHONY: all test clean
all: $(TARGET)
$(TARGET): $(SOURCES) | build
	$(CC) $(CFLAGS) -I$(COMMON) $(SOURCES) -lm -o $@
build:
	mkdir -p $@
test: $(TARGET)
	./$(TARGET)
clean:
	rm -rf build
