#include "timestamp.h"
#include "sys/time.h"
#include <stdio.h>
#include <time.h>

void getTimeStamp(char *buf, int len) {
  struct timeval tv;
  struct tm *tm;
  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);
  snprintf(buf, len, "%04d-%02d-%02d_%02d:%02d:%02d.%06ld", tm->tm_year + 1900,
           tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
           tv.tv_usec);
}