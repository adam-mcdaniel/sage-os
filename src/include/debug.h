/**
 * @file debug.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Debug log routines.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

typedef enum {
    LOG_ERROR = 1 << 0,
    LOG_DEBUG = 1 << 1,
    LOG_INFO  = 1 << 2,
    LOG_FATAL = 1 << 3,
    LOG_WARN  = 1 << 4,
    LOG_TEXT  = 1 << 5,
} log_type;

int  logf(log_type lt, const char *fmt, ...);
int  debugf(const char *fmt, ...);
int  infof(const char *fmt, ...);
int  warnf(const char *fmt, ...);
int  textf(const char *fmt, ...);
void fatalf(const char *fmt, ...);
void logset(log_type lt);
void logclear(log_type lt);


#define ENABLE_DEBUG
// #ifdef ENABLE_DEBUG
// #define debugf(...) debugf(__VA_ARGS__)
// #else
// #define debugf(...)
// #endif


#define ENABLE_INFO
// #ifdef ENABLE_INFO
// #define infof(...) infof(__VA_ARGS__)
// #else
// #define infof(...)
// #endif

#define ENABLE_WARN
// #ifdef ENABLE_WARN
// #define warnf(...) warnf(__VA_ARGS__)
// #else
// #define warnf(...)
// #endif

// #define ENABLE_TEXT
// #ifdef ENABLE_TEXT
// #define textf(...) textf(__VA_ARGS__)
// #else
// #define textf(...)
// #endif