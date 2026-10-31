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

from analysis.columns import has_columns, missing_columns, present_columns
from analysis.config import COMPARISON_COLUMNS
from analysis.plotting import save_figure, safe_name

logger = logging.getLogger(__name__)


def _geometric_mean(values: pd.Series) -> float:
    """Geometric mean of positive runtime ratios (validated by callers)."""
    return float(np.exp(np.log(values).mean()))


def _chunk_suffix(scaling: object, chunk: object) -> str:
    """Consistent filename suffix for a scaling mode and chunk size."""
    return f'{safe_name(str(scaling))}_chunk_{safe_name(str(chunk))}'

def pair_compiler_results(benchmarks: pd.DataFrame, reference: str, other: str,) -> pd.DataFrame:
    """
    Match equivalent benchmark configurations and compute speedup.
    """
    keys = ['scaling_type', 'kernel', *present_columns(benchmarks, COMPARISON_COLUMNS)]
    required = keys + ['compiler', 'wall_time_sec_mean']
    missing = missing_columns(benchmarks, required)
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
        .agg(_geometric_mean)
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

def pair_scaling_efficiency(
    scaling: pd.DataFrame,
    reference: str,
    other: str,
) -> pd.DataFrame:
    """Pair independently validated scaling efficiencies for identical workloads.

    Efficiency is NOT the runtime ratio between compilers: each compiler uses
    its own one-thread (or other explicitly recorded) scaling baseline.
    """
    required = {
        'compiler', 'test_mode', 'kernel', 'view_type', 'scaled_threads',
        'baseline_threads', 'chunk_size', 'efficiency_pct',
    }
    if scaling.empty or not has_columns(scaling, *required):
        return pd.DataFrame()

    subset = scaling[scaling['compiler'].isin([reference, other])].copy()
    if subset['compiler'].nunique() < 2:
        return pd.DataFrame()

    work_keys = ['baseline_work_elements', 'scaled_work_elements']
    if not has_columns(subset, *work_keys):
        logger.warning('Skipping efficiency comparison %s vs %s: workload keys unavailable',
                       other, reference)
        return pd.DataFrame()

    keys = [
        'test_mode', 'kernel', 'view_type', 'scaled_threads',
        'baseline_threads', 'chunk_size',
    ]
    keys.extend(present_columns(subset, (
        'window_offset', 'baseline_work_elements', 'scaled_work_elements',
    )))
    subset['efficiency_pct'] = pd.to_numeric(subset['efficiency_pct'], errors='coerce')
    subset = subset.replace([np.inf, -np.inf], np.nan)
    subset = subset.dropna(subset=keys + ['efficiency_pct'])
    subset = subset[subset['efficiency_pct'] > 0]
    if subset.empty:
        return pd.DataFrame()

    if subset.duplicated(keys + ['compiler']).any():
        logger.warning('Skipping efficiency comparison %s vs %s: repeated scaling configurations',
                       other, reference)
        return pd.DataFrame()

    pivot = subset.pivot(index=keys, columns='compiler', values='efficiency_pct')
    if not {reference, other}.issubset(pivot.columns):
        return pd.DataFrame()
    paired = pivot[[reference, other]].dropna().reset_index()
    if paired.empty:
        return paired
    paired = paired.rename(columns={
        reference: 'reference_efficiency_pct', other: 'other_efficiency_pct',
    })
    paired['delta_efficiency_pp'] = (
        paired['other_efficiency_pct'] - paired['reference_efficiency_pct']
    )
    return paired


def plot_speedup_heatmap(
    summary: pd.DataFrame,
    reference: str,
    other: str,
    path: Path,
    scaling: str,
    chunk: object,
    show: bool = False,
) -> None:
    """Show geometric mean compiler speedup per kernel/thread, with 1x as parity."""
    if summary.empty:
        return
    matrix = summary.pivot(index='kernel', columns='num_threads', values='speedup')
    if matrix.empty:
        return
    log2_speedup = pd.DataFrame(
        np.log2(matrix.to_numpy(dtype=float)),
        index=matrix.index,
        columns=matrix.columns,
    )
    max_abs = float(np.nanmax(np.abs(log2_speedup.to_numpy())))
    max_abs = max(max_abs, 0.1)
    fig, ax = plt.subplots(figsize=(max(9, 1.2 * len(matrix.columns) + 4),
                                    max(5, .55 * len(matrix.index) + 2)))
    sns.heatmap(
        log2_speedup, annot=matrix.to_numpy(), fmt='.2f',
        cmap='RdBu_r', center=0, vmin=-max_abs, vmax=max_abs,
        mask=matrix.isna(), ax=ax,
        cbar_kws={'label': 'log₂(speedup): 0 = parity'},
    )
    ax.set_title(f'Geometric mean runtime speedup: {other} / {reference}\n'
                 f'{scaling} | chunk_size={chunk} | >1: {other} faster')
    ax.set_xlabel('Number of threads')
    ax.set_ylabel('Kernel')
    save_figure(fig, path, show)


def plot_efficiency_heatmap(
    summary: pd.DataFrame,
    reference: str,
    other: str,
    path: Path,
    scaling: str,
    chunk: object,
    show: bool = False,
) -> None:
    """Plot matched scaling efficiency for both compilers and their pp difference."""
    if summary.empty:
        return
    index, columns = 'kernel', 'scaled_threads'
    ref = summary.pivot(index=index, columns=columns, values='reference_efficiency_pct')
    alt = summary.pivot(index=index, columns=columns, values='other_efficiency_pct')
    delta = summary.pivot(index=index, columns=columns, values='delta_efficiency_pp')
    max_eff = max(100.0, float(np.nanmax([ref.max().max(), alt.max().max()])))
    max_delta = max(1., float(np.nanmax(np.abs(delta.to_numpy()))))
    fig, axes = plt.subplots(1, 3, figsize=(max(18, 3.5 * len(ref.columns) + 8),
                                          max(5, .55 * len(ref.index) + 2)))
    for ax, data, title in zip(axes[:2], (ref, alt), (reference, other)):
        sns.heatmap(data, ax=ax, cmap='YlGnBu', vmin=0, vmax=max_eff,
                    annot=True, fmt='.1f', mask=data.isna(),
                    cbar_kws={'label': 'Efficiency (%)'})
        ax.set_title(title)
        ax.set_xlabel('Number of threads')
        ax.set_ylabel('Kernel' if ax is axes[0] else '')
    sns.heatmap(delta, ax=axes[2], cmap='RdBu_r', center=0,
                vmin=-max_delta, vmax=max_delta,
                annot=True, fmt='+.1f', mask=delta.isna(),
                cbar_kws={'label': 'Efficiency difference (pp)'})
    axes[2].set_title(f'{other} − {reference}')
    axes[2].set_xlabel('Number of threads')
    axes[2].set_ylabel('')
    fig.suptitle(f'Scaling efficiency (%) | {scaling} | chunk_size={chunk}\n'
                 'Median over matched views/configurations; difference in percentage points',
                 y=1.02)
    save_figure(fig, path, show)


def plot_compiler_efficiency_comparisons(
    paired: pd.DataFrame,
    reference: str,
    other: str,
    folder: Path,
    show: bool = False,
) -> None:
    """Export matched efficiency and plot per-mode/per-chunk efficiency heatmaps."""
    if paired.empty:
        return
    target = folder / f'by_chunk_threads_{safe_name(other)}_vs_{safe_name(reference)}'
    target.mkdir(parents=True, exist_ok=True)
    for (scaling, chunk), subset in paired.groupby(['test_mode', 'chunk_size'], sort=True):
        summary = (subset.groupby(['kernel', 'scaled_threads'], as_index=False)
                   .agg(reference_efficiency_pct=('reference_efficiency_pct', 'median'),
                        other_efficiency_pct=('other_efficiency_pct', 'median'),
                        delta_efficiency_pp=('delta_efficiency_pp', 'median'),
                        samples=('delta_efficiency_pp', 'size')))
        if summary.empty:
            continue
        suffix = _chunk_suffix(scaling, chunk)
        summary.to_csv(target / f'efficiency_by_threads_{suffix}.csv', index=False)
        plot_efficiency_heatmap(
            summary, reference, other, target / f'heatmap_efficiency_{suffix}.png',
            str(scaling), chunk, show,
        )


def compiler_comparisons(
    benchmarks: pd.DataFrame,
    output_dir: Path,
    show: bool = False,
    detailed: bool = False,
    scaling: pd.DataFrame | None = None,
    legacy_lines: bool = False,
) -> None:
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
        plot_threaded_compiler_comparisons(paired, reference, other, folder, show,
                                           legacy_lines=legacy_lines)
        if scaling is not None and not scaling.empty:
            paired_efficiency = pair_scaling_efficiency(scaling, reference, other)
            plot_compiler_efficiency_comparisons(
                paired_efficiency, reference, other, folder, show,
            )
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
        suffix = _chunk_suffix(scaling, chunk)
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
    legacy_lines: bool = False,
) -> None:
    """
    Plot speedup by kernel with a distinct series for each thread count.
    Scaling type and chunk size are fixed within each plot. Points represent
    geometric means of the matched configurations for that kernel/thread count.
    """
    required = ['scaling_type', 'chunk_size', 'kernel', 'num_threads', 'speedup']
    if paired.empty or not has_columns(paired, *required):
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
                speedup=('speedup', _geometric_mean),
                samples=('speedup', 'size'),
            )
        )
        if summary.empty:
            continue

        suffix = _chunk_suffix(scaling, chunk)
        summary.to_csv(thread_folder / f'geomean_by_threads_{suffix}.csv', index=False)
        plot_speedup_heatmap(
            summary, reference, other,
            thread_folder / f'heatmap_speedup_{suffix}.png', str(scaling), chunk, show,
        )

        if not legacy_lines:
            continue
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
