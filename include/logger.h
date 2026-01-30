#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>

void KMM_log(const char *event, const char *arg);

#endif