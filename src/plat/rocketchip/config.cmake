#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

cmake_minimum_required(VERSION 3.7.2)

declare_platform(rocketchip KernelPlatformRocketchip PLAT_ROCKETCHIP KernelArchRiscV)

if(KernelPlatformRocketchip)
    declare_seL4_arch(riscv64)
    config_set(KernelRiscVPlatform RISCV_PLAT "rocketchip")
    config_set(KernelPlatformFirstHartID FIRST_HART_ID 0)
    list(APPEND KernelDTSList "tools/dts/rocketchip.dts")

    # Until extension B (bitmanip) is available, we use library functions to
    # count leading/trailing zeros.
    config_set(KernelClzNoBuiltin CLZ_NO_BUILTIN ON)
    config_set(KernelCtzNoBuiltin CTZ_NO_BUILTIN ON)

    declare_default_headers(
        TIMER_FREQUENCY 10000000llu PLIC_MAX_NUM_INT 0
        INTERRUPT_CONTROLLER arch/machine/plic.h
    )
else()
    unset(KernelPlatformFirstHartID CACHE)
endif()
