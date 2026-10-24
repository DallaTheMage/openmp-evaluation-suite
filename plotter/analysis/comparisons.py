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
    keys = ['scaling_type', 'test_set_id', 'test_case_id', 'kernel',]
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
        paired.groupby(
            ['kernel', 'scaling_type']
        )['speedup']
        .agg(
            lambda values: np.exp(
                np.log(values).mean()
            )
        )
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

def compiler_comparisons(benchmarks: pd.DataFrame, output_dir: Path, show: bool = False,) -> None:
    """
    Compare all available compilers against a reference.
    """
    available = (benchmarks['compiler'].dropna().unique().tolist())
    if len(available) < 2:
        logger.info("Skipping compiler comparisons: fewer than two compilers available")
        return
    reference = ('gcc' if 'gcc' in available else available[0])
    folder = output_dir / 'comparisons'
    folder.mkdir(parents = True, exist_ok = True,)
    for other in available:
        if other == reference:
            continue
        paired = pair_compiler_results(benchmarks, reference, other,)
        if paired.empty:
            logger.warning("No comparable benchmarks for %s vs %s", reference, other,)
            continue
        filename = (f'paired_{safe_name(other)}_vs_{safe_name(reference)}.csv')
        paired.to_csv(folder / filename, index = False,)
        plot_compiler_speedup(paired, reference, other, folder, show,)
        summary = compute_geometric_speedup(paired)
        plot_geometric_speedup(summary, reference, other, folder, show,)