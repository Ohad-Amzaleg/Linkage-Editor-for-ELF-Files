#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

// Define log levels
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

// Function prototypes
void log_message(FILE *stream, LogLevel level, const char *format, ...);

#endif // LOGGER_H
