/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <machine/registerset.h>

const syscall_fault_message_t syscall_fault_message = { .msg = SYSCALL_MESSAGE };
const exception_fault_message_t exception_fault_message = { .msg = EXCEPTION_MESSAGE };
#ifdef CONFIG_KERNEL_MCS
const timeout_fault_message_t timeout_fault_message = { .msg = TIMEOUT_REPLY_MESSAGE };
#endif
