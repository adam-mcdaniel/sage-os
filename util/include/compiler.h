/**
 * @file compiler.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Compiler macros
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

// Attributes to support non-returning (NORET) or naked functions
#define ATTR_NAKED_NORET                   __attribute__((naked, noreturn))
#define ATTR_NAKED                         __attribute__((naked))
#define ATTR_NORET                         __attribute__((noreturn))
#define ATTR_WEAK                          __attribute__((weak))
