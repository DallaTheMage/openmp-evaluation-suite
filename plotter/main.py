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

from analysis.loader import load_data
from analysis.processing import prepare
from analysis.plotting import per_compiler_plots
from analysis.comparisons import compiler_comparisons
from analysis.export import export_tables

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
    # Load benchmark datasets
    datasets = load_data(input_dir)
    # Prepare and enrich data
    benchmarks = prepare(datasets)
    logger.info("Loaded compilers: %s", sorted(benchmarks['compiler'].unique()),)
    logger.info("Total benchmark rows: %d", len(benchmarks),)
    # Generate compiler-specific plots
    per_compiler_plots(benchmarks, output_dir, show = args.show,)
    # Compare compiler performance
    compiler_comparisons(benchmarks, output_dir, show = args.show,)
    # Export summary tables
    export_tables(benchmarks, output_dir,)
    logger.info("Analysis completed: %s", output_dir,)

if __name__ == '__main__':
    main()