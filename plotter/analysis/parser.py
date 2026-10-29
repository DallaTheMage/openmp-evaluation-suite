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

import argparse
from pathlib import Path

"""
Utility for CLI args parsing.
"""

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