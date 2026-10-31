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
Controlled schedule and data-layout comparisons within one independent run.
Optimization labels/directories are intentionally ignored as experimental factors.
They are never paired or pooled by this module.
"""
import logging
from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from analysis.config import COMPARISON_COLUMNS
from analysis.columns import has_columns
from analysis.plotting import safe_name, save_figure

logger = logging.getLogger(__name__)


def _geomean(values: pd.Series) -> float:
    """All input ratios are strictly positive."""
    return float(np.exp(np.log(values).mean()))


def _pair(data: pd.DataFrame, dimension: str, reference: str) -> pd.DataFrame:
    """Pair only identical compiler/kernel/scaling/workload/thread configurations."""
    keys = ['compiler', 'scaling_type', 'kernel', *COMPARISON_COLUMNS]
    if dimension == 'view_type':
        keys.remove('view_type')
        keys.insert(2, 'schedule')
    required = keys + [dimension, 'wall_time_sec_mean']
    if not has_columns(data, *required):
        logger.warning('Skipping %s comparison: missing columns', dimension)
        return pd.DataFrame()
    df = data[required].copy()
    df['wall_time_sec_mean'] = pd.to_numeric(df['wall_time_sec_mean'], errors='coerce')
    df = df[np.isfinite(df.wall_time_sec_mean) & (df.wall_time_sec_mean > 0)]
    df = df.dropna(subset=keys + [dimension])
    if df.empty or reference not in df[dimension].unique():
        return pd.DataFrame()
    if df.duplicated(keys + [dimension]).any():
        logger.warning('Skipping %s comparison: duplicate experiment keys', dimension)
        return pd.DataFrame()
    wide = df.pivot(index=keys, columns=dimension, values='wall_time_sec_mean')
    if reference not in wide:
        return pd.DataFrame()
    alternatives = [value for value in wide.columns if value != reference]
    if not alternatives:
        return pd.DataFrame()
    rows = []
    for other in alternatives:
        aligned = wide[[reference, other]].dropna()
        if aligned.empty:
            continue
        paired = aligned.reset_index()
        paired[dimension] = other
        paired['reference'] = reference
        paired['reference_time_sec'] = paired[reference]
        paired['other_time_sec'] = paired[other]
        paired['speedup'] = paired['reference_time_sec'] / paired['other_time_sec']
        rows.append(paired[keys + [dimension, 'reference', 'reference_time_sec',
                                  'other_time_sec', 'speedup']])
    return pd.concat(rows, ignore_index=True) if rows else pd.DataFrame()


def _heatmap(summary: pd.DataFrame, col: str, title: str, path: Path, show: bool) -> None:
    if summary.empty:
        return
    matrix = summary.pivot(index='kernel', columns=col, values='speedup')
    if matrix.empty:
        return
    log2 = np.log2(matrix)
    extent = max(0.1, float(np.nanmax(np.abs(log2))))
    fig, ax = plt.subplots(figsize=(max(9, 1.35 * len(matrix.columns) + 4),
                                    max(5, 0.48 * len(matrix.index) + 2)))
    sns.heatmap(log2, ax=ax, annot=matrix.to_numpy(), fmt='.2f', cmap='RdBu_r',
                center=0, vmin=-extent, vmax=extent, mask=matrix.isna(),
                cbar_kws={'label': 'log₂(speedup), 0 = parity'})
    ax.set_xticks(np.arange(len(matrix.columns)) + 0.5,
                  labels=[str(x) for x in matrix.columns])
    ax.set_xlabel(col.replace('_', ' ').title())
    ax.set_ylabel('Kernel')
    ax.set_title(title)
    save_figure(fig, path, show)


def schedule_comparisons(data: pd.DataFrame, output: Path, show: bool = False) -> None:
    """Compare available scheduling strategies, preferably using static as reference."""
    if data.empty or 'schedule' not in data:
        return
    names = sorted(data.schedule.dropna().unique().tolist())
    if len(names) < 2:
        logger.info('Schedule comparison omitted: only one scheduling strategy available')
        return
    reference = 'static' if 'static' in names else names[0]
    paired = _pair(data, 'schedule', reference)
    if paired.empty:
        logger.info('Schedule comparison omitted: no equivalent configurations')
        return
    folder = output / 'schedule_comparisons'
    folder.mkdir(parents=True, exist_ok=True)
    paired.to_csv(folder / 'matched_schedule_runtime_ratios.csv', index=False)
    summary = (paired.groupby(['compiler', 'scaling_type', 'chunk_size', 'schedule',
                               'kernel', 'num_threads'], as_index=False)
               .agg(speedup=('speedup', _geomean), matched_configs=('speedup', 'size')))
    summary.to_csv(folder / 'schedule_geomean_by_threads.csv', index=False)
    for (compiler, mode, chunk, other), group in summary.groupby(
            ['compiler', 'scaling_type', 'chunk_size', 'schedule'], dropna=False):
        suffix = '_'.join(map(safe_name, (compiler, mode, f'chunk_{chunk}',
                                              f'{other}_vs_{reference}')))
        _heatmap(group, 'num_threads',
                 f'Scheduling: {other} vs {reference} (runtime ratio)\n'
                 f'{compiler} | {mode} | chunk={chunk} | >1: {other} faster',
                 folder / f'schedule_speedup_{suffix}.png', show)


def layout_comparisons(data: pd.DataFrame, output: Path, show: bool = False,
                       reference: str = '1D') -> None:
    """Pair alternative views against the 1D view for the same experiment.

    Plots show the maximum thread count per group; the CSV retains all counts.
    A missing 1D baseline means omission, not a fabricated substitution.
    """
    if data.empty or 'view_type' not in data or reference not in data.view_type.unique():
        logger.info('Layout comparison omitted: no %s baseline', reference)
        return
    paired = _pair(data, 'view_type', reference)
    if paired.empty:
        return
    folder = output / 'layout_comparisons'
    folder.mkdir(parents=True, exist_ok=True)
    paired.to_csv(folder / 'matched_layout_runtime_ratios.csv', index=False)
    summary = (paired.groupby(['compiler', 'schedule', 'scaling_type', 'chunk_size',
                               'num_threads', 'kernel', 'view_type'], as_index=False)
               .agg(speedup=('speedup', _geomean), matched_configs=('speedup', 'size')))
    summary.to_csv(folder / 'layout_geomean_by_threads.csv', index=False)
    for (compiler, schedule, mode, chunk), group in summary.groupby(
            ['compiler', 'schedule', 'scaling_type', 'chunk_size'], dropna=False):
        p = group.num_threads.max()
        subset = group[group.num_threads.eq(p)]
        suffix = '_'.join(map(safe_name, (compiler, schedule, mode, f'chunk_{chunk}')))
        _heatmap(subset, 'view_type',
                 f'Layout speedup vs {reference} at {p} threads\n'
                 f'{compiler} | {schedule} | {mode} | chunk={chunk}',
                 folder / f'layout_speedup_{suffix}.png', show)


def energy_tradeoffs(data: pd.DataFrame, output: Path, show: bool = False) -> None:
    """Energy/runtime/EDP normalized to valid serial baselines, fixed workload.

    Only strong scaling is comparable against a constant-work serial baseline.
    A missing energy observation removes the pair instead of becoming zero.
    """
    need = ['compiler', 'schedule', 'scaling_type', 'kernel', 'view_type',
            'chunk_size', 'window_offset', 'window_size', 'num_threads',
            'wall_time_sec_mean', 'energy_total', 'edp']
    if not has_columns(data, *need):
        return
    clean = data[data.scaling_type.eq('Full_scale')].copy()
    for field in ('wall_time_sec_mean', 'energy_total', 'edp', 'num_threads'):
        clean[field] = pd.to_numeric(clean[field], errors='coerce')
    clean = clean.replace([np.inf, -np.inf], np.nan).dropna(subset=need)
    clean = clean[(clean.wall_time_sec_mean > 0) & (clean.energy_total > 0) &
                  (clean.edp > 0) & (clean.num_threads >= 1)]
    keys = ['compiler', 'schedule', 'kernel', 'view_type', 'chunk_size',
            'window_offset', 'window_size']
    if clean.empty or clean.duplicated(keys + ['num_threads']).any():
        return
    baseline = clean[clean.num_threads.eq(1)][keys + ['wall_time_sec_mean',
                                                     'energy_total', 'edp']]
    if baseline.empty:
        return
    baseline = baseline.rename(columns={
        'wall_time_sec_mean': 'baseline_runtime_sec',
        'energy_total': 'baseline_energy_j',
        'edp': 'baseline_edp_js',
    })
    paired = clean.merge(baseline, on=keys, how='inner', validate='many_to_one')
    paired = paired[paired.num_threads.gt(1)].copy()
    if paired.empty:
        return
    paired['runtime_speedup'] = paired.baseline_runtime_sec / paired.wall_time_sec_mean
    paired['energy_gain'] = paired.baseline_energy_j / paired.energy_total
    paired['edp_gain'] = paired.baseline_edp_js / paired.edp
    folder = output / 'energy_tradeoffs'
    folder.mkdir(parents=True, exist_ok=True)
    paired.to_csv(folder / 'matched_energy_tradeoffs.csv', index=False)
    summary = (paired.groupby(['compiler', 'schedule', 'chunk_size', 'kernel',
                               'view_type', 'num_threads'], as_index=False)
               .agg(runtime_speedup=('runtime_speedup', _geomean),
                    energy_gain=('energy_gain', _geomean),
                    edp_gain=('edp_gain', _geomean),
                    matched_configs=('edp_gain', 'size')))
    summary.to_csv(folder / 'energy_tradeoffs_by_threads.csv', index=False)
    for (compiler, schedule, chunk), group in summary.groupby(
            ['compiler', 'schedule', 'chunk_size'], dropna=False):
        maximum = group.num_threads.max()
        selection = group[group.num_threads.eq(maximum)]
        fig, axes = plt.subplots(1, 3, figsize=(20, max(6, 0.45 * selection.kernel.nunique() + 2)))
        for ax, column, title in zip(axes,
                                     ('runtime_speedup', 'energy_gain', 'edp_gain'),
                                     ('Runtime speedup', 'Energy gain', 'EDP gain')):
            pivot = selection.pivot(index='kernel', columns='view_type', values=column)
            if pivot.empty:
                ax.set_axis_off()
                continue
            values = np.log2(pivot)
            extent = max(0.1, float(np.nanmax(np.abs(values))))
            sns.heatmap(values, ax=ax, annot=pivot.to_numpy(), fmt='.2f',
                        cmap='RdBu_r', center=0, vmin=-extent, vmax=extent,
                        mask=pivot.isna(),
                        cbar_kws={'label': 'log₂(gain), 0 = parity'})
            ax.set_title(title + ' (×, >1 = improvement)')
            ax.set_xlabel('View')
            ax.set_ylabel('Kernel' if ax is axes[0] else '')
        fig.suptitle(f'Strong scaling vs 1 thread | {compiler} / {schedule} | '
                     f'p={int(maximum)} | chunk={chunk}', y=1.02)
        suffix = '_'.join(map(safe_name, (compiler, schedule, f'chunk_{chunk}')))
        save_figure(fig, folder / f'energy_tradeoff_{suffix}.png', show)
