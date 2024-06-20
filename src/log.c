#include "log.h"
#include <stdlib.h>
FILE *server_log;

void init_server_log(const char *log_file) {
  // open log file, create if not exists
  server_log = fopen(log_file, "a");
  // LOG("Server log file opened\n");
  if (server_log == NULL) {
    perror("Error opening log file");
    exit(1);
  }
}