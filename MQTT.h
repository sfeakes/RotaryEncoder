
#ifndef _MQTT_H_
#define _MQTT_H_

typedef enum {mqttstarting, mqttrunning, mqttstopped} mqttstatus;

#ifdef _MQTT_C_
mqttstatus _mqtt_status_ = mqttstopped;
//struct mg_connection *_mqtt_connection;
#else
extern mqttstatus _mqtt_status_;
//extern struct mg_connection *_mqtt_connection;
#endif

void send_mqtt_msg(char *message);
void start_mqtt (struct mg_mgr *mgr, char *mqtt_server, char *mqtt_topic);
//void start_mqtt (struct mg_mgr *mgr);

#endif