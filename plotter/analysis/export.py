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
Benchmark summary and CSV export utilities.
"""

import logging
from pathlib import Path
import pandas as pd

from analysis.columns import present_columns
from analysis.config import GROUP_COLUMNS, SUMMARY_COLUMNS

logger = logging.getLogger(__name__)


def select_summary_columns(df: pd.DataFrame,) -> pd.DataFrame:
    columns = present_columns(df, SUMMARY_COLUMNS)
    return df[columns]

def get_best_performance(benchmarks: pd.DataFrame,) -> pd.DataFrame:
    """
    Select the fastest configuration for each compiler,
    scaling type and kernel.
    """
    valid = benchmarks.dropna(subset=['wall_time_sec_mean'])
    if valid.empty:
        return select_summary_columns(valid)
    indices = valid.groupby(list(GROUP_COLUMNS))['wall_time_sec_mean'].idxmin()
    return select_summary_columns(valid.loc[indices])

def get_best_edp(benchmarks: pd.DataFrame,) -> pd.DataFrame:
    """
    Select the configuration with minimum EDP.
    """
    valid = benchmarks[(benchmarks['energy_total'] > 0) & (benchmarks['edp'] > 0)]
    if valid.empty:
        return select_summary_columns(valid)
    indices = valid.groupby(list(GROUP_COLUMNS))['edp'].idxmin()
    return select_summary_columns(valid.loc[indices])

def get_unstable_benchmarks(benchmarks: pd.DataFrame, n: int = 20,) -> pd.DataFrame:
    """
    Select benchmarks with the highest execution time CV.
    """
    return benchmarks.nlargest(n, 'wall_time_cv_pct',)

def export_tables(benchmarks: pd.DataFrame, output_dir: Path,) -> None:
    """
    Export enriched benchmarks and summary tables.
    """
    output_dir.mkdir(parents = True, exist_ok = True,)
    benchmarks.to_csv(output_dir / 'benchmarks_enriched.csv', index = False,)
    best_performance = get_best_performance(benchmarks)
    best_edp = get_best_edp(benchmarks)
    unstable = get_unstable_benchmarks(benchmarks)
    best_performance.to_csv(output_dir / 'best_performance.csv', index = False,)
    best_edp.to_csv(output_dir / 'best_edp.csv', index = False,)
    unstable.to_csv(output_dir / 'unstable_benchmarks.csv', index = False,)
    logger.info("Exported benchmark tables to %s", output_dir,)
    if not best_performance.empty:
        logger.info( "Fastest configurations:\n%s", best_performance.to_string(index=False),)