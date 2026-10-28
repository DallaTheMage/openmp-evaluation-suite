#!/usr/bin/env python3

#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Copyright (C) 2026 Lorenzo Dall'Ara
#
# This file is part of the OpenMP compiler-agnostic benchmark suite,
# developed as part of a Bachelor's thesis at the
# Alma Mater Studiorum - University of Bologna.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

"""
Shared configuration for benchmark analysis.
"""
COMPILERS = ('gcc', 'clang', 'icx',)
SCALING_TYPES = ('Full_scale', 'Proportional',)
DATASETS = ('raw_samples', 'aggregated_samples', 'scaling', 'test_plan',)
REQUIRED_DATASETS = ('aggregated_samples', 'test_plan',)
KEYS = ('test_set_id', 'test_case_id', 'kernel',)
CONFIG_COLUMNS = ('view_type', 'window_offset', 'window_size', 'chunk_size',)
COMPARISON_COLUMNS = (
    'view_type',
    'window_offset',
    'window_size',
    'num_threads',
    'chunk_size',
)
CORRELATION_METRICS = (
    'wall_time_sec_mean',
    'cycles_mean',
    'instructions_mean',
    'ipc_mean',
    'l1d_miss_ratio_mean',
    'llc_miss_ratio_mean',
    'energy_pkg_joules_mean',
    'energy_dram_joules_mean',
    'wall_time_cv_pct',
)

SCHEDULES = ('static', 'dynamic', 'guided')