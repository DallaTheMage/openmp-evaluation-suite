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
Compiler performance comparisons.
"""

import logging
from itertools import combinations
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

from analysis.config import COMPARISON_COLUMNS
from analysis.plotting import save_figure, safe_name

logger = logging.getLogger(__name__)

def pair_compiler_results(benchmarks: pd.DataFrame, reference: str, other: str,) -> pd.DataFrame:
    """
    Match equivalent benchmark configurations and compute speedup.
    """
    keys = ['scaling_type', 'kernel',]
    keys.extend(column for column in COMPARISON_COLUMNS
                if column in benchmarks.columns)
    required = keys + ['compiler', 'wall_time_sec_mean',]
    missing = [column for column in required
               if column not in benchmarks.columns]
    if missing:
        raise ValueError(f"Missing comparison columns: {missing}")
    df = benchmarks[benchmarks['compiler'].isin([reference, other])
    ].dropna(subset=['wall_time_sec_mean']).copy()
    if df.duplicated(keys + ['compiler']).any():
        logger.warning( "Cannot compare %s and %s: duplicate benchmark keys", reference, other,)
        return pd.DataFrame()
    pivot = df.pivot(index = keys, columns = 'compiler', values = 'wall_time_sec_mean',)
    if not {reference, other,}.issubset(pivot.columns):
        return pd.DataFrame()
    paired = pivot[[reference, other]].dropna()
    paired = paired[(paired[reference] > 0) & (paired[other] > 0)].copy()
    if paired.empty:
        return pd.DataFrame()
    paired['speedup'] = (paired[reference] / paired[other])
    return paired.reset_index()

def compute_geometric_speedup(paired: pd.DataFrame,) -> pd.DataFrame:
    """
    Compute geometric mean speedup by kernel and scaling type.
    """
    if paired.empty:
        return pd.DataFrame()
    return (
        paired.groupby(['kernel', 'scaling_type'])['speedup']
        .agg(lambda values: np.exp(np.log(values).mean()))
        .reset_index()
    )

def plot_compiler_speedup(
    paired: pd.DataFrame,
    reference: str,
    other: str,
    folder: Path,
    show: bool = False,
) -> None:
    if paired.empty:
        return
    fig, ax = plt.subplots(figsize=(14, 6))
    sns.boxplot(data = paired, x = 'kernel', y = 'speedup', hue = 'scaling_type', ax = ax,)
    ax.axhline(1, color = 'red', linestyle = '--',)
    ax.set_yscale('log')
    ax.tick_params(axis = 'x', labelrotation = 45,)
    ax.set_title(f'Speedup {other} vs {reference} (>1: {other} faster)')
    save_figure(
        fig,
        folder / (f'speedup_{safe_name(other)}_vs_{safe_name(reference)}.png'),
        show,
    )

def plot_geometric_speedup(
    summary: pd.DataFrame,
    reference: str,
    other: str,
    folder: Path,
    show: bool = False,
) -> None:
    if summary.empty:
        return
    fig, ax = plt.subplots(figsize=(14, 6))
    sns.barplot(data = summary, x = 'kernel', y = 'speedup', hue = 'scaling_type', ax = ax,)
    ax.axhline(1, color = 'red', linestyle = '--',)
    ax.tick_params(axis = 'x', labelrotation = 45,)
    ax.set_title(f'Geometric mean speedup: {other} vs {reference}')
    save_figure(fig,
                folder / (f'geomean_{safe_name(other)}_vs_{safe_name(reference)}.png'),
                show,
            )

def compiler_comparisons(benchmarks: pd.DataFrame, output_dir: Path, show: bool = False, detailed: bool = False) -> None:
    """
    Compare every unordered pair of available compilers.

    Keep gcc as the first reference when present for consistent filenames
    with previous runs, but also compare all remaining compilers to each other.
    """
    available = sorted(benchmarks['compiler'].dropna().unique().tolist())
    if len(available) < 2:
        logger.info("Skipping compiler comparisons: fewer than two compilers available")
        return
    if 'gcc' in available:
        available.remove('gcc')
        available.insert(0, 'gcc')
    folder = output_dir / 'comparisons'
    folder.mkdir(parents=True, exist_ok=True)
    for reference, other in combinations(available, 2):
        paired = pair_compiler_results(benchmarks, reference, other)
        if paired.empty:
            logger.warning("No comparable benchmarks for %s vs %s", reference, other)
            continue
        filename = f'paired_{safe_name(other)}_vs_{safe_name(reference)}.csv'
        paired.to_csv(folder / filename, index=False)
        # Controlled comparisons by scaling type, thread count and chunk size.
        plot_threaded_compiler_comparisons(paired, reference, other, folder, show)
        if detailed:
            plot_compiler_speedup(paired, reference, other, folder, show)
            summary = compute_geometric_speedup(paired)
            plot_geometric_speedup(summary, reference, other, folder, show)
            plot_chunked_compiler_comparisons(paired, reference, other, folder, show)

def plot_chunked_compiler_comparisons(
    paired: pd.DataFrame,
    reference: str,
    other: str,
    folder: Path,
    show: bool = False,
) -> None:
    """
    Create kernel comparisons at fixed scaling type and chunk size.
    All other matched benchmark dimensions remain as observations in the
    distribution; no measurements are discarded or silently averaged.
    """
    if paired.empty or 'chunk_size' not in paired.columns:
        return
    chunk_folder = folder / f'by_chunk_{safe_name(other)}_vs_{safe_name(reference)}'
    chunk_folder.mkdir(parents=True, exist_ok=True)
    group_columns = ['scaling_type', 'chunk_size']
    for (scaling, chunk), subset in paired.dropna(subset=['chunk_size']).groupby(group_columns, sort=True):
        if subset.empty:
            continue
        suffix = f'{safe_name(str(scaling))}_chunk_{safe_name(str(chunk))}'
        fig, ax = plt.subplots(figsize=(14, 6))
        sns.boxplot(data=subset, x='kernel', y='speedup', ax=ax)
        ax.axhline(1, color='red', linestyle='--')
        ax.set_yscale('log')
        ax.tick_params(axis='x', labelrotation=45)
        ax.set_ylabel(f'Speedup {other} / {reference} (log scale)')
        ax.set_title(f'{other} vs {reference} | {scaling} | chunk_size={chunk}')
        fig.tight_layout()
        save_figure(fig, chunk_folder / f'speedup_{suffix}.png', show)

        summary = compute_geometric_speedup(subset)
        fig, ax = plt.subplots(figsize=(14, 6))
        sns.barplot(data=summary, x='kernel', y='speedup', ax=ax, color='steelblue')
        ax.axhline(1, color='red', linestyle='--')
        ax.tick_params(axis='x', labelrotation=45)
        ax.set_ylabel(f'Geometric mean speedup {other} / {reference}')
        ax.set_title(f'Geometric mean | {other} vs {reference} | {scaling} | chunk_size={chunk}')
        fig.tight_layout()
        save_figure(fig, chunk_folder / f'geomean_{suffix}.png', show)
        summary.to_csv(chunk_folder / f'geomean_{suffix}.csv', index=False)

def plot_threaded_compiler_comparisons(
    paired: pd.DataFrame,
    reference: str,
    other: str,
    folder: Path,
    show: bool = False,
) -> None:
    """
    Plot speedup by kernel with a distinct series for each thread count.
    Scaling type and chunk size are fixed within each plot. Points represent
    geometric means of the matched configurations for that kernel/thread count.
    """
    required = ['scaling_type', 'chunk_size', 'kernel', 'num_threads', 'speedup']
    if paired.empty or any(column not in paired.columns for column in required):
        return

    valid = paired.replace([np.inf, -np.inf], np.nan).dropna(subset=required)
    valid = valid[valid['speedup'] > 0]
    if valid.empty:
        return

    thread_folder = folder / f'by_chunk_threads_{safe_name(other)}_vs_{safe_name(reference)}'
    thread_folder.mkdir(parents=True, exist_ok=True)

    for (scaling, chunk), subset in valid.groupby(['scaling_type', 'chunk_size'], sort=True):
        summary = (
            subset.groupby(['kernel', 'num_threads'], as_index=False)
            .agg(
                speedup=('speedup', lambda values: np.exp(np.log(values).mean())),
                samples=('speedup', 'size'),
            )
        )
        if summary.empty:
            continue

        suffix = f'{safe_name(str(scaling))}_chunk_{safe_name(str(chunk))}'
        summary.to_csv(thread_folder / f'geomean_by_threads_{suffix}.csv', index=False)

        fig, ax = plt.subplots(figsize=(15, 7))
        sns.lineplot(
            data=summary,
            x='kernel',
            y='speedup',
            hue='num_threads',
            style='num_threads',
            markers=True,
            dashes=False,
            estimator=None,
            sort=False,
            ax=ax,
        )
        ax.axhline(1, color='red', linestyle='--', label='Parity (1x)')
        ax.set_yscale('log')
        ax.set_xlabel('Kernel')
        ax.set_ylabel(f'Geometric mean speedup {other} / {reference} (log scale)')
        ax.set_title(f'{other} vs {reference} by threads | {scaling} | chunk_size={chunk}')
        ax.tick_params(axis='x', labelrotation=45)
        ax.legend(title='Threads', bbox_to_anchor=(1.02, 1), loc='upper left')
        save_figure(fig, thread_folder / f'geomean_by_threads_{suffix}.png', show)
