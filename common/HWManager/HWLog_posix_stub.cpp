#include "HWLog.h"
#include <cstdarg>
#include <cstdio>

void HWLog::SetLogDir(char* strDir) { }

void HWLog::Write(char* strLog, ...)
{
    va_list ap; va_start(ap, strLog);
    vfprintf(stderr, strLog, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void HWLog::Write(wchar_t* strLog, ...)
{
    // naive wide->narrow conversion
    va_list ap; va_start(ap, strLog);
    // Not attempting wide formatting; just print a placeholder
    fprintf(stderr, "(wlog) ");
    va_end(ap);
}

void HWLog::WriteK(char* strKey, char* strLog, ...)
{
    va_list ap; va_start(ap, strLog);
    fprintf(stderr, "[%s] ", strKey);
    vfprintf(stderr, strLog, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void HWLog::WriteK(wchar_t* strKey, wchar_t* strLog, ...)
{
    va_list ap; va_start(ap, strLog);
    fprintf(stderr, "(wlogk) ");
    va_end(ap);
}
