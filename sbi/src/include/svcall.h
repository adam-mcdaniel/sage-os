/**
 * @file svcall.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Supervisor call handler
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once
#include <sbicalls.h>

/**
 * @brief Handle a supervisor call on a given HART
 *
 * @param hart - the hart that received the ECALL from S-mode
 */
void svcall_handle(int hart);
