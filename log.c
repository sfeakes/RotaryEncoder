
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <syslog.h>

extern bool _daemon_;

void log_message(char *format, ...)
{
  va_list arglist;
  va_start( arglist, format );
  vfprintf(stdout, format, arglist );
  va_end( arglist );
}

void log_error(char *format, ...)
{
  va_list arglist;
  va_start( arglist, format );

  if (_daemon_)
    syslog (LOG_ERR, format, arglist);
  else
    vfprintf(stderr, format, arglist );
  
  va_end( arglist );
}