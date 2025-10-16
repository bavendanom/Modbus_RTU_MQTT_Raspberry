#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>

bool mqtt_client_init(MqttClient *client, const char *id, const char *host, int port, const char *topic) {
    mosquitto_lib_init();
    client->mosq = mosquitto_new(id, true, NULL);
    if (!client->mosq) {
        fprintf(stderr, "Error creando cliente MQTT\n");
        return false;
    }
    if (mosquitto_connect(client->mosq, host, port, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error conectando a broker MQTT %s:%d\n", host, port);
        mosquitto_destroy(client->mosq);
        return false;
    }
    client->host = host;
    client->port = port;
    client->topic = topic;
    return true;
}

void mqtt_client_cleanup(MqttClient *client) {
    if (client->mosq) {
        mosquitto_disconnect(client->mosq);
        mosquitto_destroy(client->mosq);
    }
    mosquitto_lib_cleanup();
}

bool mqtt_client_publish(MqttClient *client, const char *payload) {
    int rc = mosquitto_publish(client->mosq, NULL, client->topic, strlen(payload), payload, 0, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error publicando MQTT: %s\n", mosquitto_strerror(rc));
        return false;
    }
    mosquitto_loop(client->mosq, -1, 1);
    return true;
}

