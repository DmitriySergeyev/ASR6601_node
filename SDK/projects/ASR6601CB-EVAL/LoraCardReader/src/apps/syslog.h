#ifndef SYSLOG_H_
#define SYSLOG_H_

#include <stdio.h>

#ifndef LOG_NOT
#define LOG_NOT     	0  /* Debug-level message */
#endif

#ifndef LOG_ERR
#define LOG_ERR       1  /* Error conditions */
#endif

#ifndef LOG_WARNING
#define LOG_WARNING   2  /* Warning conditions */
#endif

#ifndef LOG_INFO
#define LOG_INFO      3  /* Informational message */
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG     4  /* Debug-level message */
#endif

extern void wsyslog(int level, const char* format, ...);
extern void wsysdump(int level, const char *prefix, const void* buff, size_t size);
extern void wsyslevel(int level);

#define SYSLOG_E(FORMAT, ...) 		wsyslog(LOG_ERR,FORMAT, ##__VA_ARGS__)
#define SYSDUMP_E(PREFIX, BUFFER, LENGTH) wsysdump(LOG_ERR, PREFIX, BUFFER, LENGTH)

#define SYSLOG_W(FORMAT, ...) 		wsyslog(LOG_WARNING, FORMAT, ##__VA_ARGS__)
#define SYSDUMP_W(PREFIX, BUFFER, LENGTH) wsysdump(LOG_WARNING, PREFIX, BUFFER, LENGTH)

#define SYSLOG_I(FORMAT, ...) 		wsyslog(LOG_INFO, FORMAT, ##__VA_ARGS__)
#define SYSDUMP_I(PREFIX, BUFFER, LENGTH) wsysdump(LOG_INFO, PREFIX, BUFFER, LENGTH)

#define SYSLOG_D(FORMAT, ...) 		wsyslog(LOG_DEBUG, FORMAT, ##__VA_ARGS__)
#define SYSDUMP_D(PREFIX, BUFFER, LENGTH) wsysdump(LOG_DEBUG, PREFIX, BUFFER, LENGTH)



#endif