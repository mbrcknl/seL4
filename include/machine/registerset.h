/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <util.h>
#include <arch/types.h>
#include <arch/machine/registerset.h>
#include <arch/object/structures.h>

struct syscall_fault_message_global {
    register_t msg[n_syscallMessage];
};

typedef struct syscall_fault_message_global syscall_fault_message_t;
extern const syscall_fault_message_t syscall_fault_message VISIBLE;

struct exception_fault_message_global {
    register_t msg[n_exceptionMessage];
};

typedef struct exception_fault_message_global exception_fault_message_t;
extern const exception_fault_message_t exception_fault_message VISIBLE;

#ifdef CONFIG_KERNEL_MCS
struct timeout_fault_message_global {
    register_t msg[n_timeoutMessage];
};

typedef struct timeout_fault_message_global timeout_fault_message_t;
extern const timeout_fault_message_t timeout_fault_message VISIBLE;
#endif

static inline void setRegister(tcb_t *thread, register_t reg, word_t w)
{
    thread->tcbArch.tcbContext.registers[reg] = w;
}

static inline word_t PURE getRegister(tcb_t *thread, register_t reg)
{
    return thread->tcbArch.tcbContext.registers[reg];
}

#ifdef CONFIG_KERNEL_MCS
word_t getNBSendRecvDest(void);
#endif

