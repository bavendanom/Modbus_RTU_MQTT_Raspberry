#include "modbus_client.h"
#include <stdio.h>
#include <errno.h>


bool modbus_client_init(ModbusClient *client, const char *device, int baud, char parity, int data_bit, int stop_bit, int slave_id) {
    client->ctx = modbus_new_rtu(device, baud, parity, data_bit, stop_bit);
    if (client->ctx == NULL) {
        fprintf(stderr, "Unable to create libmodbus context\n");
        return false;
    }

    if (modbus_connect(client->ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(client->ctx);
        return false;
    }

    client->slave_id = slave_id;
    modbus_set_slave(client->ctx, slave_id);
    return true;
}

void modbus_client_close(ModbusClient *client) {
    if (client->ctx != NULL) {
        modbus_close(client->ctx);
        modbus_free(client->ctx);
    }
}

int modbus_client_read_register(ModbusClient *client, int reg_addr, uint16_t *dest) {
    return modbus_read_registers(client->ctx, reg_addr, 1, dest);
}

