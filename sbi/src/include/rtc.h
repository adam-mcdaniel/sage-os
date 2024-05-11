/**
 * @file rtc.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief The realtime clock (RTC) routines.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

/**
 * @brief Get the RTC time. The first time this function
 * is called, the RTC will wake, but the return value is invalid.
 * 
 * @return unsigned long - the combined RTC value precise to nanoseconds
 */
unsigned long rtc_get_time(void);

