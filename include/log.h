/** log.h **/
// define logging macros

// wrap fprintf to log to file
#include "time.h"
#include <stdio.h>

//  log with timestamp
#define LOG(fmt, ...)                                                          \
  do {                                                                         \
    time_t t = time(NULL);                                                     \
    struct tm *tm = localtime(&t);                                             \
    fprintf(server_log, "[%04d-%02d-%02d %02d:%02d:%02d] " fmt,                \
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,      \
            tm->tm_min, tm->tm_sec, ##__VA_ARGS__);                            \
    fflush(server_log);                                                        \
  } while (0)

// log with special file
#define LOG_FILE(file, fmt, ...)                                               \
  do {                                                                         \
    time_t t = time(NULL);                                                     \
    struct tm *tm = localtime(&t);                                             \
    fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] " fmt, tm->tm_year + 1900,  \
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,  \
            ##__VA_ARGS__);                                                    \
    fflush(file);                                                              \
  } while (0)

extern FILE *server_log;

void init_server_log(const char *log_file);