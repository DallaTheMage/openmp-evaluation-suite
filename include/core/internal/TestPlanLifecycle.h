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
 * @file TestPlanLifecycle.h
 * @brief Shared internal cleanup for test-plan construction and destruction.
 */

#ifndef CORE_INTERNAL_TEST_PLAN_LIFECYCLE_H
#define CORE_INTERNAL_TEST_PLAN_LIFECYCLE_H

#include "core/TestPlan.h"

/**
 * @brief Release all kernel groups, including partially built groups.
 * @param groups Kernel groups to release.
 */
void test_plan_destroy_kernel_groups(KernelTestGroupArray *groups);

#endif
