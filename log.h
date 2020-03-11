
#ifndef _LOG_H_
#define _LOG_H_

#ifdef LOGGING_ENABLED
  #define LOG(fmt, ...) log_message (fmt, ##__VA_ARGS__)
#else
  #define LOG(...) {}
#endif

void log_message(char *format, ...);
void log_error(char *format, ...);

#endif