#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "logger.h"

extern void logger(int LoggingLevel, const char *ModuleName, const char *File, int Line, const char *Format, ...)
{
char Buffer[2048];
int BufSize= 2048 - 1;
va_list Args;
char *LoggingLevelString;
char CurrentTimeBuffer[64];
struct timeval tv;
time_t CurrentTime;
FILE *DestFile= stdout;//stderr;


	if (LoggingLevel < LOGGING_LEVEL)
		return;

	switch(LoggingLevel)
	{
		case ORCH_DEBUG: LoggingLevelString= "DEBUG"; break;
		case ORCH_DEBUG_INFO: LoggingLevelString= "DEBUG-INFO"; break;
		case ORCH_WARNING: LoggingLevelString= "WARNING"; break;
		case ORCH_ERROR: LoggingLevelString= "ERROR"; break;
		case ORCH_INFO: LoggingLevelString= "INFO"; break;
		// The 'default' case is just to avoid a compiler warning
		default: LoggingLevelString= "UNKNOWN_LEVEL";
	}

	// Format input string
	va_start(Args, Format);
	vsnprintf(Buffer, BufSize, Format, Args);

	// Get current time and format it appropriately
	gettimeofday(&tv, NULL); 
	CurrentTime=tv.tv_sec;
	strftime(CurrentTimeBuffer,sizeof(CurrentTimeBuffer),"%F-%T",localtime(&CurrentTime));

	// Print the first part of the message
	fprintf(DestFile, "[%s.%06ld] [%s] [%s] ", CurrentTimeBuffer, tv.tv_usec, ModuleName, LoggingLevelString);


#ifdef _DEBUG
	fprintf(DestFile, "[%s:%d] %s\n", File, Line, Buffer);
#else
	fprintf(DestFile, "%s\n", Buffer);
#endif
};

