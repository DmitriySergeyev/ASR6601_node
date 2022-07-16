#include "syslog.h"
#include "stdarg.h"
#include "rtc-board.h"

#define LOGM_LMASK    (4 << 0) /* Log level mask */

static int Level = LOG_DEBUG;
static const char* const LogLevelStr[] = {"", "E", "W", "I", "D"};

bool IsPrintMessage(int level)
{
	if (level <= Level)
	{
		return true;
	}
	return false;
}

void wsyslog(int level, const char* format, ...)
{	
	va_list ap;

	va_start(ap, format);      /* Variable argument begin */
	if (IsPrintMessage(level) == true)
	{
		printf("%s:", LogLevelStr[level]);
		printf(format, ap);
		printf("\r\n");
	}
	va_end(ap);
}

void wsysdump(int level, const char *prefix, const void* buff, size_t size)
{
	if (IsPrintMessage(level) == true)
	{
		printf("%s:%s:", LogLevelStr[level], prefix);
		for (size_t i = 0; i < size; i++)
		{
			if ((i % 16) == 0)
			{
				printf("%04X", i);
			}
			printf(" %02X", ((char*)buff)[i]);
			if ((i % 16) == 15 || (i + 1) >= size)
			{
				printf("\r\n");
			}
		}
	}
}
