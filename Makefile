CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
SRCS = main.c cpu.c assembler.c display.c programs.c
HDRS = cpu.h assembler.h display.h programs.h
TARGET = cpu

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
