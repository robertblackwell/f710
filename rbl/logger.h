#ifndef rbl_logger_h
#define rbl_logger_h

#include <stdio.h>

extern "C" {
    void rbl_log_function(const char* level, const char* funcname, const char* filename, int line, char* message);
};

// RBL_LOG_ALLOW_GLOBAL - is a global switch to turn logging off globally

// RBL_LOG_ENABLED - each c/c++ file can turn logging on by defining CHLOG_ON before including rbl.h
// this only turns logging on if the global switch is defined
//
// RBL_LOG_ERROR - error logging is always on
//
#if defined(RBL_LOG_ENABLED) && defined(RBL_LOG_ALLOW_GLOBAL)
#define RBL_LOG_ERROR(f_, ...) do {\
		char* s; \
		asprintf(&s, f_, ##__VA_ARGS__); \
		rbl_log_function("ERR", __FUNCTION__, __FILE__, __LINE__, s); \
		free(s); \
	} while(0);

/* #define RBL_LOG_FMT(f_, ...) do {\
// 		char* s; \
// 		asprintf(&s, f_, ##__VA_ARGS__); \
// 		rbl_log_function("LOG", __FUNCTION__, __FILE__, __LINE__, s); \
// 		free(s); \
// 	} while(0);
*/

	#define RBL_LOG_PRINTF(f_, ...) printf((f_), ##__VA_ARGS__)
	#define RBL_LOG_FMT(f_, ...) do {\
		char* s; \
		asprintf(&s, f_, ##__VA_ARGS__); \
		rbl_log_function("LOG", __FUNCTION__, __FILE__, __LINE__, s); \
		free(s); \
	} while(0);

    #define RBL_LOG_ENTRY() do {\
		rbl_log_function("ENTR", __FUNCTION__, __FILE__, __LINE__, ""); \
	} while(0);

    #define RBL_LOG_MSG(m)  do {\
		rbl_log_function("MSG", __FUNCTION__, __FILE__, __LINE__, m); \
	} while(0);

#else
	#define RBL_LOG_PRINTF(f_, ...)
	#define RBL_LOG_FMT(f_, ...)
    #define RBL_LOG_ENTRY()
    #define RBL_LOG_MSG(m)
#endif
#endif