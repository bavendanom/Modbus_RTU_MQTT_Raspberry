CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lmodbus -lmosquitto
SRC = src/main.c src/modbus_client.c src/utils.c src/mqtt_client.c
OBJ = $(SRC:.c=.o)
TARGET = app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

