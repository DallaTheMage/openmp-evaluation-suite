/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file ResultsWriter.h
 * @brief CSV serialization of benchmark plans and measurements.
 *
 * ResultsWriter is the persistence boundary of the benchmark suite. It
 * converts the in-memory test plan and measurement structures into stable,
 * machine-readable CSV files without coupling the dispatcher or profiler
 * to filesystem details.
 *
 * A complete benchmark run is written to the currently selected
 * FileManager test directory as:
 *
 *     test_plan.csv
 *     raw_samples.csv
 *     aggregated_samples.csv
 *
 * The test-plan file contains the configuration identifiers required to
 * interpret raw and aggregated samples. Measurement files can therefore
 * remain compact and use stable numeric test-set and test-case identifiers.
 *
 * The writer is intended to be called after benchmark execution has
 * completed. It is not part of the benchmark hot path and is not
 * thread-safe.
 */

#ifndef CORE_RESULTS_WRITER_H
#define CORE_RESULTS_WRITER_H

#include <stddef.h>

#include "core/Analyzer.h"
#include "core/FileManager.h"
#include "core/TestPlan.h"


/**
 * @brief Write the complete benchmark result set to CSV files.
 *
 * The function creates or replaces the three CSV files associated with the
 * current FileManager test directory.
 *
 * @param manager
 *     Initialized FileManager with a current test selected.
 *
 * @param plan
 *     Complete test plan used for the benchmark run.
 *
 * @param samples
 *     Raw benchmark measurements.
 *
 * @param aggregated
 *     Aggregated measurements produced by analyzer_aggregate_samples().
 *
 * @param aggregated_count
 *     Number of elements in @p aggregated.
 *
 * @return
 *     0 on success, non-zero on invalid input or an I/O error.
 *
 * @pre manager != NULL.
 * @pre plan != NULL.
 * @pre samples != NULL.
 * @pre aggregated_count == 0 or aggregated != NULL.
 * @pre set_current_test() has completed successfully for @p manager.
 */
int write_benchmark_results(
    FileManager *manager,
    const TestPlan *plan,
    const RawSampleSet *samples,
    const AggregatedSample *aggregated,
    size_t aggregated_count
);


#endif /* CORE_RESULTS_WRITER_H */