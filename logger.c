
#include "logger.h"
#include <stdlib.h>
#include <time.h>

// Function to get current timestamp
char *get_timestamp() {
  time_t rawtime;
  struct tm *info;
  char *buffer =
      malloc(sizeof(char) *
             20); // Allocate space for timestamp (e.g., "YYYY-MM-DD HH:MM:SS")

  if (buffer == NULL) {
    return NULL; // Handle memory allocation failure
  }

  time(&rawtime);
  info = localtime(&rawtime);
  strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", info);

  return buffer;
}

// Log message to specified stream with timestamp and log level
void log_message(FILE *stream, LogLevel level, const char *format, ...) {
  va_list args;
  va_start(args, format);

  char *timestamp = get_timestamp();
  const char *level_str;

  switch (level) {
  case LOG_INFO:
    level_str = "INFO";
    break;
  case LOG_WARNING:
    level_str = "WARNING";
    break;
  case LOG_ERROR:
    level_str = "ERROR";
    break;
  case LOG_DEBUG:
    level_str = "DEBUG";
    break;
  default:
    level_str = "UNKNOWN";
    break;
  }

  fprintf(stream, "[%s] [%s] ", timestamp, level_str);
  vfprintf(stream, format, args);
  fprintf(stream, "\n");

  free(timestamp);
  va_end(args);
}
