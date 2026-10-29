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
CLI utility for benchmark analysis and visualization.
"""

import argparse
import logging
from pathlib import Path
from analysis.config import SCHEDULES
from analysis.loader import load_data
from analysis.processing import prepare
from analysis.plotting import per_compiler_plots
from analysis.comparisons import compiler_comparisons
from analysis.export import export_tables
from analysis.scaling import prepare_scaling, generate_scaling_plots, plot_fixed_thread_metrics
from analysis.plotting import safe_name

logger = logging.getLogger(__name__)

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=('Generate plots and statistics from benchmark results.')
    )

    parser.add_argument(
        '-i',
        '--input',
        type = Path,
        default = Path('../output'),
        help = ('Directory containing benchmark results (default: output)'),
    )

    parser.add_argument(
        '-o',
        '--output',
        type = Path,
        default = Path('plots'),
        help = ('Directory where plots and tables are saved (default: plots)'),
    )

    parser.add_argument(
        '--show',
        action = 'store_true',
        help = 'Display generated plots interactively',
    )

    parser.add_argument(
        '-v',
        '--verbose',
        action = 'store_true',
        help = 'Enable verbose logging',
    )
    parser.add_argument(
        '--exploratory', action='store_true',
        help='Also generate legacy exploratory boxplots, scatter plots and correlations',
    )
    parser.add_argument(
        '--correlations', action='store_true',
        help='Generate Spearman correlations even without --exploratory',
    )
    return parser.parse_args()

def configure_logging(verbose: bool,) -> None:
    logging.basicConfig(
        level=(logging.DEBUG if verbose else logging.INFO),
        format='[%(levelname)s] %(message)s',
    )

def main() -> None:
    args = parse_args()
    configure_logging(args.verbose)
    input_dir = args.input.resolve()
    output_dir = args.output.resolve()
    logger.info("Input directory: %s", input_dir,)
    logger.info("Output directory: %s", output_dir,)
    for schedule_str in SCHEDULES:
        datasets = load_data(input_dir, schedule_str)
        if datasets:
            benchmarks = prepare(datasets)
            logger.info("Loaded compilers: %s", sorted(benchmarks['compiler'].unique()),)
            logger.info("Total benchmark rows: %d", len(benchmarks),)
            benchmarks['schedule'] = schedule_str
            for compiler, entries in datasets.items():
                for mode in ('Full_scale', 'Proportional'):
                    if mode not in entries:
                        continue
                    subset = benchmarks[(benchmarks.compiler == compiler)
                                        & (benchmarks.scaling_type == mode)]
                    enriched = prepare_scaling(entries[mode].get('scaling'), subset, mode)
                    folder = (output_dir / safe_name(compiler)
                              / safe_name(schedule_str) / safe_name(mode))
                    generate_scaling_plots(enriched, folder, mode, show=args.show)
                    plot_fixed_thread_metrics(subset, folder, mode, show=args.show)
            if args.exploratory or args.correlations:
                per_compiler_plots(benchmarks, output_dir, schedule_str,
                                   show=args.show, exploratory=args.exploratory,
                                   correlations=args.correlations or args.exploratory)
            schedule_dir = output_dir / safe_name(schedule_str)
            compiler_comparisons(benchmarks, schedule_dir, show=args.show,
                                 detailed=args.exploratory)
            export_tables(benchmarks, schedule_dir)
    logger.info("Analysis completed: %s", output_dir,)

if __name__ == '__main__':
    main()