# OpenMP Evaluation Suite — Plotting Utility

Scientific visualizations of benchmark CSVs. Python >= 3.10, with the packages in
`plotter/requirements.txt`.

## Installation

From the repository root:

```bash
python3 -m pip install -r plotter/requirements.txt
cd plotter
python3 main.py -i ../output -o plots
```

## Input organization: independent runs, *not* optimization comparisons

**Optimization folders are entirely optional.** A single experiment directory
is enough:

```
output/
    gcc/static/Full_scale/{aggregated_samples.csv,test_plan.csv,...}
    gcc/static/Proportional/{...}
    clang/dynamic/Full_scale/{...}
    ...
```

Use `python3 main.py -i ../output -o plots` for the layout above.

Alternatively, one input root may contain any number of independent runs:

```
RESULTS/
    output_O2/gcc/static/...
    output_O3/gcc/static/...
    experimental-flags/gcc/static/...
```

Use `python3 main.py -i ../RESULTS -o plots`. The output is separated into
`plots/output_O2/`, `plots/output_O3/`, and `plots/experimental-flags/`.
The run directories **need not represent compiler optimization flags** and may
have arbitrary names. **No comparison, pooling, or assumption about optimization
levels** is performed. A single child directory works exactly the same way.
The tool chooses direct-run mode when the input directly contains compiler
subdirectories; otherwise it discovers immediate child directories with that
structure. Missing compilers, schedules, or scaling modes are skipped.

## Flags

```bash
python3 main.py -i ../output -o plots --formats png pdf svg
python3 main.py -i ../output -o plots --diagnostics
python3 main.py -i ../output -o plots --legacy-lines
python3 main.py -i ../output -o plots --exploratory
python3 main.py -i ../output -o plots --correlations
python3 main.py -i ../output -o plots --verbose
```

`--diagnostics` enables the crowded per-view scaling/relative-overhead plots.
`--legacy-lines` enables the categorical `geomean_by_threads_*.png` plots
already summarized by the compiler heatmaps. `--exploratory` retains legacy
heterogeneous distributions/boxplots and pooled correlations. Without these
flags, those diagnostic and redundant plots are omitted. `--show` displays
figures interactively. Default formats: PNG only.

## Core plots and matching rules

| Output family | Meaning |
|---|---|
| `<compiler>/<schedule>/<mode>/efficiency_by_threads.png` | Kernel × view scaling efficiency at each non-baseline thread count. |
| `<compiler>/<schedule>/<mode>/execution_time_fixed_threads.png` | Absolute wall time at the maximum thread count, provided workloads can be compared. |
| `<compiler>/<schedule>/<mode>/energy_edp_fixed_threads.png` | Absolute measured energy and EDP; excludes invalid measurements. |
| `<schedule>/comparisons/.../heatmap_speedup_*.png` | Geometric mean runtime ratio between compilers, matched by all experiment keys. |
| `<schedule>/comparisons/.../heatmap_efficiency_*.png` | Per-compiler matched scaling efficiency and the paired difference in percentage points. |
| `schedule_comparisons/schedule_speedup_*.png` | Runtime ratios to `static` (when available), per compiler/scaling/chunk, matching the *same kernel, view, workload, thread count, and chunk*. |
| `layout_comparisons/layout_speedup_*.png` | Runtime ratios to 1D for matching compiler, schedule, kernel, workload, thread count and chunk; maximum-thread overview. |
| `energy_tradeoffs/energy_tradeoff_*.png` | Strong scaling runtime, energy, EDP gains relative to the *same workload's valid one-thread baseline*. |
| `figures_manifest.csv` | Figure inventory with relative path, family, file format, and source directory. |

All new cross-dimension comparisons export paired observations and per-group
summaries to CSV beside the figures. Geometric means are applied to **positive
ratios from paired configurations only**; the `matched_configs` column gives
the number of matched rows for each summary cell. These are *descriptive*
summaries, not statistical significance claims or confidence intervals.

A ratio > 1 means the alternative is faster (or uses less energy/EDP for
the energy-normalized metrics). All heatmap color scales are symmetric in log₂
around 1× parity. Missing comparable configurations remain missing, not zero.

### Scaling

Strong scaling (`Full_scale`) keeps workload constant:

- `S(p) = T(p0,W)/T(p,W)`; `E_strong(p) = S(p)/(p/p0)`.
- Ideal time: `T(p0,W) × p0/p`.

Weak scaling (`Proportional`) grows workload with thread count:

- For `W(p)/W(p0) = p/p0`, weak efficiency is
  `E_weak(p) = T(p0,W(p0))/T(p,W(p))`.
- Ideal time: `T(p0,W(p0))`.

The exported `overhead_sec` measures extra time relative to the *ideal model*;
it does **not** isolate OpenMP runtime scheduling overhead. It may be negative.
Scaling columns are read from `scaling.csv` with workload validation, or
reconstructed when possible from aggregated data. Older strong-scaling CSVs
that store the fixed workload as `window_size` are supported.

### Energy

Energy is derived from package + DRAM readings. A package measurement must be
positive; DRAM must be nonnegative. If per-run `raw_samples.csv` validation
flags are available, **every run** contributing to an aggregate must have
valid flags and numeric readings; otherwise a conservative aggregate-minimum
check is used. Missing or invalid readings make energy, EDP, and average power
`NaN`, never zero. Extremely short kernel readings may be excluded when RAPL
reports zero; that is intentional. This is more conservative than analyzing
means that contain invalid raw runs.

The `energy_tradeoffs` overview uses *only* strong scaling with paired valid
one-thread baselines for exactly the same workload. There is intentionally no
energy or time comparison across arbitrary kernel workloads.

### Selection and limitations

- `scaling_overhead_<view>.png` and categorical kernel line plots are optional
  diagnostics, not main-paper figures.
- Runtime and EDP extrema are grouped by workload size and window offset.
- Scheduling may have no effect on kernels that do not use schedule clauses;
  interpret those contrasts carefully.
- Compiler, schedule and layout comparisons cannot establish causality alone.
- If multiple sizes/other matched workloads contribute to one cell, its
  geometric mean and match count are exported, so disaggregation is possible.
- Plotting does not establish statistical significance. Inspect run-level data
  and measurement variability before making fine-grained claims.

### Default scaling figures

`speedup_by_threads_<view>.png` is generated by default for every eligible view,
independently of `--diagnostics`. It plots strong speedup with the ideal
reference line for `Full_scale`, or weak scaling efficiency with the 100%
reference line for `Proportional`. `efficiency_by_threads.png` remains a
default output. `--diagnostics` additionally generates the combined
`scaling_overhead_<view>.png` figures.
