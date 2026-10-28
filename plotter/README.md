# OpenMP Evaluation Suite — Plotting Utility

A Python utility for generating plots and summary statistics from benchmark results produced by the **OpenMP Evaluation Suite**.

The tool processes the suite's CSV output files and automatically generates visualizations for performance analysis, scaling behavior, energy consumption, and compiler comparisons.

## Requirements

- Python 3.10+
- NumPy
- Pandas
- Matplotlib
- Seaborn

Install the dependencies:

```bash
pip install -r requirements.txt
```

## Usage

Run the utility from the project root:

```bash
python main.py
```

By default, the script reads benchmark results from `output/` and saves generated plots and CSV summaries to `plots/`.

### Custom Directories

Specify custom input and output directories using `-i` and `-o`:

```bash
python main.py -i /path/to/benchmark/output -o /path/to/plots
```

### Additional Options

Display plots interactively:

```bash
python main.py --show
```

Enable verbose logging:

```bash
python main.py --verbose
```

Show all available options:

```bash
python main.py --help
```

## Output

The generated files are organized by compiler (`gcc`, `clang`, `icx`) and scaling type (`Full_scale`, `Proportional`).
At the moment the plotter utility generates only static schedule plots, soon will be an update to support a full plotting tool.

The utility produces:

- Performance and execution-time plots.
- Speedup and efficiency plots.
- Energy consumption and EDP plots.
- Cache performance and correlation plots.
- Compiler comparison plots.
- CSV files containing processed results and summary statistics.

Plots are saved in PNG format, while summary data is exported as CSV.

Missing datasets or unavailable metrics are skipped when possible.
