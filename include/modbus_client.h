#ifndef MODBUS_CLIENT_H
#define MODBUS_CLIENT_H

#include <modbus/modbus.h>
#include <stdbool.h>

typedef struct {
    modbus_t *ctx;
    int slave_id;
} ModbusClient;

bool modbus_client_init(ModbusClient *client, const char *device, int baud, char parity, int data_bit, int stop_bit, int slave_id);
void modbus_client_close(ModbusClient *client);
int modbus_client_read_register(ModbusClient *client, int reg_addr, uint16_t *dest);

#endif

