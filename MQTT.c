
#include <stdio.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "mongoose.h"

#define _MQTT_C_
#include "MQTT.h"
#include "log.h"

//char _mqtt_address[64];
//char _mqtt_pub_topic[128];
//char _mqtt_address[64];
char *_mqtt_address;
char *_mqtt_pub_topic;
char _mqtt_ID[30];

struct mg_connection *_mqtt_connection;

 // Find the first network interface with valid MAC and put mac address into buffer upto length
bool mac(char *buf, int len)
{
  struct ifreq s;
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  struct if_nameindex *if_nidxs, *intf;

  if_nidxs = if_nameindex();
  if (if_nidxs != NULL)
  {
    for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++)
    {
      strcpy(s.ifr_name, intf->if_name);
      if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
      {
        int i;
        if ( s.ifr_addr.sa_data[0] == 0 &&
             s.ifr_addr.sa_data[1] == 0 &&
             s.ifr_addr.sa_data[2] == 0 &&
             s.ifr_addr.sa_data[3] == 0 &&
             s.ifr_addr.sa_data[4] == 0 &&
             s.ifr_addr.sa_data[5] == 0 ) {
          continue;
        }
        for (i = 0; i < 6 && i * 2 < len; ++i)
        {
          sprintf(&buf[i * 2], "%02x", (unsigned char)s.ifr_addr.sa_data[i]);
        }
        return true;
      }
    }
  }

  return false;
}

// Need to update network interface.
char *generate_mqtt_id(char *buf, int len) {
  extern char *__progname; // glibc populates this
  int i;
  strncpy(buf, basename(__progname), len);
  i = strlen(buf);

  if (i < len) {
    buf[i++] = '_';
    // If we can't get MAC to pad mqtt id then use PID
    if (!mac(&buf[i], len - i)) {
      sprintf(&buf[i], "%.*d", (len-i), getpid());
    }
  }
  buf[len] = '\0';

  return buf;
}


void _send_mqtt_msg(struct mg_connection *nc, char *message) {
  static uint16_t msg_id = 0;

  if ( _mqtt_status_ == mqttstopped || _mqtt_connection == NULL) {
  //if ( _mqtt_exit_flag == true || _mqtt_connection == NULL) {
    log_error("ERROR: No mqtt connection, can't send message '%s'\n", message);
    return;
  }
  // basic counter to give each message a unique ID.
  if (msg_id >= 65535) {
    msg_id = 1;
  } else {
    msg_id++;
  }

  mg_mqtt_publish(nc, _mqtt_pub_topic, msg_id, MG_MQTT_QOS(0), message, strlen(message));

  LOG("MQTT: Published: '%s' with id %d\n", message, msg_id);

}
void send_mqtt_msg(char *message) {
  return _send_mqtt_msg(_mqtt_connection, message);
}

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;
  (void)nc;

  //if (ev != MG_EV_POLL) LOG("MQTT handler got event %d\n", ev);
  switch (ev) {
    case MG_EV_CONNECT: {
      struct mg_send_mqtt_handshake_opts opts;
      memset(&opts, 0, sizeof(opts));
      //opts.user_name = _config.mqtt_user;
      //opts.password = _config.mqtt_passwd;
      opts.keep_alive = 5;
      opts.flags |= MG_MQTT_CLEAN_SESSION; // NFS Need to readup on this
      mg_set_protocol_mqtt(nc);
      mg_send_mqtt_handshake_opt(nc, _mqtt_ID, opts);
      LOG("Connected to mqtt %s with id of: %s\n", _mqtt_address, _mqtt_ID);
      _mqtt_status_ = mqttrunning;
      _mqtt_connection = nc;
    } break;
    case MG_EV_MQTT_CONNACK:
      if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
        LOG("Got mqtt connection error: %d\n", msg->connack_ret_code);
        _mqtt_status_ = mqttstopped;
        _mqtt_connection = NULL;
      }
    break;
    case MG_EV_MQTT_PUBACK:
      LOG("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
    break;
    case MG_EV_CLOSE:
      log_error("MQTT Connection closed to %s\n",_mqtt_address);
      _mqtt_status_ = mqttstopped;
      _mqtt_connection = NULL;
    break;
  }
}


void start_mqtt (struct mg_mgr *mgr, char *mqtt_server, char *mqtt_topic) {
  LOG("Starting MQTT service\n");

  static bool init = false;

  if (init == false){
    //sprintf(_mqtt_address, "trident:1883");
    //sprintf(_mqtt_pub_topic, "shairport-sync/study/remote");
    generate_mqtt_id(_mqtt_ID, 20);
    init = true;
    //printf("********\n");
    //printf("** %s **\n",_mqtt_ID);
    //printf("** %s **\n",_mqtt_address);
    //printf("** %s **\n",_mqtt_pub_topic);
  }
  _mqtt_pub_topic = mqtt_topic;
  _mqtt_address = mqtt_server;

  LOG("Connecting to mqtt at '%s'\n", mqtt_server);
  if (mg_connect(mgr, mqtt_server, ev_handler) == NULL) {
    log_error("mg_connect(%s) failed\n", mqtt_server);
    exit(EXIT_FAILURE);
  }
  _mqtt_status_ = mqttstarting;
}


/*








char _mqtt_address[64];
char _mqtt_ID[30];


struct mg_connection *_mqtt_connection;

bool mac(char *buf, int len)
{
  #ifdef DEBUG_LOGGING_ENABLED
  len = len - 2; // NSF REMOVE THIS.
  #endif

  struct ifreq s;
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  struct if_nameindex *if_nidxs, *intf;

  if_nidxs = if_nameindex();
  if (if_nidxs != NULL)
  {
    for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++)
    {
      strcpy(s.ifr_name, intf->if_name);
      if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
      {
        int i;
        if ( s.ifr_addr.sa_data[0] == 0 &&
             s.ifr_addr.sa_data[1] == 0 &&
             s.ifr_addr.sa_data[2] == 0 &&
             s.ifr_addr.sa_data[3] == 0 &&
             s.ifr_addr.sa_data[4] == 0 &&
             s.ifr_addr.sa_data[5] == 0 ) {
          continue;
        }
        for (i = 0; i < 6 && i * 2 < len; ++i)
        {
          sprintf(&buf[i * 2], "%02x", (unsigned char)s.ifr_addr.sa_data[i]);
        }
        return true;
      }
    }
  }

  return false;
}

// Need to update network interface.
char *generate_mqtt_id(char *buf, int len) {
  extern char *__progname; // glibc populates this
  int i;
  strncpy(buf, basename(__progname), len);
  i = strlen(buf);

  if (i < len) {
    buf[i++] = '_';
    // If we can't get MAC to pad mqtt id then use PID
    if (!mac(&buf[i], len - i)) {
      sprintf(&buf[i], "%.*d", (len-i), getpid());
    }
  }
  buf[len] = '\0';

  return buf;
}

void send_mqtt_msg(struct mg_connection *nc, char *message) {
  static uint16_t msg_id = 0;

  if ( _mqtt_status_ == mqttstopped || _mqtt_connection == NULL) {
  //if ( _mqtt_exit_flag == true || _mqtt_connection == NULL) {
    fprintf(stderr, "ERROR: No mqtt connection, can't send message '%s'\n", message);
    //log_message("ERROR: No mqtt connection, can't send message '%s'\n", message);
    return;
  }
  // basic counter to give each message a unique ID.
  if (msg_id >= 65535) {
    msg_id = 1;
  } else {
    msg_id++;
  }

  //mg_mqtt_publish(nc, _config.mqtt_pub_topic, msg_id, MG_MQTT_QOS(0), message, strlen(message));
  mg_mqtt_publish(nc, "test/volume", msg_id, MG_MQTT_QOS(0), message, strlen(message));

  LOG("MQTT: Published: '%s' with id %d\n", message, msg_id);

}

static void mqtt_ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;
  (void)nc;

  //if (ev != MG_EV_POLL) LOG("MQTT handler got event %d\n", ev);
  switch (ev) {
    case MG_EV_CONNECT: {
      //set_mqtt(nc);
      struct mg_send_mqtt_handshake_opts opts;
      memset(&opts, 0, sizeof(opts));
      //opts.user_name = _config.mqtt_user;
      //opts.password = _config.mqtt_passwd;
      opts.keep_alive = 5;
      opts.flags |= MG_MQTT_CLEAN_SESSION; // NFS Need to readup on this
      mg_set_protocol_mqtt(nc);
      mg_send_mqtt_handshake_opt(nc, _mqtt_ID, opts);
      //fprintf(stdout, "Connected to mqtt %s with id of: %s\n", _config.mqtt_address, _config.mqtt_ID);
      log_message("Notice: Connected to mqtt %s with id of: %s\n", _mqtt_address, _mqtt_ID);
      _mqtt_status_ = mqttrunning;
      _mqtt_connection = nc;
    } break;
    case MG_EV_MQTT_CONNACK:
      if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
        LOG("Got mqtt connection error: %d\n", msg->connack_ret_code);
        _mqtt_status_ = mqttstopped;
        _mqtt_connection = NULL;
      }
    break;
    case MG_EV_MQTT_PUBACK:
      LOG("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
    break;
    case MG_EV_CLOSE:
      //fprintf( stderr, "MQTT Connection closed\n");
      log_message("Warning: MQTT Connection closed\n");
      _mqtt_status_ = mqttstopped;
      _mqtt_connection = NULL;
    break;
  }
}

void start_mqtt (struct mg_mgr *mgr) {
  LOG("Starting MQTT service\n");
  const bool init = false;

  if (init == false){
    sprintf(_mqtt_address, "trident:1883");
    _mqtt_ID = generate_mqtt_id(&_mqtt_ID, 20);
    init == true;
  }

  if (mg_connect(mgr, _mqtt_address, mqtt_ev_handler) == NULL) {
    fprintf(stderr, "mg_connect(%s) failed\n", _mqtt_address);
    //log_message("ERROR: MQTT mg_connect(%s) failed\n", _mqtt_address);
    exit(EXIT_FAILURE);
  }
  _mqtt_status_ = mqttstarting;
}
*/
/*
generate_mqtt_id(_config.mqtt_ID, MQTT_ID_LEN);

  mg_mgr_init(&_mgr, NULL);

  if (_config.mqtt_address != NULL) {
    LOG("Connecting to mqtt at '%s'\n", _config.mqtt_address);
    start_mqtt(&_mgr);
  }
*/