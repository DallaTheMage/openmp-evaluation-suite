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
from analysis.columns import has_columns, present_columns
from analysis.config import CORRELATION_METRICS

logger = logging.getLogger(__name__)
sns.set_theme(style='whitegrid')

def safe_name(value: object) -> str:
    return re.sub(r'[^A-Za-z0-9_.-]+', '_', str(value),)

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

def plot_correlations(df: pd.DataFrame, folder: Path, show: bool = False,) -> None:
    metrics = present_columns(df, CORRELATION_METRICS)
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

def plot_compiler_results(df: pd.DataFrame, compiler: str, schedule: str,
                          scaling: str, output_dir: Path, show: bool = False,
                          exploratory: bool = False, correlations: bool = False) -> None:
    """
    Optional exploratory charts; canonical scaling plots are in scaling.py.
    These figures span different workloads and thread counts, so they must not
    be interpreted as controlled comparisons. No incorrectly paired
    Proportional 'speedup' graph is produced.
    """
    folder = (output_dir / safe_name(compiler) / safe_name(schedule)
              / safe_name(scaling) / 'exploratory')
    if exploratory:
        boxplots = (
            ('wall_time_sec_mean', 'Execution time (mixed configurations)',
             'execution_time.png', True),
            ('wall_time_cv_pct', 'Variability (mixed configurations)',
             'variability.png', False),
            ('ipc_mean', 'IPC (mixed configurations)', 'ipc.png', False),
            ('edp', 'EDP (mixed configurations)', 'edp.png', True),
        )
        for metric, title, filename, log_scale in boxplots:
            plot_boxplot(df, metric, f'{title} - {compiler}/{scaling}',
                         folder / filename, log_scale, show)
        plot_energy_vs_time(df, folder, show)
        plot_cache_vs_time(df, folder, show)
    if correlations:
        plot_correlations(df, folder, show)


def per_compiler_plots(benchmarks: pd.DataFrame, output_dir: Path, schedule: str,
                       show: bool = False, exploratory: bool = False,
                       correlations: bool = False) -> None:
    """Generate optional figures for each available compiler and scaling type."""
    for (compiler, scaling), df in benchmarks.groupby(['compiler', 'scaling_type']):
        plot_compiler_results(df, str(compiler), str(schedule), str(scaling),
                              output_dir, show, exploratory, correlations)