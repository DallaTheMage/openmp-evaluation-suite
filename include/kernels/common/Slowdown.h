/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2026 Lorenzo Dall'Ara
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite,
 * developed as part of a Bachelor's thesis at the
 * Alma Mater Studiorum - University of Bologna.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file Slowdown.h
 * @brief Optional extra arithmetic work for amplified kernel benchmarks.
 */

#ifndef OES_KERNELS_COMMON_SLOWDOWN_H
#define OES_KERNELS_COMMON_SLOWDOWN_H

#include <stdint.h>
#include "config/params.h"

#if SLOWDOWN_FACTOR > 0U
/**
 * @brief Execute extra local arithmetic for amplified benchmarks.
 * @param token Value used to initialize local arithmetic state.
 *
 * The volatile accumulator makes the loop observable to the compiler,
 * introducing extra local memory traffic rather than a precise delay.
 *
 * The volatile, thread-local accumulator makes every arithmetic iteration
 * observable to the C compiler, without touching benchmark output or shared
 * memory. It deliberately incurs local memory traffic; this is an amplified
 * microbenchmark, not a cycle-accurate CPU busy loop. */
static inline void slowdown_work(uint64_t token) {
    volatile double value = (double)(token & UINT64_C(1023)) + 1.0;
    for (uint64_t i = 0; i < SLOWDOWN_FACTOR; ++i) {
        value = value * 1.0000001192092896 + 0.0000001192092896;
    }
}
#define SLOWDOWN_AFTER_UPDATE(index) slowdown_work((uint64_t)(index))
#else
#define SLOWDOWN_AFTER_UPDATE(index) ((void)0)
#endif

#endif /* OES_KERNELS_COMMON_SLOWDOWN_H */
