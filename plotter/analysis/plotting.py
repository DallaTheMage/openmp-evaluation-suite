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
Benchmark plotting utilities.
"""
import logging
import re
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from analysis.config import (CONFIG_COLUMNS, CORRELATION_METRICS,)

logger = logging.getLogger(__name__)
sns.set_theme(style='whitegrid')

def safe_name(value: object) -> str:
    return re.sub(r'[^A-Za-z0-9_.-]+', '_', str(value),)

def has_columns(df: pd.DataFrame, *columns: str,) -> bool:
    return all(column in df.columns for column in columns)

def save_figure(fig, path: Path, show: bool = False,) -> None:
    path.parent.mkdir(parents = True, exist_ok = True,)
    fig.tight_layout()
    fig.savefig(path, dpi=200, bbox_inches='tight',)
    if show:
        plt.show()
    plt.close(fig)
    logger.info("Saved plot: %s", path)

def plot_boxplot(
    df: pd.DataFrame,
    metric: str,
    title: str,
    path: Path,
    log_scale: bool = False,
    show: bool = False,
) -> None:
    if not has_columns(df, 'kernel', metric):
        return
    valid = (df.replace([np.inf, -np.inf], np.nan).dropna(subset=['kernel', metric]))
    if log_scale:
        valid = valid[valid[metric] > 0]
    if valid.empty:
        return
    fig, ax = plt.subplots(figsize=(14, 6))
    sns.boxplot(data = valid, x = 'kernel', y = metric, ax = ax,)
    if log_scale:
        ax.set_yscale('log')
    ax.tick_params(axis='x', labelrotation=45,)
    ax.set_title(title)
    save_figure(fig, path, show)

def plot_kernel_view_heatmap(df: pd.DataFrame, folder: Path, show: bool = False,) -> None:
    if not has_columns(df, 'kernel', 'view_type', 'wall_time_sec_mean',):
        return
    matrix = df.pivot_table(
        index = 'kernel',
        columns = 'view_type',
        values = 'wall_time_sec_mean',
        aggfunc = 'median',
    )
    if matrix.empty:
        return
    fig, ax = plt.subplots(figsize=(12, 8))
    sns.heatmap(matrix, annot = True, fmt = '.3g', cmap = 'viridis', ax = ax,)
    ax.set_title('Kernel vs view type')
    save_figure(fig, folder / 'kernel_view_type.png', show,)

def plot_energy_vs_time(df: pd.DataFrame, folder: Path, show: bool = False,) -> None:
    required = ('wall_time_sec_mean', 'energy_total', 'kernel',)
    if not has_columns(df, *required):
        return
    valid = df[(df['wall_time_sec_mean'] > 0)
               & (df['energy_total'] > 0)]
    if valid.empty:
        return
    fig, ax = plt.subplots(figsize=(12, 7))
    sns.scatterplot(
        data = valid,
        x = 'wall_time_sec_mean',
        y = 'energy_total',
        hue = 'kernel',
        alpha = 0.7,
        ax = ax,
    )
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_title('Energy vs execution time')
    save_figure(fig, folder / 'energy_vs_time.png', show,)

def plot_cache_vs_time(df: pd.DataFrame, folder: Path, show: bool = False,) -> None:
    required = ('wall_time_sec_mean', 'l1d_miss_ratio_mean', 'kernel',)
    if not has_columns(df, *required):
        return
    valid = df[df['wall_time_sec_mean'] > 0
    ].dropna(subset=['l1d_miss_ratio_mean'])
    if valid.empty:
        return
    fig, ax = plt.subplots(figsize=(12, 7))
    sns.scatterplot(
        data = valid,
        x = 'l1d_miss_ratio_mean',
        y = 'wall_time_sec_mean',
        hue = 'kernel',
        alpha = 0.7,
        ax = ax,
    )
    ax.set_yscale('log')
    ax.set_title('Cache misses vs execution time')
    save_figure(fig,folder / 'cache_vs_time.png',show,)

def plot_chunk_size(df: pd.DataFrame, folder: Path, show: bool = False,) -> None:
    required = ('kernel', 'chunk_size', 'num_threads', 'wall_time_sec_mean',)
    if not has_columns(df, *required):
        return
    valid = df[
        (df['num_threads'] == 8)
        & df['chunk_size'].notna()
    ]
    if valid.empty:
        return
    grid = sns.relplot(
        data=valid,
        x='chunk_size',
        y='wall_time_sec_mean',
        col='kernel',
        col_wrap=3,
        hue=(
            'view_type'
            if 'view_type' in valid.columns
            else None
        ),
        kind='line',
        marker='o',
        facet_kws={
            'sharey': False,
        },
        height=3.5,
    )
    save_figure(grid.figure, folder / 'chunk_size.png', show,)

def compute_scaling(df: pd.DataFrame,) -> pd.DataFrame:
    required = ('kernel', 'num_threads', 'wall_time_sec_mean',)
    if not has_columns(df, *required):
        return pd.DataFrame()
    config = [column for column in CONFIG_COLUMNS
              if column in df.columns]
    frames = []
    for kernel, kernel_df in df.groupby('kernel'):
        valid = kernel_df.dropna(subset=config + ['num_threads', 'wall_time_sec_mean',])
        if valid.empty:
            continue
        group_columns = config + ['num_threads']
        grouped = (
            valid.groupby(group_columns, dropna=False, as_index=False,)['wall_time_sec_mean']
            .mean()
        )
        baseline = grouped[
            grouped['num_threads'] == 1
        ][
            config + ['wall_time_sec_mean']
        ].rename(
            columns={'wall_time_sec_mean': 'baseline'}
        )
        if baseline.empty:
            continue
        if config:
            grouped = grouped.merge(baseline, on = config, how = 'inner',)
        else:
            grouped['baseline'] = (baseline['baseline'].iloc[0])
        grouped = grouped[
            (grouped['wall_time_sec_mean'] > 0)
            & (grouped['num_threads'] > 0)
            & (grouped['baseline'] > 0)
        ].copy()
        if grouped['num_threads'].nunique() < 2:
            continue
        grouped['speedup'] = (grouped['baseline'] / grouped['wall_time_sec_mean'])
        grouped['efficiency'] = (grouped['speedup'] / grouped['num_threads'])
        if config:
            grouped['configuration'] = (grouped[config] .astype(str) .agg(' | '.join, axis=1))
        else:
            grouped['configuration'] = 'default'
        grouped['kernel'] = kernel
        frames.append(grouped)
    if not frames:
        return pd.DataFrame()
    return pd.concat(frames, ignore_index=True,)

def plot_scaling(
    df: pd.DataFrame,
    folder: Path,
    compiler: str,
    scaling: str,
    show: bool = False,
) -> None:
    scaling_data = compute_scaling(df)
    if scaling_data.empty:
        return
    for kernel, kernel_df in scaling_data.groupby('kernel'):
        for metric in ('speedup', 'efficiency'):
            fig, ax = plt.subplots(figsize=(11, 6))
            sns.lineplot(
                data = kernel_df,
                x = 'num_threads',
                y = metric,
                hue = 'configuration',
                estimator = None,
                marker = 'o',
                ax = ax,
            )
            if metric == 'speedup':
                threads = sorted(kernel_df['num_threads'].unique())
                ax.plot(threads, threads, 'k--', label='Ideal',)
            else:
                ax.axhline(1, color='red', linestyle='--', label='Ideal',)
            ax.set_title(f'{metric.title()} - ' f'{kernel} - {compiler}/{scaling}')
            ax.legend(bbox_to_anchor = (1.05, 1), loc = 'upper left',)
            save_figure(fig, folder / (f'{metric}_{safe_name(kernel)}.png'), show,)

def plot_correlations(df: pd.DataFrame, folder: Path, show: bool = False,) -> None:
    metrics = [column for column in CORRELATION_METRICS
               if column in df.columns]
    if len(metrics) < 2:
        return
    corr = df[metrics].corr(method='spearman')
    if corr.empty or corr.isna().all().all():
        return
    fig, ax = plt.subplots(figsize=(11, 9))
    sns.heatmap(
        corr,
        annot = True,
        fmt = '.2f',
        cmap = 'coolwarm',
        center = 0,
        vmin = -1,
        vmax = 1,
        ax = ax,
    )
    ax.set_title('Spearman correlations')
    save_figure(fig, folder / 'correlation.png', show,)

def plot_compiler_results(df: pd.DataFrame, compiler: str, schedule:str, scaling: str, output_dir: Path, show: bool = False,) -> None:
    """
    Generate all plots for a compiler/scaling combination.
    """
    folder = (output_dir / safe_name(compiler) / safe_name(schedule) / safe_name(scaling))
    logger.info("Generating plots for %s/%s (%d rows)", compiler, scaling, len(df),)
    boxplots = (
        ('wall_time_sec_mean', 'Execution time', 'execution_time.png', True,),
        ('wall_time_cv_pct', 'Variability', 'variability.png', False,),
        ('ipc_mean', 'IPC', 'ipc.png', False,),
        ('edp', 'EDP', 'edp.png', True,),
    )
    for metric, title, filename, log_scale in boxplots:
        plot_boxplot(
            df = df,
            metric = metric,
            title = f'{title} - {compiler}/{scaling}',
            path = folder / filename,
            log_scale = log_scale,
            show = show,
        )
    plot_kernel_view_heatmap(df, folder, show)
    plot_energy_vs_time(df, folder, show)
    plot_cache_vs_time(df, folder, show)
    plot_chunk_size(df, folder, show)
    plot_scaling(df, folder, compiler, scaling, show,)
    plot_correlations(df, folder, show)

def per_compiler_plots(benchmarks: pd.DataFrame, output_dir: Path, schedule: str, show: bool = False,) -> None:
    """
    Generate plots for every available compiler/scaling pair.
    """
    for (compiler, scaling), df in benchmarks.groupby(['compiler', 'scaling_type']):
        plot_compiler_results(df, str(compiler), str(schedule), str(scaling), output_dir, show,)