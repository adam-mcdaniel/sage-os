#include <config.h>
#include <compiler.h>
#include <debug.h>
#include <stdarg.h>
#include <stdbool.h>
#include <csr.h>

static bool last_was_newline = true;

static int k_log_level = 0xFFF;

static const char *lgprefix(log_type lt) {
    switch (lt) {
        case LOG_DEBUG:
            return "[DEBUG]: ";
        case LOG_INFO:
            return "[INFO]: ";
        case LOG_WARN:
            return "[WARN]: ";
        case LOG_ERROR:
            return "[ERROR]: ";
        case LOG_FATAL:
            return "[FATAL]: ";
        default:
            return "";
    }
}

static int vlogf(log_type lt, const char *fmt, va_list args)
{
    if (!(lt & k_log_level)) {
        return 0;
    }
    int printf(const char *fmt, ...);
    if (last_was_newline) {
        printf("%s", lgprefix(lt));
    }
    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p == '\n') {
            last_was_newline = true;
        } else {
            last_was_newline = false;
        }
    }
    int vprintf_(const char *format, va_list va);
    return vprintf_(fmt, args);
}

int logf(log_type lt, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int ret = vlogf(lt, fmt, va);
    va_end(va);

    return ret;
}
int debugf(const char *fmt, ...)
{
#ifdef ENABLE_DEBUG
    va_list va;
    va_start(va, fmt);
    int ret = vlogf(LOG_DEBUG, fmt, va);
    va_end(va);

    return ret;
#endif
}

int warnf(const char *fmt, ...)
{
#ifdef ENABLE_WARN
    va_list va;
    va_start(va, fmt);
    int ret = vlogf(LOG_WARN, fmt, va);
    va_end(va);

    return ret;
#endif
}

int textf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int ret = vlogf(LOG_TEXT, fmt, va);
    va_end(va);

    return ret;
}

int infof(const char *fmt, ...)
{
#ifdef ENABLE_INFO
    va_list va;
    va_start(va, fmt);
    int ret = vlogf(LOG_INFO, fmt, va);
    va_end(va);

    return ret;
#endif
}

ATTR_NORET void fatalf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vlogf(LOG_FATAL, fmt, va);
    va_end(va);

    CSR_CLEAR("sstatus");
    WFI_LOOP();
}

void klogset(log_type lt)
{
    k_log_level |= lt;
}

void klogclear(log_type lt)
{
    k_log_level &= ~lt;
}