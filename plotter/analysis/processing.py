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
Benchmark data preparation and metric computation.
"""
import logging
import numpy as np
import pandas as pd
from analysis.config import KEYS, REQUIRED_METRIC_COLUMNS, SCALING_TYPES

logger = logging.getLogger(__name__)

def flatten_index(df: pd.DataFrame,) -> pd.DataFrame:
    """
    Convert MultiIndex levels into regular columns.
    """
    df = df.copy()
    if isinstance(df.index, pd.MultiIndex):
        for level, name in enumerate(df.index.names):
            if name is not None and name not in df.columns:
                df[name] = df.index.get_level_values(level)
        df = df.reset_index(drop=True)
    return df

def merge_datasets(datasets: dict,) -> pd.DataFrame:
    """
    Merge aggregated benchmark samples with their test plans.
    """
    frames = []
    for compiler, entries in datasets.items():
        for scaling in SCALING_TYPES:
            if scaling not in entries:
                continue
            data         = entries[scaling]
            aggregated   = flatten_index(data['aggregated_samples'])
            plan         = flatten_index(data['test_plan'])
            missing_agg  = [key for key in KEYS
                            if key not in aggregated.columns]
            missing_plan = [key for key in KEYS
                            if key not in plan.columns]
            if missing_agg or missing_plan:
                logger.warning(
                    "Skipping %s/%s: missing merge keys "
                    "(aggregated=%s, plan=%s)",
                    compiler,
                    scaling,
                    missing_agg,
                    missing_plan,
                )
                continue
            extras = [column for column in plan.columns
                      if column not in aggregated.columns]
            plan   = plan[list(KEYS) + extras]
            try:
                merged = aggregated.merge(
                    plan,
                    on = list(KEYS),
                    how = 'left',
                    validate = 'one_to_one',
                )
            except pd.errors.MergeError as exc:
                logger.warning(
                    "Skipping %s/%s: invalid merge (%s)",
                    compiler,
                    scaling,
                    exc,
                )
                continue
            merged['compiler']     = compiler
            merged['scaling_type'] = scaling
            frames.append(merged)
    if not frames:
        raise ValueError("No usable benchmark datasets after merging")
    return pd.concat(frames, ignore_index=True,)

def normalize_numeric_columns(df: pd.DataFrame,) -> pd.DataFrame:
    """
    Convert aggregated metric columns to numeric values.
    """
    df = df.copy()
    suffixes       = ('_min', '_max', '_mean', '_variance',)
    target_columns = ('num_threads','chunk_size','window_size','window_offset',)
    for column in df.columns:
        if not column.endswith(suffixes):
            continue
        df[column] = pd.to_numeric(
            df[column]
            .astype(str)
            .str.replace(',', '.', regex=False),
            errors='coerce',
        )
    for column in target_columns:
        if column in df.columns:
            df[column] = pd.to_numeric(df[column], errors='coerce',)
    return df

def compute_metrics(df: pd.DataFrame,) -> pd.DataFrame:
    """
    Compute derived benchmark metrics.
    """
    df = df.copy()
    missing = [column for column in REQUIRED_METRIC_COLUMNS
               if column not in df.columns]
    if missing:
        raise ValueError(f"Missing required metric columns: {missing}")
    wall_time = df['wall_time_sec_mean'].replace(0, np.nan,)
    df['wall_time_std']    = np.sqrt(df['wall_time_sec_variance'].clip(lower = 0))
    df['wall_time_cv_pct'] = (100 * df['wall_time_std'] / wall_time)
    df['wall_time_range']  = (df['wall_time_sec_max'] - df['wall_time_sec_min'])
    energy_columns = ('energy_pkg_joules_mean', 'energy_dram_joules_mean',)
    if all(column in df.columns for column in energy_columns):
        df['energy_total'] = (df['energy_pkg_joules_mean'] + df['energy_dram_joules_mean'])
        df['edp']          = (df['energy_total'] * df['wall_time_sec_mean'])
        df['power_avg']    = (df['energy_total'] / wall_time)
    else:
        logger.warning("Energy metrics unavailable: missing energy columns")
        df['energy_total'] = np.nan
        df['edp']          = np.nan
        df['power_avg']    = np.nan
    return df

def prepare(datasets: dict,) -> pd.DataFrame:
    """
    Execute the complete data preparation pipeline.
    """
    df = merge_datasets(datasets)
    df = normalize_numeric_columns(df)
    df = compute_metrics(df)
    return df