CC = gcc
CFLAGS = -O2 -Wall

TARGET = salsa20
SRC = salsa20.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)