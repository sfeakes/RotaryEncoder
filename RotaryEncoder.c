#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <syslog.h>
#include <libgen.h>

#ifdef WITH_DAAPD
  #include <curl/curl.h>
#endif

#include "mongoose.h"
#include "MQTT.h"


#include "version.h"
#include "GPIO_Pi.h"
#include "log.h"

// Default run parameters
#define P_CLK  22
#define P_DT   27
#define P_SW   17
#define MQTT_S "trident:1883"
#define MQTT_T "shairport-sync/study/remote"

#ifdef WITH_DAAPD
  #define DAAPD_S "thunderbird3:80"
  #define DAAPD_PLAYER "11259997263078"
#endif

// PID file for daemon
#define PIDLOCATION "/run/"

// Itterations for quick reading of GPIO interupts.
#define QT_CNT 500

bool _daemon_ = false;
bool FOREVER = true;

int _p_clk = P_CLK;
int _p_dt = P_DT;
int _p_sw = P_SW;
char *_mqtt_server = MQTT_S;
char *_mqtt_topic = MQTT_T;
bool _use_daapd = false;

typedef enum {volumeup, volumedown, mutetoggle} play_command;

#ifdef WITH_DAAPD
  char *_daapd_server = DAAPD_S;
  char *_daapd_player = DAAPD_PLAYER;
  void send_daapd_msg(play_command command);
#endif

void daemonise (char *pidFile, void (*main_function) (void));
int main_loop(void);
void send_player_msg(play_command command);


void intHandler(int signum) {
  static int called=0;
  
  if (_daemon_)
    syslog (LOG_NOTICE, "Stopping! - signel(%d)\n",signum);
  else
    LOG ( "Stopping! - signel(%d)\n",signum);

  gpioShutdown();

  FOREVER = false;
  called++;
  if (called > 3)
    exit(1);
}

void printHelp()
{
  printf("%s %s\n",ROTARYENCODER_NAME,ROTARYENCODER_VERSION);
  printf("\t-h         (this message)\n");
  printf("\t-d         (run as a deamon)\n");
  printf("\t-ms        (MQTT Server:Port)\n");
  printf("\t-mt        (MQTT Topic)\n");
#ifdef WITH_DAAPD
  printf("\t-ds        (DAAPD Server:Port)\n");
  printf("\t-dp        (DAAPD Player ID)\n");
#endif
  printf("\t-rc        (Rotaryencoder Clock GPIO pin)\n");
  printf("\t-rd        (Rotaryencoder Data GPIO pin)\n");
  printf("\t-rs        (Rotaryencoder Switch GPIO pin)\n");
  printf("\n");
  printf("example :- RotaryEncoder -ms 192.168.1.20:1883 -mt \"shairport-sync/Living Room/remote\" -rc 27 -rd 22 -rc 17\n");
  printf("\n");
}

int main(int argc, char *argv[]) {
  int i;

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-h") == 0)
    {
      printHelp();
      return EXIT_SUCCESS;
    }
    else if (strcmp(argv[i], "-d") == 0)
      _daemon_ = true;
    else if (strcmp(argv[i], "-ms") == 0)
      _mqtt_server = argv[++i];
    else if (strcmp(argv[i], "-mt") == 0)
      _mqtt_topic = argv[++i];
    else if (strcmp(argv[i], "-rc") == 0)
      _p_clk = atoi(argv[++i]);
    else if (strcmp(argv[i], "-rd") == 0)
      _p_dt = atoi(argv[++i]);
    else if (strcmp(argv[i], "-rs") == 0)
      _p_sw = atoi(argv[++i]);
#ifdef WITH_DAAPD
    else if (strcmp(argv[i], "-ds") == 0) {
      _use_daapd = true;
      _daapd_server = argv[++i];
    } else if (strcmp(argv[i], "-dp") == 0) {
      _use_daapd = true;
      _daapd_player = argv[++i];
    }
#endif
  }

  if (getuid() != 0)
  {
    //logMessage(LOG_ERR, "%s Can only be run as root\n", argv[0]);
    log_error("ERROR %s Can only be run as root\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (_daemon_ == false)
  {
    return main_loop ();
  }
  else
  {
    char pidfile[256];
    sprintf(pidfile, "%s/%s.pid",PIDLOCATION, basename(argv[0]));
    daemonise (pidfile, (void *)main_loop);
  }

  exit (EXIT_SUCCESS);
}

int main_loop() {
  
  struct mg_mgr mgr;
  int cnt = QT_CNT;
  int bad_mqtt = 0;

  int clk = 1;
  int dt = 1;
  int sw = 1;

  if (_daemon_) {
    #ifdef WITH_DAAPD
      if (_use_daapd)
        syslog (LOG_NOTICE, "%s %s daemon starting with:-\n\tdaapd server %s, daapd player %s, Clock %d, Data %d, Switch %d\n",\
                      ROTARYENCODER_NAME,ROTARYENCODER_VERSION,_daapd_server,_daapd_player,_p_clk,_p_dt,_p_sw);
      else
    #endif
      syslog (LOG_NOTICE, "%s %s daemon starting with:-\n\tMQTT server %s, MQTT topic %s, Clock %d, Data %d, Switch %d\n",\
                      ROTARYENCODER_NAME,ROTARYENCODER_VERSION,_mqtt_server,_mqtt_topic,_p_clk,_p_dt,_p_sw);
  } else {
    #ifdef WITH_DAAPD
      if (_use_daapd)
        printf ("%s %s starting with:-\n\tdaapd server %s, daapd player %s, Clock %d, Data %d, Switch %d\n",\
                      ROTARYENCODER_NAME,ROTARYENCODER_VERSION,_daapd_server,_daapd_player,_p_clk,_p_dt,_p_sw);
      else
    #endif
    printf("%s %s starting with :-\n\tMQTT server %s, MQTT topic %s, Clock %d, Data %d, Switch %d\n",\
          ROTARYENCODER_NAME,ROTARYENCODER_VERSION,_mqtt_server,_mqtt_topic,_p_clk,_p_dt,_p_sw);
  }

  gpioSetup();

  if (pinMode(_p_clk, INPUT) < 0 || pinMode(_p_dt, INPUT) < 0 || pinMode(_p_sw, INPUT) < 0) {
    log_error("Error setting pinmode\n");
    return EXIT_FAILURE;
  }

  setPullUpDown(_p_clk, PUD_UP);
  setPullUpDown(_p_dt, PUD_UP);
  setPullUpDown(_p_sw, PUD_UP);

  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);
  signal(SIGSEGV, intHandler);

#ifdef WITH_DAAPD
  if (_use_daapd) {
    curl_global_init(CURL_GLOBAL_ALL);
  } else {
#endif
    mg_mgr_init(&mgr, NULL);
    start_mqtt(&mgr, _mqtt_server, _mqtt_topic);
#ifdef WITH_DAAPD
  }
#endif

  while(FOREVER) {

    if ( !_use_daapd && _mqtt_status_ == mqttstopped) {
      LOG("Reset connections\n");
      // Should put a pause in here after first try failed.
      // Not putting it in yet untill debugged mqtt random disconnects.
      start_mqtt(&mgr, _mqtt_server, _mqtt_topic);
      bad_mqtt++;
      if (bad_mqtt >= INT_MAX) {
        log_error("Error MQTT down, giving up\n");
        return EXIT_FAILURE;
      } else if (bad_mqtt > 30) {
        sleep(10);
      } else if (bad_mqtt > 3) {
        sleep(1);
      }
    } else if ( _mqtt_status_ == mqttrunning) {
      bad_mqtt=0;
    }
    #ifdef WITH_DAAPD
    if (!_use_daapd)
    #endif
      mg_mgr_poll(&mgr, 0);

    // Using interupts here rather that registering with GPIO_Pi as timing & sequence is highly important. 
    if ( digitalRead(_p_clk) != clk) {
      clk = !clk;
      dt = digitalRead(_p_dt);
      LOG("Clock|DT = %d|%d\n",clk,dt);
      if (clk==1 && dt==1) {
          send_player_msg(volumedown);
      } else if (clk==1 && dt==0) {
          send_player_msg(volumeup);
      }
      cnt = 0;
    }
    if ( digitalRead(_p_sw) != sw) {
      sw = !sw;
      LOG("SW  = %d\n",sw);
      if (sw == 1) {
          send_player_msg(mutetoggle);
      }
      cnt = 0;
    }
    
    // If we received an interupt poll quicker for a short period of time
    if ( cnt++ < QT_CNT)
      usleep(500);
    else {
      usleep(5000);
      if (cnt >= INT_MAX )
        cnt = QT_CNT;
    }
  }

  gpioShutdown();

  #ifdef WITH_DAAPD
    if (!_use_daapd)
  #endif
    mg_mgr_free(&mgr);

  sleep(1);

  return EXIT_SUCCESS;
}


void daemonise (char *pidFile, void (*main_function) (void))
{
  FILE *fp = NULL;
  pid_t process_id = 0;
  pid_t sid = 0;

  _daemon_ = true;
  
  /* Check we are root */
  if (getuid() != 0)
  {
    log_error("Can only be run as root\n");
    exit(EXIT_FAILURE);
  }

  int pid_file = open (pidFile, O_CREAT | O_RDWR, 0666);
  int rc = flock (pid_file, LOCK_EX | LOCK_NB);
  if (rc)
  {
    if (EWOULDBLOCK == errno)
    ; // another instance is running
    //fputs ("\nAnother instance is already running\n", stderr);
    log_error("\nAnother instance is already running\n");
    exit (EXIT_FAILURE);
  }

  process_id = fork ();
  // Indication of fork() failure
  if (process_id < 0)
  {
    log_error("fork failed!");
    // Return failure in exit status
    exit (EXIT_FAILURE);
  }
  // PARENT PROCESS. Need to kill it.
  if (process_id > 0)
  {
    fp = fopen (pidFile, "w");

    if (fp == NULL)
    log_error("can't write to PID file %s",pidFile);
    else
    fprintf(fp, "%d", process_id);

    fclose (fp);
    log_error("process_id of child process %d \n", process_id);
    // return success in exit status
    exit (EXIT_SUCCESS);
  }
  //unmask the file mode
  umask (0);
  //set new session
  sid = setsid ();
  if (sid < 0)
  {
    // Return failure
    log_error("Failed to fork process");
    exit (EXIT_FAILURE);
  }
  // Change the current working directory to root.
  chdir ("/");
  // Close stdin. stdout and stderr
  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);

  // this is the first instance
  (*main_function) ();

  return;
}

void send_player_msg(play_command command)
{
  #ifdef WITH_DAAPD
    if (_use_daapd)
      return send_daapd_msg(command);
  #endif

  switch(command) {
      case volumeup:
        send_mqtt_msg("volumeup");
      break;
      case volumedown:
        send_mqtt_msg("volumedown");
      break;
      case mutetoggle:
        send_mqtt_msg("mutetoggle");
      break;
      default:
        return;
      break;
    }
}



#ifdef WITH_DAAPD
//http://thunderbird3:80/api/player/volume?step=+5&output_id=11259997263078

// PUT /api/outputs/{id}/toggle
// PUT /api/player/volume

void send_daapd_msg(play_command command)
{
  CURL *curl;
  CURLcode res;
  char url[128];

  curl = curl_easy_init();
  if(curl) {
    // Really strange way to do put, () but works
    switch(command) {
      case volumeup:
        sprintf(url, "http://%s/api/player/volume?step=+5&output_id=%s",_daapd_server,_daapd_player);
      break;
      case volumedown:
        sprintf(url, "http://%s/api/player/volume?step=-5&output_id=%s",_daapd_server,_daapd_player);
      break;
      case mutetoggle:
        sprintf(url, "http://%s/api/outputs/%s/toggle",_daapd_server,_daapd_player);
      break;
      default:
        return;
      break;
    }
    //sprintf(url, "http://%s/api/player/volume?%s&output_id=%s",_daapd_server,msg,_daapd_player);
    
    LOG("calling %s\n",url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); /* !!! */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); 

    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      log_error("curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  } else {
    log_error("Failed to initialize curl");
  }
}
#endif



#ifdef MONGOOSE_PUT_PLAYING

static int s_show_headers = 0;
static const char *s_show_headers_opt = "--show-headers";
static int s_exit_flag = 0;

static void http_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_CONNECT:
      if (*(int *) ev_data != 0) {
        fprintf(stderr, "connect() failed: %s\n", strerror(*(int *) ev_data));
        s_exit_flag = 1;
      }
      break;
    case MG_EV_HTTP_REPLY:
      printf("Server Reply\n");
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      if (s_show_headers) {
        fwrite(hm->message.p, 1, hm->message.len, stdout);
      } else {
        fwrite(hm->body.p, 1, hm->body.len, stdout);
      }
      putchar('\n');
      s_exit_flag = 1;
      break;
    case MG_EV_CLOSE:
      if (s_exit_flag == 0) {
        printf("Server closed connection\n");
        s_exit_flag = 1;
      }
      break;
    default:
      break;
  }
}

void send_daapd_mongoose(play_command command)
{
  char url[128];
  struct mg_mgr mgr;
  struct at_http_request* request;

  sprintf(url, "http://%s/api/outputs/%s/toggle",_daapd_server,_daapd_player);
  LOG("Connecting %s\n",url);
  
  mg_mgr_init(&mgr, NULL);

  mg_connect_http(&mgr, http_ev_handler, url, NULL, NULL);

/*
  struct mg_connect_opts opts;
  //struct mg_bind_opts opts;
  memset( &opts, 0, sizeof( opts ) );
  //request = malloc(sizeof (struct at_http_request));
  request = malloc(10000);

  opts.user_data = request;
  //opts.
  mg_connect_http_opt(mgr, http_client_ev_handler, opts, url, NULL, NULL);
*/
  while (s_exit_flag == 0) {
    mg_mgr_poll(&mgr, 100);
  }

  mg_mgr_free(&mgr);

}

#endif





