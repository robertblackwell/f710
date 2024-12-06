#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include <rbl/logger.h>
// needed for multi threading
static bool needs_initialization = true;
static pthread_mutex_t lock;

void rbl_log_function(const char* level, const char* funcname, const char* filename, int line, char* message)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

	if (needs_initialization) {
		needs_initialization = false;
		pthread_mutex_init(&lock, NULL);
	}
	pthread_mutex_lock(&lock);
	// here put in locking for multi threaded applications
//	printf("%2d:%2d:%2d %s %s[%d] %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, level, funcname, line, message);
    printf("%2d:%2d:%2d %s %s %s[%d] %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, level, funcname, filename, line, message);
	pthread_mutex_unlock(&lock);

}