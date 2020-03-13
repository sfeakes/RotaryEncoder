//http://thunderbird3:80/api/player/volume?step=+5&output_id=11259997263078

//libcurl4-openssl-dev

#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <syslog.h>
#include <libgen.h>
#include <curl/curl.h>

#include "mongoose.h"

#include "version.h"
#include "GPIO_Pi.h"
#include "MQTT.h"
#include "log.h"

// Default run parameters
#define P_CLK  22
#define P_DT   27
#define P_SW   17
#define MQTT_S "trident:1883"
#define MQTT_T "shairport-sync/study/remote"

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

void daemonise (char *pidFile, void (*main_function) (void));
int main_loop(void);

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
    syslog (LOG_NOTICE, "%s %s daemon starting with:-\n\tMQTT server %s, MQTT topic %s, Clock %d, Data %d, Switch %d\n",\
                      ROTARYENCODER_NAME,ROTARYENCODER_VERSION,_mqtt_server,_mqtt_topic,_p_clk,_p_dt,_p_sw);
  } else {
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

  mg_mgr_init(&mgr, NULL);
  start_mqtt(&mgr, _mqtt_server, _mqtt_topic);

  while(FOREVER) {

    if ( _mqtt_status_ == mqttstopped) {
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
    mg_mgr_poll(&mgr, 0);

    // Using interupts here rather that registering with GPIO_Pi as timing & sequence is highly important. 
    if ( digitalRead(_p_clk) != clk) {
      clk = !clk;
      dt = digitalRead(_p_dt);
      LOG("Clock|DT = %d|%d\n",clk,dt);
      if (clk==1 && dt==1) {
        send_mqtt_msg("volumedown");
      } else if (clk==1 && dt==0) {
        send_mqtt_msg("volumeup");
      }
      cnt = 0;
    }
    if ( digitalRead(_p_sw) != sw) {
      sw = !sw;
      LOG("SW  = %d\n",sw);
      if (sw == 1)
        send_mqtt_msg("mutetoggle");
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