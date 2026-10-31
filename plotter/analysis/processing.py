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
from analysis.columns import has_columns, missing_columns, present_columns
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
            missing_agg = missing_columns(aggregated, KEYS)
            missing_plan = missing_columns(plan, KEYS)
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
            extras = missing_columns(aggregated, plan.columns)
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
            # Averages can hide invalid readings. Require every raw observation
            # to have valid package/DRAM flags when raw samples are available.
            raw = data.get('raw_samples')
            if raw is not None and all(c in raw.columns for c in
                    (*KEYS, 'energy_pkg_valid', 'energy_dram_valid',
                     'energy_pkg_joules', 'energy_dram_joules')):
                validity = raw.copy()
                pkg_ok = (pd.to_numeric(validity['energy_pkg_valid'], errors='coerce').eq(1)
                          & pd.to_numeric(validity['energy_pkg_joules'], errors='coerce').gt(0))
                dram_ok = (pd.to_numeric(validity['energy_dram_valid'], errors='coerce').eq(1)
                           & pd.to_numeric(validity['energy_dram_joules'], errors='coerce').ge(0))
                validity['energy_run_valid'] = pkg_ok & dram_ok
                flags = (validity.groupby(list(KEYS), as_index=False)
                         .agg(energy_valid_runs=('energy_run_valid', 'sum'),
                              energy_checked_runs=('energy_run_valid', 'size'),
                              energy_all_valid=('energy_run_valid', 'all')))
                merged = merged.merge(flags, on=list(KEYS), how='left',
                                      validate='one_to_one')
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
    for column in present_columns(df, target_columns):
        df[column] = pd.to_numeric(df[column], errors='coerce')
    return df

def compute_metrics(df: pd.DataFrame,) -> pd.DataFrame:
    """
    Compute derived benchmark metrics.
    """
    df = df.copy()
    missing = missing_columns(df, REQUIRED_METRIC_COLUMNS)
    if missing:
        raise ValueError(f"Missing required metric columns: {missing}")
    wall_time = df['wall_time_sec_mean'].replace(0, np.nan,)
    df['wall_time_std']    = np.sqrt(df['wall_time_sec_variance'].clip(lower = 0))
    df['wall_time_cv_pct'] = (100 * df['wall_time_std'] / wall_time)
    df['wall_time_range']  = (df['wall_time_sec_max'] - df['wall_time_sec_min'])
    energy_columns = ('energy_pkg_joules_mean', 'energy_dram_joules_mean',)
    if has_columns(df, *energy_columns):
        pkg = pd.to_numeric(df['energy_pkg_joules_mean'], errors='coerce')
        dram = pd.to_numeric(df['energy_dram_joules_mean'], errors='coerce')
        # A zero or negative package reading is not a free measurement.
        # Zero DRAM is legitimate only if explicitly marked valid in raw data.
        valid = pkg.gt(0) & dram.ge(0) & np.isfinite(pkg) & np.isfinite(dram)
        fallback = valid.copy()
        # Fall back conservatively only for observations lacking raw validity.
        # Never override a negative/invalid flag with a positive mean.
        for name, positive in (('energy_pkg_joules_min', True),
                               ('energy_dram_joules_min', False)):
            if name in df:
                values = pd.to_numeric(df[name], errors='coerce')
                fallback &= values.gt(0) if positive else values.ge(0)
        if 'energy_all_valid' in df:
            checked = df['energy_all_valid'].notna()
            valid = valid & df['energy_all_valid'].eq(True)
            valid = valid.where(checked, fallback)
            if (~checked).any():
                logger.warning('%d rows lack raw energy flags: using conservative aggregate checks',
                               int((~checked).sum()))
        else:
            valid = fallback
            logger.warning('Raw energy validity flags unavailable: using conservative aggregate checks')
        valid &= pd.to_numeric(df['wall_time_sec_mean'], errors='coerce').gt(0)
        df['energy_valid'] = valid
        df['energy_total'] = (pkg + dram).where(valid, np.nan)
        df['edp'] = (df['energy_total'] * df['wall_time_sec_mean'])
        df['power_avg'] = (df['energy_total'] / wall_time)
        logger.info('Energy validation: %d/%d valid benchmark rows', int(valid.sum()), len(df))
    else:
        logger.warning("Energy metrics unavailable: missing energy columns")
        df['energy_valid'] = False
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