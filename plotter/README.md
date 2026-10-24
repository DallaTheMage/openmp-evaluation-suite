# Benchmark Analysis Utility

A Python utility for analyzing and visualizing benchmark results.

The tool automatically generates performance plots, compares results across different compilers, and identifies the best benchmark configurations in terms of execution time and energy efficiency.

It is designed to work with previously collected benchmark data stored in CSV files, providing a convenient way to explore experimental results without manually processing each dataset.

## Features

The utility provides the following analyses:

- **Execution time:** Distribution of execution times across different computational kernels.
- **Performance variability:** Analysis of execution time variability using the coefficient of variation (CV).
- **Instructions per cycle (IPC):** Comparison of IPC values across kernels.
- **Energy Delay Product (EDP):** Evaluation of the trade-off between execution time and energy consumption.
- **Energy consumption:** Relationship between execution time and total energy consumption.
- **Cache performance:** Relationship between L1 data cache miss ratios and execution time.
- **Chunk size analysis:** Impact of chunk size on execution time for configurations using eight threads.
- **Scaling analysis:** Speedup and efficiency as the number of threads increases.
- **Correlation analysis:** Spearman correlation matrices for performance and energy metrics.
- **Compiler comparisons:** Performance comparisons between GCC, Clang, and Intel ICX.

The utility also exports CSV files containing enriched benchmark data and summary statistics.

## Requirements

- Python 3.10 or later
- NumPy
- Pandas
- Matplotlib
- Seaborn

Install the required dependencies using:

```bash
pip install -r requirements.txt
```

## Project Structure

The project is organized into separate modules, each responsible for a specific part of the analysis pipeline.

```text
benchmark-analysis/
│
├── plot_benchmarks.py
│
├── benchmark_analysis/
│   ├── __init__.py
│   ├── config.py
│   ├── loader.py
│   ├── processing.py
│   ├── plotting.py
│   ├── comparisons.py
│   └── export.py
│
├── requirements.txt
└── README.md
```

### Module Overview

| Module | Description |
|---|---|
| `plot_benchmarks.py` | Command-line entry point that coordinates the analysis pipeline. |
| `config.py` | Shared constants and benchmark configuration definitions. |
| `loader.py` | Loads benchmark datasets from CSV files. |
| `processing.py` | Merges datasets, normalizes numeric values, and computes derived metrics. |
| `plotting.py` | Generates performance, energy, scaling, and correlation plots. |
| `comparisons.py` | Matches equivalent benchmark configurations and compares compiler performance. |
| `export.py` | Exports enriched datasets and summary tables to CSV files. |

## Input Data Structure

The utility expects benchmark results to be organized by compiler and scaling type.

The default input directory is `output/`.

```text
output/
├── gcc/
│   └── static/
│       ├── metadata.csv
│       ├── Full_scale/
│       │   ├── aggregated_samples.csv
│       │   ├── raw_samples.csv
│       │   ├── scaling.csv
│       │   └── test_plan.csv
│       └── Proportional/
│           └── ...
│
├── clang/
│   └── static/
│       ├── metadata.csv
│       ├── Full_scale/
│       │   └── ...
│       └── Proportional/
│           └── ...
│
└── icx/
    └── static/
        ├── metadata.csv
        ├── Full_scale/
        │   └── ...
        └── Proportional/
            └── ...
```

For each compiler and scaling type, the following files are required:

- `aggregated_samples.csv`
- `test_plan.csv`

The following files are optional:

- `raw_samples.csv`
- `scaling.csv`

The `metadata.csv` file is also optional.

Missing optional files are ignored. If a required file or directory is missing, the corresponding compiler/scaling configuration is skipped.

### Dataset Matching

Aggregated benchmark results are merged with their corresponding test plans using the following identifiers:

- `test_set_id`
- `test_case_id`
- `kernel`

These identifiers must uniquely identify the records being merged.

## Usage

Run the utility from the project root directory.

### Basic Usage

```bash
python plot_benchmarks.py
```

By default, the program:

- Reads benchmark data from `output/`.
- Saves plots and summary tables to `plots/`.
- Generates plots without opening interactive windows.

### Custom Input Directory

Use `-i` or `--input` to specify the directory containing the benchmark results:

```bash
python plot_benchmarks.py --input results/experiment_01
```

### Custom Output Directory

Use `-o` or `--output` to specify where the generated files should be saved:

```bash
python plot_benchmarks.py --output plots/experiment_01
```

### Custom Input and Output Directories

Both options can be combined:

```bash
python plot_benchmarks.py \
    --input results/experiment_01 \
    --output plots/experiment_01
```

### Interactive Plot Display

By default, plots are saved without being displayed.

To display each generated plot interactively, use:

```bash
python plot_benchmarks.py --show
```

### Verbose Logging

Enable detailed logging with:

```bash
python plot_benchmarks.py --verbose
```

### Command-Line Options

| Option | Description | Default |
|---|---|---|
| `-i`, `--input` | Directory containing benchmark results. | `output/` |
| `-o`, `--output` | Directory where plots and tables are saved. | `plots/` |
| `--show` | Display plots interactively. | Disabled |
| `-v`, `--verbose` | Enable verbose logging. | Disabled |
| `-h`, `--help` | Display the help message. | — |

To display all available options:

```bash
python plot_benchmarks.py --help
```

## Generated Output

The utility creates an output directory containing plots for each available compiler/scaling combination, compiler comparisons, and summary CSV files.

The default output directory is `plots/`.

```text
plots/
├── gcc/
│   ├── Full_scale/
│   │   ├── execution_time.png
│   │   ├── variability.png
│   │   ├── ipc.png
│   │   ├── edp.png
│   │   ├── kernel_view_type.png
│   │   ├── energy_vs_time.png
│   │   ├── cache_vs_time.png
│   │   ├── chunk_size.png
│   │   ├── speedup_<kernel>.png
│   │   ├── efficiency_<kernel>.png
│   │   └── correlation.png
│   │
│   └── Proportional/
│       └── ...
│
├── clang/
│   └── ...
│
├── icx/
│   └── ...
│
├── comparisons/
│   ├── speedup_<compiler>_vs_<reference>.png
│   ├── geomean_<compiler>_vs_<reference>.png
│   └── paired_<compiler>_vs_<reference>.csv
│
├── benchmarks_enriched.csv
├── best_performance.csv
├── best_edp.csv
└── unstable_benchmarks.csv
```

**Note:** Not every plot is necessarily generated. Plots requiring unavailable metrics or insufficient data are skipped.

All plots are saved as PNG images at 200 DPI.

### Summary CSV Files

| File | Description |
|---|---|
| `benchmarks_enriched.csv` | Complete merged dataset, including derived performance and energy metrics. |
| `best_performance.csv` | Fastest configuration for each compiler, scaling type, and kernel. |
| `best_edp.csv` | Configuration with the lowest Energy Delay Product for each compiler, scaling type, and kernel. |
| `unstable_benchmarks.csv` | Up to 20 benchmark configurations with the highest execution time coefficient of variation. |

### Compiler Comparison Files

When at least two compilers are available, the utility compares equivalent benchmark configurations.

For each comparison, it generates:

- A boxplot showing the distribution of relative speedups.
- A bar chart showing the geometric mean speedup for each kernel and scaling type.
- A CSV file containing the matched benchmark configurations and their calculated speedups.

GCC is used as the reference compiler when available. Otherwise, the first available compiler is selected.

## Computed Metrics

During data preparation, the utility computes additional metrics from the aggregated benchmark results.

### Execution Time Standard Deviation

The standard deviation is computed from the execution time variance:

\[
\sigma_T = \sqrt{\operatorname{Var}(T)}
\]

### Coefficient of Variation

The coefficient of variation measures relative execution time variability:

\[
CV = \frac{\sigma_T}{\overline{T}} \times 100
\]

where:

- \(\sigma_T\) is the execution time standard deviation.
- \(\overline{T}\) is the mean execution time.

### Execution Time Range

\[
T_{\text{range}} = T_{\max} - T_{\min}
\]

### Total Energy Consumption

Total energy consumption is computed as:

\[
E_{\text{total}} = E_{\text{package}} + E_{\text{DRAM}}
\]

This metric is available only when both energy measurements are present.

### Energy Delay Product

The Energy Delay Product combines energy consumption and execution time:

\[
EDP = E_{\text{total}} \times \overline{T}
\]

Lower EDP values indicate a better combined result in terms of energy consumption and execution time.

### Average Power

Average power is estimated as:

\[
P_{\text{avg}} = \frac{E_{\text{total}}}{\overline{T}}
\]

### Speedup

Speedup is calculated relative to the execution time measured with one thread:

\[
S(p) = \frac{T(1)}{T(p)}
\]

where \(p\) is the number of threads.

### Parallel Efficiency

Parallel efficiency is calculated as:

\[
E(p) = \frac{S(p)}{p}
\]

The interpretation of these metrics depends on the scaling methodology used during the experiments.

## Scaling Modes

The utility supports two benchmark scaling modes:

### Full_scale

This mode can be used to study strong scaling when the total workload remains constant as the number of threads increases.

The ideal strong-scaling speedup is:

\[
S(p) = p
\]

### Proportional

This mode can be used to study weak scaling when the workload increases proportionally to the number of threads.

In weak-scaling experiments, execution time stability is generally more meaningful than conventional strong-scaling speedup.

**Important:** The current implementation computes speedup and efficiency using the same formulas for both scaling modes. Results from proportional scaling experiments should therefore be interpreted with care.

## Compiler Comparisons

Compiler performance is compared using matched benchmark configurations.

For a reference compiler \(A\) and another compiler \(B\), relative speedup is defined as:

\[
S_{B/A} = \frac{T_A}{T_B}
\]

Therefore:

- \(S_{B/A} > 1\): Compiler B is faster.
- \(S_{B/A} = 1\): Both compilers have equivalent execution times.
- \(S_{B/A} < 1\): Compiler A is faster.

To summarize multiple speedup measurements, the utility computes their geometric mean:

\[
GM = \left(\prod_{i=1}^{n} S_i\right)^{1/n}
\]

The geometric mean is appropriate for aggregating positive performance ratios.

## Notes and Limitations

- The utility analyzes previously collected benchmark results; it does not execute benchmarks.
- Benchmark datasets must follow the expected directory structure and CSV schema.
- Missing compiler directories or scaling configurations are skipped.
- Some plots require specific performance counters and are skipped when the necessary columns are unavailable.
- Energy-related metrics require package and DRAM energy measurements.
- Compiler comparisons require equivalent benchmark configurations with matching identifiers.
- Speedup and efficiency calculations use single-thread execution time as their baseline.
- The generated results should be interpreted according to the experimental setup and scaling methodology.

## License

The project includes a `LICENSE` file describing its **GNU General Public License** terms. Refer to that file for the complete conditions and redistribution requirements.
