CC = gcc 
CFLAGS = -Wall -Wextra -D_FILE_OFFSET_BITS=64
LDFLAGS = -lfuse
DEBUGFLAGS = -g -O0
SRC = src/client/rtfs.c
OBJ = $(SRC:.c=.o)
TARGET = rtfs

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

debug: CFLAGS += $(DEBUGFLAGS)
debug: clean $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all debug clean