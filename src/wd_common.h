#ifndef WD_COMMON_H
#define WD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef true
#undef true
#endif
#define true 1

#ifdef false
#undef false
#endif
#define false 0

#include <stdint.h>
typedef uint8_t   u8;
typedef  int8_t   i8;
typedef uint16_t u16;
typedef  int16_t i16;
typedef uint32_t u32;
typedef  int32_t i32;
typedef uint64_t u64;
typedef  int64_t i64;

typedef u32 b32;

typedef float r32;
typedef double r64;


#define wdMin(a,b)   ((a) < (b) ? (a) : (b))
#define wdMax(a,b)   ((a) > (b) ? (a) : (b))

#define wdSquare(a) ((a)*(a))
#define wdArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define WD_ANSI_COLOR_RED     "\x1b[31m"
#define WD_ANSI_COLOR_GREEN   "\x1b[32m"
#define WD_ANSI_COLOR_YELLOW  "\x1b[33m"
#define WD_ANSI_COLOR_BLUE    "\x1b[34m"
#define WD_ANSI_COLOR_MAGENTA "\x1b[35m"
#define WD_ANSI_COLOR_CYAN    "\x1b[36m"
#define WD_ANSI_COLOR_RESET   "\x1b[0m"

void wdPrefixPrint(const char *Prefix, const char *Format, ...)
{
    int PrefixSize = snprintf(0, 0, Prefix);
    va_list ArgsCount;
    va_start(ArgsCount, Format);
    int RemainSize = vsnprintf(0, 0, Format, ArgsCount) + 1;
    va_end(ArgsCount);

    va_list Args;
    va_start(Args, Format);
    int TotalSize = PrefixSize + RemainSize;
    char *TDSString = (char *)malloc(sizeof(char)*TotalSize);
    snprintf(TDSString, TotalSize, Prefix);
    vsnprintf(TDSString + PrefixSize, RemainSize, Format, Args);
    va_end(Args);

    char *TDSStringStart = TDSString;
    char *TDSLocation = strchr(TDSString,'\n');
    while (TDSLocation)
    {
        if (TDSLocation[1])
        {
            TDSLocation[0] = '\0';
            fprintf(stderr, TDSStringStart);
            fprintf(stderr, "\n%s", Prefix);
            TDSStringStart = TDSLocation + 1;
        }
        TDSLocation = strchr(TDSLocation+1,'\n');
    }
    fprintf(stderr, TDSStringStart);
    free(TDSString);
}

#define WD_ERROR_PREFIX WD_ANSI_COLOR_RED "[ERROR]" WD_ANSI_COLOR_RESET WD_ANSI_COLOR_YELLOW " (%s:%d) " WD_ANSI_COLOR_RESET 
#define wdErrorPrint(Format, ...)                                       \
    do                                                                  \
    {                                                                   \
        int Size = 1 + snprintf(0, 0, WD_ERROR_PREFIX, __FILE__, __LINE__); \
        char *Buffer = (char *)malloc(sizeof(char)*Size);               \
        snprintf(Buffer, Size, WD_ERROR_PREFIX , __FILE__, __LINE__);   \
        wdPrefixPrint(Buffer, Format, ##__VA_ARGS__);                   \
        free(Buffer);                                                   \
    } while(0);

#ifdef WD_INTERNAL
#define WD_INFO_PREFIX WD_ANSI_COLOR_GREEN "[INFO] " WD_ANSI_COLOR_RESET
#define wdInfoPrint(Format, ...) wdPrefixPrint(WD_INFO_PREFIX, Format, ##__VA_ARGS__)
#else
#define wdInfoPrint(...)
#endif

#ifdef WD_DEBUG
#define WD_DEBUG_PREFIX WD_ANSI_COLOR_CYAN "[DEBUG] " WD_ANSI_COLOR_RESET
#define wdDebugPrint(Format, ...) wdPrefixPrint(WD_DEBUG_PREFIX, Format, ##__VA_ARGS__)
#else
#define wdDebugPrint(...)
#endif

#endif
