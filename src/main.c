// main.c (actualizado) - soporta int16 y float32 con opciones de endianness
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <mosquitto.h>
#include <string.h>
#include <stdint.h>

/*
  Orders supported for 32-bit float (4 bytes coming from two 16-bit registers):
    0 = NORMAL       : [W0_hi][W0_lo][W1_hi][W1_lo]    (KLEA example)
    1 = WORD_SWAP    : [W1_hi][W1_lo][W0_hi][W0_lo]
    2 = BYTE_SWAP    : [W0_lo][W0_hi][W1_lo][W1_hi]
    3 = WORD+BYTE    : [W1_lo][W1_hi][W0_lo][W0_hi]
*/

float float_from_regs(uint16_t regs[2], int order) {
    uint8_t b[4];

    uint8_t r0_hi = (regs[0] >> 8) & 0xFF;
    uint8_t r0_lo = regs[0] & 0xFF;
    uint8_t r1_hi = (regs[1] >> 8) & 0xFF;
    uint8_t r1_lo = regs[1] & 0xFF;

    switch (order) {
        case 0: // NORMAL (KLEA: High0 Low0 High1 Low1)
            b[0]=r0_hi; b[1]=r0_lo; b[2]=r1_hi; b[3]=r1_lo;
            break;
        case 1: // WORD_SWAP (High1 Low1 High0 Low0)
            b[0]=r1_hi; b[1]=r1_lo; b[2]=r0_hi; b[3]=r0_lo;
            break;
        case 2: // BYTE_SWAP (Low0 High0 Low1 High1)
            b[0]=r0_lo; b[1]=r0_hi; b[2]=r1_lo; b[3]=r1_hi;
            break;
        case 3: // WORD+BYTE (Low1 High1 Low0 High0)
            b[0]=r1_lo; b[1]=r1_hi; b[2]=r0_lo; b[3]=r0_hi;
            break;
        default:
            // fallback to normal
            b[0]=r0_hi; b[1]=r0_lo; b[2]=r1_hi; b[3]=r1_lo;
            break;
    }

    float f;
    // copiar bytes en orden de máquina (memcpy es portable para este caso)
    memcpy(&f, b, sizeof(f));
    return f;
}

// ------------------ Configuración de Dispositivos ------------------
typedef struct {
    int slave_id;
    const int *registers;   // direcciones base (para int16: una dirección; para float32: base de los dos registros)
    const int *is_float32;  // 0 = int16, 1 = float32
    const int *float_order; // para cada elemento si is_float32==1: orden (0..3) según la tabla anterior
    int num_registers;
    const char *name;
} DeviceConfig;

/* Ejemplo: adapta direcciones y tipos a tu caso real */

const int regs1[]      = {60, 64, 68, 72, 2, 22, 42, 0, 4, 10, 8, 20, 24, 30, 28, 40, 44, 50, 48}; //klea 220
const int types1[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const int orders1[]    = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

const int regs2[]      = {60, 64, 68, 72, 2, 22, 42, 0, 4, 10, 8, 20, 24, 30, 28, 40, 44, 50, 48}; // klea 220
const int types2[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const int orders2[]    = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

//const int regs1[]      = {0, 2, 4, 8, 14, 16, 18, 28, 30, 32, 40, 152,154, 156, 164, 276, 278, 280, 288};       // Equipo1: address base por cada elemento
//const int types1[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};       // Equipo1: Todos en formato float32
//const int orders1[]    = {0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};       // BYTE_SWAP (Low0 High0 Low1 High1)

//const int regs2[]      = {0, 2, 4, 8, 14, 16, 18, 28, 30, 32, 40, 152,154, 156, 164, 276, 278, 280, 288};    // Equipo2
//const int types2[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};       // solo int16
//const int orders2[]    = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

const int regs3[]      = {0, 2, 4, 8, 14, 16, 18, 28, 30, 32, 40, 152,154, 156, 164, 276, 278, 280, 288};    // Equipo3 klea 320
const int types3[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};       // solo int16
const int orders3[]    = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

const int regs4[]      = {60, 64, 68, 72, 2, 22, 42, 0, 4, 10, 8, 20, 24, 30, 28, 40, 44, 50, 48};    // Equipo2
const int types4[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};       // solo int16
const int orders4[]    = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

const int regs5[]      = {0, 2, 4, 8, 14, 16, 18, 28, 30, 32, 40, 152,154, 156, 164, 276, 278, 280, 288};    // Equipo2
const int types5[]     = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};       // solo int16
const int orders5[]    = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};


DeviceConfig devices[] = {
    {1, regs1, types1, orders1, 19, "Equipo1"},
    {2, regs2, types2, orders2, 19, "Equipo2"},
    {3, regs3, types3, orders3, 19, "Equipo3"},
    {4, regs4, types4, orders4, 19, "Equipo4"},
    {5, regs5, types5, orders5, 19, "Equipo5"}
};
int num_devices = sizeof(devices) / sizeof(devices[0]);

// ------------------ Configuración MQTT ------------------
#define MQTT_HOST "192.168.1.12"   // Cambia por tu broker
#define MQTT_PORT 1883
#define MQTT_TOPIC "Energy"

int main(void) {
    modbus_t *ctx = NULL;
    struct mosquitto *mosq = NULL;
    int rc;

    // Init MQTT
    mosquitto_lib_init();
    mosq = mosquitto_new("modbus_mqtt_client", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Error creando cliente MQTT\n");
        goto error;
    }
    rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error conectando broker MQTT: %s\n", mosquitto_strerror(rc));
        goto error;
    }

    // Init Modbus RTU
    ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
    if (!ctx) {
        fprintf(stderr, "No se pudo crear contexto modbus\n");
        goto error;
    }
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Conexión Modbus fallida: %s\n", modbus_strerror(errno));
        goto error;
    }

    printf("Iniciando lectura Modbus y envío por MQTT...\n");

    while (1) {
        for (int d = 0; d < num_devices; d++) {
            DeviceConfig dev = devices[d];
            if (modbus_set_slave(ctx, dev.slave_id) == -1) {
                fprintf(stderr, "modbus_set_slave fallo para id %d: %s\n", dev.slave_id, modbus_strerror(errno));
                continue;
            }

            char payload[1024];
            size_t payload_len = snprintf(payload, sizeof(payload), "%s", dev.name);

            for (int i = 0; i < dev.num_registers; i++) {
                if (dev.is_float32[i] == 1) {
                    // leer 2 registros consecutivos
                    uint16_t regs[2] = {0};
                    int rr = modbus_read_registers(ctx, dev.registers[i], 2, regs);
                    if (rr != 2) {
                        fprintf(stderr, "Error leyendo float reg %d del esclavo %d: %s\n",
                                dev.registers[i], dev.slave_id, modbus_strerror(errno));
                        // agregar marcador 0.0
                        payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len, "@0.0");
                    } else {
                        float v = float_from_regs(regs, dev.float_order[i]);
                        payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len, "@%.6f", v);
                    }
                } else {
                    // leer 1 registro int16
                    uint16_t reg = 0;
                    int rr = modbus_read_registers(ctx, dev.registers[i], 1, &reg);
                    if (rr != 1) {
                        fprintf(stderr, "Error leyendo int reg %d del esclavo %d: %s\n",
                                dev.registers[i], dev.slave_id, modbus_strerror(errno));
                        payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len, "@0");
                    } else {
                        payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len, "@%u", (unsigned)reg);
                    }
                }
                // protección: si se llena el buffer, salimos
                if (payload_len >= sizeof(payload) - 100) break;
            }

            // publicar
            int pub_rc = mosquitto_publish(mosq, NULL, MQTT_TOPIC, (int)strlen(payload), payload, 0, false);
            if (pub_rc != MOSQ_ERR_SUCCESS) {
                fprintf(stderr, "Error publicando MQTT: %s\n", mosquitto_strerror(pub_rc));
            }
            printf("Publicado: %s\n", payload);
        }

        mosquitto_loop(mosq, -1, 1);
        usleep(100000); // 100 ms entre ciclos
    }

error:
    if (ctx) {
        modbus_close(ctx);
        modbus_free(ctx);
    }
    if (mosq) {
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
    return 0;
}

