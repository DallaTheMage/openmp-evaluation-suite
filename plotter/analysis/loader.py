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
Utilities for loading benchmark datasets.
"""
import logging
from pathlib import Path
import pandas as pd
from .config import (COMPILERS, SCALING_TYPES, DATASETS, REQUIRED_DATASETS,)
logger = logging.getLogger(__name__)

def load_data(data_dir: Path, schedule_str: str,) -> dict[str, dict]:
    """
    Load benchmark datasets from the input directory.

    Expected structure:
        data_dir/
            compiler/
                static/
                    metadata.csv
                    scaling_type/
                        aggregated_samples.csv
                        test_plan.csv
                        ...
    Missing optional datasets are ignored.
    Returns
    -------
    dict
        Nested dictionary containing the loaded DataFrames.
    """
    datasets = {}
    for compiler in COMPILERS:
        base = data_dir / compiler / schedule_str
        if not base.is_dir():
            logger.warning(
                "Skipping compiler %s: directory not found (%s)",
                compiler,
                base,
            )
            continue
        compiler_data = {}
        metadata_path = base / 'metadata.csv'
        if metadata_path.is_file():
            compiler_data['metadata'] = pd.read_csv(metadata_path)
        for scaling in SCALING_TYPES:
            folder = base / scaling
            if not folder.is_dir():
                logger.warning(
                    "Skipping %s/%s: directory not found",
                    compiler,
                    scaling,
                )
                continue
            missing = [ name for name in REQUIRED_DATASETS
                        if not (folder / f'{name}.csv').is_file()]
            if missing:
                logger.warning(
                    "Skipping %s/%s: missing datasets %s",
                    compiler,
                    scaling,
                    missing,
                )
                continue
            scaling_data = {}
            for name in DATASETS:
                path = folder / f'{name}.csv'
                if not path.is_file():
                    logger.info("Optional dataset not found: %s", path,)
                    continue
                scaling_data[name] = pd.read_csv(path)
            compiler_data[scaling] = scaling_data
            logger.info("Loaded %s/%s", compiler, scaling,)
        if any(scaling in compiler_data for scaling in SCALING_TYPES):
            datasets[compiler] = compiler_data
    if not datasets:
        print(f"[WARNING] No valid benchmark datasets found in {data_dir / compiler / schedule_str}, skipping")
    return datasets