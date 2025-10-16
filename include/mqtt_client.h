#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mosquitto.h>
#include <stdbool.h>

typedef struct {
    struct mosquitto *mosq;
    const char *host;
    int port;
    const char *topic;
} MqttClient;

bool mqtt_client_init(MqttClient *client, const char *id, const char *host, int port, const char *topic);
void mqtt_client_cleanup(MqttClient *client);
bool mqtt_client_publish(MqttClient *client, const char *payload);

#endif

