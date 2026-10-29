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

import logging
from pathlib import Path
from matplotlib.colors import LogNorm
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

from analysis.columns import has_columns, missing_columns, present_columns
from analysis.plotting import safe_name, save_figure
from analysis.config import SCALING_NUMERIC

logger = logging.getLogger(__name__)

def _reconstruct(df: pd.DataFrame, mode: str) -> pd.DataFrame:
    needed = {'kernel', 'view_type', 'num_threads', 'window_size', 'wall_time_sec_mean'}
    absent = missing_columns(df, sorted(needed))
    if absent:
        logger.warning('Cannot reconstruct %s scaling: missing %s', mode, absent)
        return pd.DataFrame()
    data = df.copy()
    for c in ('num_threads', 'window_size', 'wall_time_sec_mean'):
        data[c] = pd.to_numeric(data[c], errors='coerce')
    data = data.dropna(subset=list(needed))
    data = data[(data.num_threads > 0) & (data.window_size > 0) & (data.wall_time_sec_mean > 0)]
    match = ['kernel', 'view_type', *present_columns(data, ('chunk_size', 'window_offset'))]
    if mode == 'Full_scale':
        match.append('window_size')
    else:
        data['work_per_thread'] = data['window_size'] / data['num_threads']
        match.append('work_per_thread')
    if data.duplicated(match + ['num_threads']).any():
        logger.warning('Cannot reconstruct %s scaling: ambiguous repeated configurations', mode)
        return pd.DataFrame()
    baseline_columns = match + ['wall_time_sec_mean']
    if mode != 'Full_scale':
        baseline_columns.append('window_size')
    base = data[data.num_threads == 1][baseline_columns].copy()
    if base.empty:
        logger.warning('Cannot reconstruct %s scaling: no one-thread baseline', mode)
        return pd.DataFrame()
    base = base.rename(columns={'wall_time_sec_mean': 'baseline_time_sec'})
    if mode != 'Full_scale':
        base = base.rename(columns={'window_size': 'baseline_work_elements'})
    merged = data.merge(base, on=match, validate='many_to_one', how='inner')
    if merged.empty:
        return pd.DataFrame()
    merged = merged.rename(columns={'num_threads': 'scaled_threads',
                                    'wall_time_sec_mean': 'scaled_time_sec',
                                    'window_size': 'scaled_work_elements'})
    merged['baseline_threads'] = 1
    if mode == 'Full_scale':
        merged['baseline_work_elements'] = merged['scaled_work_elements']
    merged['work_scale'] = merged.scaled_work_elements / merged.baseline_work_elements
    return merged

def prepare_scaling(source: pd.DataFrame | None, benchmarks: pd.DataFrame, mode: str) -> pd.DataFrame:
    expected = 'strong' if mode == 'Full_scale' else 'weak'
    if source is None or source.empty:
        logger.info('No scaling.csv for %s: reconstructing from measurements', mode)
        df = _reconstruct(benchmarks, mode)
    else:
        df = source.copy()
        if 'scaling_type' in df:
            df = df[df.scaling_type.eq(expected)]
    required = {'kernel', 'view_type', 'baseline_threads', 'scaled_threads',
                'baseline_time_sec', 'scaled_time_sec'}
    absent = missing_columns(df, sorted(required))
    if df.empty or absent:
        logger.warning('%s: no valid scaling rows (missing %s)', mode, absent)
        return pd.DataFrame()
    for column in present_columns(df, SCALING_NUMERIC):
        df[column] = pd.to_numeric(df[column], errors='coerce')
    df = df.replace([np.inf, -np.inf], np.nan).dropna(subset=list(required)).copy()
    df = df[(df.baseline_threads > 0) & (df.scaled_threads >= df.baseline_threads)
            & (df.baseline_time_sec > 0) & (df.scaled_time_sec > 0)]
    if df.empty:
        return df
    ratio = df.scaled_threads / df.baseline_threads
    if 'work_scale' not in df:
        if has_columns(df, 'baseline_work_elements', 'scaled_work_elements'):
            df['work_scale'] = df.scaled_work_elements / df.baseline_work_elements
        else:
            df['work_scale'] = np.nan
    work = df.work_scale
    if expected == 'strong':
        valid = work.isna() | np.isclose(work, 1, rtol=1e-8)
        if has_columns(df, 'baseline_work_elements', 'scaled_work_elements'):
            valid &= np.isclose(df.baseline_work_elements, df.scaled_work_elements, rtol=1e-8)
        df['work_scale'] = 1.0
    else:
        valid = work.notna() & np.isclose(work, ratio, rtol=1e-8)
        if has_columns(df, 'baseline_work_elements', 'scaled_work_elements'):
            valid &= (df.baseline_work_elements > 0) & np.isclose(
                df.scaled_work_elements / df.baseline_work_elements, ratio, rtol=1e-8)
    removed = int((~valid).sum())
    if removed:
        logger.warning('Excluded %d %s scaling rows: workload/thread ratio is not valid', removed, expected)
    df = df[valid].copy()
    if df.empty:
        return df
    ratio = df.scaled_threads / df.baseline_threads
    expected_speedup = df.work_scale * df.baseline_time_sec / df.scaled_time_sec
    expected_efficiency = expected_speedup / ratio
    ideal_time = df.baseline_time_sec * df.work_scale / ratio
    expected_overhead = df.scaled_time_sec - ideal_time
    for col, correct in (('speedup', expected_speedup), ('efficiency', expected_efficiency),
                         ('overhead_sec', expected_overhead)):
        if col in df:
            mismatches = (~np.isclose(df[col], correct, rtol=1e-6, atol=1e-12) & df[col].notna()).sum()
            if mismatches:
                logger.warning('%s: corrected %d inconsistent %s values', mode, mismatches, col)
        df[col] = correct
    df['ideal_time_sec'] = ideal_time
    df['efficiency_pct'] = 100.0 * df.efficiency
    df['overhead_relative_pct'] = 100.0 * df.overhead_sec / ideal_time
    df['test_mode'] = mode
    return df.sort_values(['view_type', 'kernel', 'scaled_threads']).reset_index(drop=True)

def plot_efficiency_heatmaps(df: pd.DataFrame, folder: Path, mode: str, show: bool = False) -> None:
    valid = df[(df.scaled_threads > df.baseline_threads)].copy()
    if valid.empty:
        return
    threads = sorted(valid.scaled_threads.unique())
    views   = sorted(valid.view_type.dropna().unique())
    kernels = sorted(valid.kernel.dropna().unique())
    if not views or not kernels:
        return
    fig, axes = plt.subplots(1, len(threads), figsize=(max(8, 6 * len(threads)),
                              max(7, .5 * len(kernels) + 2)), squeeze=False)
    for ax, p in zip(axes[0], threads):
        chunk = valid[valid.scaled_threads.eq(p)]
        if chunk.duplicated(['kernel', 'view_type']).any():
            logger.warning('%s: efficiency heatmap at p=%s omitted (multiple chunks/configs)', mode, p)
            ax.set_axis_off()
            ax.set_title(f'{p} threads: multiple configurations')
            continue
        mat = chunk.pivot(index='kernel', columns='view_type', values='efficiency_pct')
        mat = mat.reindex(index=kernels, columns=views)
        sns.heatmap(mat, ax=ax, annot=True, fmt='.0f', cmap='YlGnBu',
                    vmin=0, vmax=max(100, float(valid.efficiency_pct.max())),
                    cbar=ax is axes[0, -1], mask=mat.isna(),
                    annot_kws={'fontsize': 8})
        ax.set_title(f'{int(p)} threads')
        ax.set_xlabel('View')
        ax.set_ylabel('Kernel' if ax is axes[0, 0] else '')
    fig.suptitle(f'{"Strong" if mode == "Full_scale" else "Weak"} scaling efficiency (%) by kernel/view', y=1.01)
    save_figure(fig, folder / 'efficiency_by_threads.png', show)

def plot_scaling_and_overhead(df: pd.DataFrame, folder: Path, mode: str, show: bool = False) -> None:
    for view, part in df.groupby('view_type'):
        if part.scaled_threads.nunique() < 2:
            continue
        options = ('chunk_size', 'window_offset', 'window_size', 'baseline_work_elements')
        labels = ['kernel'] + [
            column for column in present_columns(part, options)
            if part[column].nunique(dropna=False) > 1
        ]
        part = part.copy()
        part[labels] = part[labels].fillna('N/A')
        part['series'] = part[labels].astype(str).agg(' · '.join, axis=1)
        if part.duplicated(['series', 'scaled_threads']).any():
            logger.warning('Skipping scaling plot for %s: ambiguous repeated series', view)
            continue
        if part.baseline_threads.nunique() != 1:
            logger.warning('Skipping view %s: different baseline thread counts', view)
            continue
        fig, (top, bottom) = plt.subplots(2, 1, sharex=True, figsize=(12.5, 9),
                                         gridspec_kw={'height_ratios': [1, 1]})
        series_groups = list(part.groupby('series', sort=True))
        colors = sns.color_palette('tab20', n_colors=len(series_groups))
        for (label, series), color in zip(series_groups, colors):
            series = series.sort_values('scaled_threads')
            if mode == 'Full_scale':
                ys = series.speedup
            else:
                ys = series.efficiency_pct
            (line,) = top.plot(series.scaled_threads, ys, marker='o', markersize=4,
                               linewidth=1.5, color=color, label=label)
            bottom.plot(series.scaled_threads, series.overhead_relative_pct,
                        marker='o', markersize=4, linewidth=1.5,
                        color=line.get_color(), label=label)
        ticks = sorted(part.scaled_threads.unique())
        if mode == 'Full_scale':
            baseline = part.baseline_threads.iloc[0]
            top.plot(ticks, np.array(ticks) / baseline, '--', color='black',
                     linewidth=1.6, label='Ideal')
            top.set_ylabel('Strong speedup (×)')
        else:
            top.axhline(100, linestyle='--', color='black', linewidth=1.6, label='Ideal')
            top.set_ylabel('Weak efficiency (%)')
        bottom.axhline(0, linestyle='--', color='black', linewidth=1.2)
        bottom.set_ylabel('Extra time vs ideal (%)')
        ov = part.overhead_relative_pct.abs()
        if ov.max() > 500:
            bottom.set_yscale('symlog', linthresh=10)
        bottom.set_xlabel('Number of threads')
        bottom.set_xticks(ticks)
        top.set_title(f'{"Strong speedup" if mode == "Full_scale" else "Weak efficiency"} and overhead · {view}')
        handles, names = top.get_legend_handles_labels()
        fig.legend(handles, names, title='Kernel / configuration', loc='center left',
                   bbox_to_anchor=(1.005, .5), fontsize='small')
        save_figure(fig, folder / f'scaling_overhead_{safe_name(view)}.png', show)

def export_scaling(df: pd.DataFrame, folder: Path) -> None:
    if df.empty:
        return
    folder.mkdir(parents=True, exist_ok=True)
    df.to_csv(folder / 'scaling_enriched.csv', index=False)

def generate_scaling_plots(df: pd.DataFrame, folder: Path, mode: str,
                           show: bool = False) -> None:
    if df.empty:
        return
    export_scaling(df, folder)
    plot_efficiency_heatmaps(df, folder, mode, show)
    plot_scaling_and_overhead(df, folder, mode, show)

def plot_fixed_thread_metrics(benchmarks: pd.DataFrame, folder: Path, mode: str, show: bool = False) -> None:
    if benchmarks.empty or 'num_threads' not in benchmarks:
        return
    p = pd.to_numeric(benchmarks.num_threads, errors='coerce').max()
    subset = benchmarks[pd.to_numeric(benchmarks.num_threads, errors='coerce').eq(p)].copy()
    required = {'kernel', 'view_type', 'window_size', 'wall_time_sec_mean'}
    if subset.empty or not has_columns(subset, *required):
        return
    if subset.window_size.nunique() != 1:
        logger.warning('%s p=%s: different workloads; skipping absolute metric heatmaps', mode, p)
        return
    if subset.duplicated(['kernel', 'view_type']).any():
        logger.warning('%s p=%s: repeated kernel/view configurations; skipping absolute heatmaps', mode, p)
        return

    def metric_axes(ax, metric: str, title: str):
        if metric not in subset:
            ax.set_axis_off()
            return
        valid = subset[subset[metric] > 0]
        if valid.empty:
            ax.set_axis_off()
            return
        mat = valid.pivot(index='kernel', columns='view_type', values=metric)
        if mat.empty:
            ax.set_axis_off()
            return
        mat = mat.reindex(index=sorted(subset.kernel.unique()),
                            columns=sorted(subset.view_type.unique()))
        low = mat.min().min()
        high = mat.max().max()
        norm = LogNorm(vmin=low, vmax=high if high > low else high * 1.00001)
        sns.heatmap(mat, ax=ax, cmap='viridis', norm=norm,
                    mask=mat.isna(), cbar_kws={'label': title})
        ax.set_title(title)
        ax.set_xlabel('View')
        ax.set_ylabel('Kernel')

    fig, ax = plt.subplots(figsize=(10, 9))
    metric_axes(ax, 'wall_time_sec_mean', 'Wall time (s, log scale)')
    fig.suptitle(f'{mode}: {int(p)} threads, {int(subset.window_size.iloc[0]):,} elements')
    save_figure(fig, folder / 'execution_time_fixed_threads.png', show)

    if has_columns(subset, 'energy_total', 'edp') and (subset.energy_total > 0).any():
        fig, axes = plt.subplots(1, 2, figsize=(18, 9))
        metric_axes(axes[0], 'energy_total', 'Total measured energy (J, log scale)')
        metric_axes(axes[1], 'edp', 'Energy-delay product (J·s, log scale)')
        fig.suptitle(f'{mode}: {int(p)} threads, {int(subset.window_size.iloc[0]):,} elements')
        save_figure(fig, folder / 'energy_edp_fixed_threads.png', show)
