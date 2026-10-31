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

"""Machine-readable inventory of plot artifacts produced in one invocation."""
from pathlib import Path
import pandas as pd

def export_manifest(output: Path, input_root: Path, modified_since: float) -> None:
    """Collect only graphics written by the current run, excluding stale files."""
    categories = {
        'heatmap_speedup': 'compiler_comparison',
        'heatmap_efficiency': 'compiler_scaling_comparison',
        'schedule_speedup': 'schedule_comparison',
        'layout_speedup': 'layout_comparison',
        'efficiency_by_threads': 'scaling_efficiency',
        'scaling_overhead': 'scaling_diagnostics',
        'execution_time_fixed_threads': 'absolute_runtime',
        'energy_edp_fixed_threads': 'absolute_energy',
        'energy_tradeoff': 'normalized_energy_tradeoff',
        'geomean_by_threads': 'legacy',
    }
    rows = []
    for file in sorted(output.rglob('*')):
        if not file.is_file() or file.suffix not in ('.png', '.pdf', '.svg'):
            continue
        if file.stat().st_mtime < modified_since:
            continue
        family = next((value for prefix, value in categories.items()
                       if file.stem.startswith(prefix)), 'exploratory')
        rows.append({'figure': str(file.relative_to(output)), 'category': family,
                     'format': file.suffix.lstrip('.'),
                     'source_dataset': str(input_root.resolve())})
    output.mkdir(parents=True, exist_ok=True)
    pd.DataFrame(rows, columns=['figure', 'category', 'format', 'source_dataset']).to_csv(
        output / 'figures_manifest.csv', index=False)
