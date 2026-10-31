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

"""CLI utility for benchmark analysis and visualization."""
import logging
import time
from pathlib import Path
import pandas as pd
from analysis.config import SCHEDULES
from analysis.loader import load_data
from analysis.processing import prepare
from analysis.plotting import per_compiler_plots, safe_name, set_figure_formats
from analysis.comparisons import compiler_comparisons
from analysis.cross_dimensions import schedule_comparisons, layout_comparisons, energy_tradeoffs
from analysis.manifest import export_manifest
from analysis.export import export_tables
from analysis.scaling import prepare_scaling, generate_scaling_plots, plot_fixed_thread_metrics
from analysis.parser import parse_args

logger = logging.getLogger(__name__)


def configure_logging(verbose: bool) -> None:
    logging.basicConfig(level=logging.DEBUG if verbose else logging.INFO,
                        format='[%(levelname)s] %(message)s')


def _has_compiler_results(directory: Path) -> bool:
    """Recognize a run by its actual compiler/schedule structure, not its label."""
    return directory.is_dir() and any(
        compiler.is_dir() and any((compiler / s).is_dir() for s in SCHEDULES)
        for compiler in directory.iterdir()
    )


def discover_runs(directory: Path) -> list[tuple[str | None, Path]]:
    """Accept one run, or direct child runs of any name; never compare runs."""
    if _has_compiler_results(directory):
        return [(None, directory)]
    if not directory.is_dir():
        return []
    return [(child.name, child) for child in sorted(directory.iterdir())
            if _has_compiler_results(child)]


def run_one(input_dir: Path, output_dir: Path, args) -> None:
    all_schedules = []
    for schedule_str in SCHEDULES:
        datasets = load_data(input_dir, schedule_str)
        if not datasets:
            continue
        benchmarks = prepare(datasets)
        benchmarks['schedule'] = schedule_str
        all_schedules.append(benchmarks)
        logger.info('%s/%s: %d benchmark configurations, compilers %s',
                    input_dir.name, schedule_str, len(benchmarks),
                    sorted(benchmarks.compiler.unique()))
        scaling_results = []
        for compiler, entries in datasets.items():
            for mode in ('Full_scale', 'Proportional'):
                if mode not in entries:
                    continue
                subset = benchmarks[(benchmarks.compiler == compiler)
                                    & (benchmarks.scaling_type == mode)]
                enriched = prepare_scaling(entries[mode].get('scaling'), subset, mode)
                if not enriched.empty:
                    valid = enriched.copy()
                    valid['compiler'] = compiler
                    scaling_results.append(valid)
                folder = (output_dir / safe_name(compiler)
                          / safe_name(schedule_str) / safe_name(mode))
                generate_scaling_plots(enriched, folder, mode, show=args.show,
                                       diagnostics=args.diagnostics)
                plot_fixed_thread_metrics(subset, folder, mode, show=args.show)
        if args.exploratory or args.correlations:
            per_compiler_plots(benchmarks, output_dir, schedule_str,
                               show=args.show, exploratory=args.exploratory,
                               correlations=args.correlations or args.exploratory)
        schedule_dir = output_dir / safe_name(schedule_str)
        scaling_df = (pd.concat(scaling_results, ignore_index=True)
                      if scaling_results else pd.DataFrame())
        compiler_comparisons(benchmarks, schedule_dir, show=args.show,
                             detailed=args.exploratory, scaling=scaling_df,
                             legacy_lines=args.legacy_lines)
        export_tables(benchmarks, schedule_dir)
    if all_schedules:
        combined = pd.concat(all_schedules, ignore_index=True)
        schedule_comparisons(combined, output_dir, show=args.show)
        layout_comparisons(combined, output_dir, show=args.show)
        energy_tradeoffs(combined, output_dir, show=args.show)
    else:
        logger.warning('No valid benchmark datasets in %s', input_dir)

def main() -> None:
    args = parse_args()
    configure_logging(args.verbose)
    set_figure_formats(args.formats)
    input_dir, output_dir = args.input.resolve(), args.output.resolve()
    runs = discover_runs(input_dir)
    if not runs:
        raise SystemExit(f'No benchmark runs found in {input_dir}; expected '
                         '<compiler>/<schedule>/... or child run directories')
    logger.info('Found %d independent run(s); run labels are NOT optimization comparisons',
                len(runs))
    for label, directory in runs:
        destination = output_dir / safe_name(label) if label is not None else output_dir
        started = time.time()
        logger.info('Processing independent run %s -> %s', directory, destination)
        run_one(directory, destination, args)
        export_manifest(destination, directory, started)
    logger.info('Analysis completed: %s', output_dir)

if __name__ == '__main__':
    main()
