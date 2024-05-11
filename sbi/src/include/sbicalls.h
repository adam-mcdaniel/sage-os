/**
 * @file sbicalls.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief The supervisor calls. These numbers will need to match your OS.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#define SBI_SVCALL_HART_STATUS  (1)
#define SBI_SVCALL_HART_START   (2)
#define SBI_SVCALL_HART_STOP    (3)

#define SBI_SVCALL_GET_TIME     (4)
#define SBI_SVCALL_SET_TIMECMP  (5)
#define SBI_SVCALL_ADD_TIMECMP  (6)
#define SBI_SVCALL_ACK_TIMER    (7)
#define SBI_SVCALL_RTC_GET_TIME (8)

#define SBI_SVCALL_PUTCHAR      (9)
#define SBI_SVCALL_GETCHAR      (10)

#define SBI_SVCALL_WHOAMI       (11)

#define SBI_SVCALL_POWEROFF     (12)
