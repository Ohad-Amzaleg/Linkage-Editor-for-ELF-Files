CC=gcc
CFLAGS=-Wall `pkg-config --cflags --libs glib-2.0` -I/data/linker
TARGET=linker
SRC=elf_linker.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(TARGET)
