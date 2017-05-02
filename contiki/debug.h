#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdio.h>

#define LOG_LEVEL_DEBUG		0
#define LOG_LEVEL_INFO		1
#define LOG_LEVEL_WARNING	2
#define LOG_LEVEL_ERROR		3
#define LOG_LEVEL_DISABLED	4

#define DEBUG_HEADER()             printf("[%s(): DEBUG] ", __FUNCTION__)
#define INFO_HEADER()              printf("[%s(): INFO] ", __FUNCTION__)
#define WARNING_HEADER()           printf("[%s(): WARNING] ", __FUNCTION__)
#define ERROR_HEADER()             printf("[%s(): ERROR] ", __FUNCTION__)
#define NOT_IMPLEMENTED_HEADER()   printf("[%s(): NOT IMPLEMENTED] ", __FUNCTION__)

#define DEBUG_ENDL()               printf("\r\n")

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif // LOG_LEVEL

#if (LOG_LEVEL <= LOG_LEVEL_DEBUG)
#define log_debug(...) do {DEBUG_HEADER(); printf(__VA_ARGS__);DEBUG_ENDL();}while(0)
#else // (LOG_LEVEL <= LOG_LEVEL_DEBUG)
#define log_debug(...)
#endif // (LOG_LEVEL <= LOG_LEVEL_DEBUG)

#if (LOG_LEVEL <= LOG_LEVEL_INFO)
#define log_info(...) do {INFO_HEADER(); printf(__VA_ARGS__);DEBUG_ENDL();}while(0)
#else // (LOG_LEVEL <= LOG_LEVEL_INFO)
#define log_info(...)
#endif // (LOG_LEVEL <= LOG_LEVEL_INFO)

#if (LOG_LEVEL <= LOG_LEVEL_WARNING)
#define log_warning(...) do {WARNING_HEADER(); printf(__VA_ARGS__);DEBUG_ENDL();}while(0)
#else // (LOG_LEVEL <= LOG_LEVEL_INFO)
#define log_warning(...)
#endif // (LOG_LEVEL <= LOG_LEVEL_INFO)

#if (LOG_LEVEL <= LOG_LEVEL_ERROR)
#define log_error(...) do {ERROR_HEADER(); printf(__VA_ARGS__);DEBUG_ENDL();}while(0)
#define log_not_implemented(...) do {NOT_IMPLEMENTED_HEADER(); printf(__VA_ARGS__);DEBUG_ENDL();}while(0)
#else // (LOG_LEVEL <= LOG_LEVEL_ERROR)
#define log_error(...)
#define log_not_implemented(...)
#endif // (LOG_LEVEL <= LOG_LEVEL_ERROR)

#endif // !__DEBUG_H__
