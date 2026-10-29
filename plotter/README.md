# OpenMP Evaluation Suite — Plotting Utility

Generate interpretable visualizations from OpenMP Evaluation Suite benchmark CSVs.

## Installation and usage

Python >=3.10; install dependencies:

```bash
pip install -r requirements.txt
python main.py -i /path/to/output -o /path/to/plots
```

For the provided example archive, use `-i /path/to/extracted/output`.
The default input is `../output`, relative to the current working directory.

Optional:

```bash
python main.py -i ../output -o plots --correlations
python main.py -i ../output -o plots --exploratory
python main.py -i ../output -o plots --verbose
```

`--correlations` enables only Spearman heatmaps; `--exploratory` additionally
produces legacy time/IPC/EDP distributions and energy/cache scatterplots.
These exploratory charts include heterogeneous thread counts and (in weak
scaling) workloads; **do not treat them as controlled benchmark comparisons**.

## Default plots (few, targeted)

For each `<compiler>/<schedule>/<Full_scale|Proportional>`:

| Output | Meaning |
|---|---|
| `efficiency_by_threads.png` | Strong or weak scaling **efficiency (%)** per kernel/view, with a separate panel for each thread count. |
| `scaling_overhead_<view>.png` | Strong **speedup** vs threads or weak **efficiency** vs threads (top); signed extra execution time relative to ideal (%) (bottom). Same kernel colors in both panels. |
| `execution_time_fixed_threads.png` | Absolute execution times at the *highest available thread count* and fixed workload (log color scale); no mixing of thread counts. |
| `energy_edp_fixed_threads.png` | Measured energy and EDP at the same fixed workload and threads; zeros are treated as missing readings. |
| `scaling_enriched.csv` | Tabular baseline, timings, work scale, speedup, efficiency, absolute and relative overhead, ideal time. |

Because a kernel may have very different running times from another, the
relative-overhead plot may use a **symmetric logarithmic axis**. A negative
overhead or efficiency above 100% means that the observed time is better than
the chosen theoretical baseline (or is affected by measurement noise); it
is not an instrumentation result measuring the runtime scheduler alone.

**Strong scaling (Full_scale):** same total workload, increasing `p`.

- `S(p) = T(1, W) / T(p, W)`
- `E_strong(p) = S(p)/p`
- `O_strong(p) = T(p,W) - T(1,W)/p` in seconds

**Weak scaling (Proportional):** same work per thread, increasing total work
proportionally (`W(p)=p*W(1)`).

- `E_weak(p) = T(1, W(1))/T(p, W(p))`
- `O_weak(p) = T(p, W(p)) - T(1, W(1))` in seconds

The normalized overhead shown below a speedup/efficiency curve is
`100 * (T_measured/T_ideal - 1)`: 0% is ideal, positive is extra time.

Scaling metrics are read from each benchmark suite `scaling.csv`, with
workload ratios validated before use. If that optional file is unavailable,
the utility can reconstruct the measurements from `aggregated_samples.csv`
and `test_plan.csv` without averaging unrelated configurations. It omits
ambiguous configurations rather than synthesizing arbitrary curves.

## Cross-compiler comparison and export

Compiler directories are discovered automatically from the input directory;
there is no fixed list of supported compiler names. Compiler comparisons
require at least two compilers with matching benchmark configurations.
Every unordered compiler pair is compared (for example, with `gcc`, `clang`
and `icx`: `gcc` vs `clang`, `gcc` vs `icx`, and `clang` vs `icx`).
With `n` compilers, there are at most `n * (n - 1) / 2` pairwise comparisons;
a pair is skipped when no configurations match. Default comparisons keep
thread counts, scaling modes and chunk sizes separate, and use geometric
mean speedups across matched views. `--exploratory` adds pooled comparison charts.

All per-schedule exports/comparisons are saved to separate locations,
`plots/<schedule>/`, so static, dynamic and guided never overwrite each
other. Per-compiler graphics are in `plots/<compiler>/<schedule>/<mode>/`.

## Limitations

- Overhead is a **model-based difference** from ideal execution time, not a
  measurement exclusively of OpenMP thread scheduling overhead.
- Thread scaling is meaningful only for comparable hardware conditions and
  baseline work mapping. Weak-scaling plots validate `work_scale = p/p0`.
- Energy metrics are only as reliable as the benchmark collection backend;
  zero readings are not interpreted as free execution.
