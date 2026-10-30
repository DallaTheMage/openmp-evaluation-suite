# OpenMP Evaluation Suite

A compiler-agnostic C benchmark suite for evaluating OpenMP implementations on shared-memory CPUs. It supports fixed-size and proportional-workload scaling experiments, OpenMP constructs, algorithmic kernels, statistical measurements, and optional hardware and energy diagnostics.

## Contents

1. [Requirements](#requirements)
2. [Quick start](#quick-start)
3. [Building the suite](#building-the-suite)
4. [Automatic configuration](#automatic-configuration)
5. [Configuration reference](#configuration-reference)
6. [Running benchmarks and collecting results](#running-benchmarks-and-collecting-results)
7. [Automated test batteries](#automated-test-batteries)
8. [Plotting and analysis](#plotting-and-analysis)
9. [Experimental methodology and reproducibility](#experimental-methodology-and-reproducibility)
10. [Project structure](#project-structure)
11. [Troubleshooting](#troubleshooting)
12. [License](#license)

## Requirements

- Linux, Bash, GNU Make, and standard command-line utilities.
- A C compiler supporting C99 and OpenMP (for example GCC, Clang, or Intel `icx`).
- A compatible OpenMP runtime library.
- Sufficient RAM for the selected dataset and kernels.
- Optional: Linux `perf_event` interfaces and RAPL support for hardware/energy measurements, subject to CPU support and permissions.

Check your environment:

```bash
bash --version
make --version
gcc --version
```

Some Clang installations require a separately installed OpenMP runtime (`libomp`). Availability of energy readings and performance counters depends on the host platform and permissions.

## Quick start

Run commands **from the repository root** so relative output paths resolve as intended.

```bash
# Compile using default.conf
./build.sh

# Run the executable reported by the build script
./build/cc/main
```

The binary location depends on the selected compiler: `build/<compiler-name>/main`. For example, if `COMPILERNAME=gcc`, run `./build/gcc/main`. Always use the exact executable path printed by `build.sh`.

To generate a hardware-aware configuration:

```bash
./autotune.sh
./build.sh autotune.conf
./build/cc/main   # replace cc with the compiler name shown by build.sh
```

The autotuner **does not run the benchmarks** or search for the fastest settings. It detects system resources and generates conservative workload parameters.

If needed, enable script execution:

```bash
chmod +x build.sh autotune.sh
```

## Building the suite

The supported entry point is `build.sh`:

```bash
./build.sh                                   # use default.conf
./build.sh autotune.conf                     # use a specific configuration
./build.sh --config autotune.conf            # equivalent explicit syntax
./build.sh WORK_REPS=10U                     # override one parameter
./build.sh autotune.conf THREAD_LIST=1,2,4 WORK_REPS=10U
VERBOSE=1 ./build.sh                         # show complete build commands
./build.sh --help
```

**Precedence:** command-line `KEY=VALUE` overrides the configuration file; the `CC` environment variable overrides `COMPILERNAME`.

Choose a compiler explicitly:

```bash
CC=gcc ./build.sh
CC=clang ./build.sh
CC=icx ./build.sh
```

Customize compiler settings:

```bash
OPT_LEVEL=-O3 ./build.sh
VERBOSE=1 OPT_LEVEL=-O0 ./build.sh
OPENMP_CFLAGS="-fopenmp" OPENMP_LDFLAGS="-fopenmp" ./build.sh
```

`OPENMP_LIBS` can supply additional OpenMP-related libraries. The build script configures the OpenMP toolchain based on the selected compiler. Supported `OPT_LEVEL` values are `-O0`, `-O1`, `-O2`, `-O3`, `-Os`, `-Og`, and `-Ofast`.

The build script cleans the previous build before recompiling. You can invoke GNU Make directly, but doing so **does not automatically load** the `.conf` file and pass its values as compiler macros:

```bash
make
make clean-build       # remove build artifacts
make clean-output      # remove benchmark output
make clean-plots       # remove plotter/plots
make clean-battery-results # remove results/
make clean-all        # remove build/, output/, plotter/plots/, and results/
```

**Warning:** `make clean-output`, `make clean-battery-results`, and `make clean-all` remove results permanently. Back up experiments before running them. The Makefile does not define a `make erase` recipe.

## Automatic configuration

Run `autotune.sh` with an optional output filename:

```bash
./autotune.sh                       # writes autotune.conf
./autotune.sh experiment.conf       # writes a custom configuration
./build.sh experiment.conf
```

The autotuner detects the number of logical CPUs, available memory, a default compiler, and an estimated last-level cache (LLC) size. It selects:

- `THREAD_LIST`: thread counts based on the available logical CPUs.
- `PROBLEM_LOG2_SIZE`: the main dataset size exponent.
- `SIZE_PER_THREAD_LOG2`: the per-thread workload exponent for proportional scaling.
- `WINDOWS_LOG2_LIST`: admissible working-set window sizes.
- Cache eviction, scheduling, and repetition settings.

Its conservative memory budget is approximately **25% of detected available RAM**, using an estimate of **64 bytes per logical element**. This is a heuristic, not a guarantee that every kernel will fit comfortably in memory. Inspect the generated configuration before running large experiments.

**Important:** autotuning overwrites the chosen output file. Use separate filenames for experiments you want to preserve.

## Configuration reference

`default.conf` provides a conservative starting point. `autotune.conf` is generated for the current host. The format is one `KEY=VALUE` pair per line, with `#` comments, comma-separated lists, and optional C integer suffixes such as `U` and `ULL`. Do **not** use shell-style `export KEY=VALUE` statements.

| Parameter | Purpose |
| --- | --- |
| `COMPILERNAME` | C compiler executable |
| `THREAD_LIST` | Comma-separated thread counts |
| `CHOSEN_SCHEDULE_ID` | OpenMP schedule: 1=static, 2=dynamic, 3=guided |
| `CHUNK_SIZE_LIST` | OpenMP chunk sizes to test |
| `PROBLEM_LOG2_SIZE` | Base-2 exponent of the main dataset size |
| `SIZE_PER_THREAD_LOG2` | Base-2 exponent of the per-thread workload |
| `WINDOWS_LOG2_LIST` | Base-2 working-set window sizes |
| `MEMORY_ALIGNMENT` | Allocation alignment in bytes |
| `RUN_FULL_SCALE` | Enable fixed-size scaling tests (1/0) |
| `RUN_PROPORTIONAL` | Enable proportional-workload tests (1/0) |
| `WARMUP_REPS` | Warm-up iterations excluded from measurements |
| `WORK_REPS` | Measured iterations per configuration |
| `GENERATION_SEED` | Seed for reproducible input generation |
| `DETECTED_LLC_SIZE_BYTES` | Detected LLC size in bytes; 0 if unknown |
| `CACHE_EVICTION_ENABLED` | Enable cache eviction between runs (1/0) |
| `CACHE_EVICTION_MULTIPLIER` | Eviction buffer size relative to LLC |
| `CACHE_EVICTION_STRIDE_BYTES` | Access stride within the eviction buffer |
| `ENABLE_HW_COUNTERS` | Enable experimental `perf_event` counters (1/0) |
| `SLOWDOWN_FACTOR` | Additional computational iterations within kernels; `0` disables the artificial slowdown |

A small example configuration for smoke testing:

```ini
COMPILERNAME=gcc
THREAD_LIST=1,2
CHOSEN_SCHEDULE_ID=1U
CHUNK_SIZE_LIST=64U
PROBLEM_LOG2_SIZE=20U
SIZE_PER_THREAD_LOG2=18U
WINDOWS_LOG2_LIST=12U,14U,16U,18U
MEMORY_ALIGNMENT=64U
RUN_FULL_SCALE=1
RUN_PROPORTIONAL=1
WARMUP_REPS=2U
WORK_REPS=3U
GENERATION_SEED=12345ULL
CACHE_EVICTION_ENABLED=0U
ENABLE_HW_COUNTERS=0
```

Other parameters can use the defaults supplied by the project; for reproducible research, save the complete effective configuration alongside your results.

**Configuration values are passed as C preprocessor macros at build time.** Changing a `.conf` file does **not** change an already compiled executable. Rebuild after every configuration change.

## Running benchmarks and collecting results

After a successful build, run the executable path printed by `build.sh`, from the repository root:

```bash
./build/gcc/main
```

The suite constructs test plans for the enabled modes:

- **Full-scale / strong scaling:** keep the dataset size fixed while varying the thread count.
- **Proportional / weak-scaling-like:** increase the number of elements in proportion to the number of threads.

The proportional mode aims to keep the amount of work per thread approximately constant; the computational cost per element may still differ between kernels.

Results are written under the `output/` directory, with subdirectories determined by the benchmark/reporting modules. The program prints the actual output location during execution. Inspect the generated CSV files for the specific columns, units, and test identifiers rather than assuming a universal output filename.

To archive a completed experiment:

```bash
mkdir -p experiments/run-01
cp autotune.conf experiments/run-01/config.conf
cp -a output experiments/run-01/output
```

Copy the results **after** the run finishes and before executing `make erase`.

## Automated test batteries

`run_battery.sh` runs a matrix of compiler, OpenMP schedule, and optimization settings. Its configuration is separate from the benchmark's `.conf` file:

```bash
# Review and adjust default.battery.conf first
./run_battery.sh default.battery.conf
# Or use a custom battery configuration
./run_battery.sh my-experiments.battery.conf
```

The battery file is **sourced as Bash** and must define `CONFIGURATION_FILE` (benchmark `.conf`), `COMPILERS` (`gcc`, `clang`, `icx`), `SCHEDULING_POLICIES` (`1U`, `2U`, `3U`), and `OPTIMIZATION_LEVELS` (`O0`–`O3`), with comma-separated values for the three lists. Run the script from the repository root. For example:

```bash
CONFIGURATION_FILE=default.conf
COMPILERS=gcc,clang
SCHEDULING_POLICIES=1U,2U
OPTIMIZATION_LEVELS=O2,O3
```

Each battery creates `results/<timestamp>_<pid>/`, saves its two configuration files and compiler versions, and stores benchmark output separately under `output_O2/`, `output_O3/`, etc. Output directories for different schedules and compilers are accumulated within each optimization directory.

**Current script constraints:** `run_battery.sh` sources `/opt/intel/oneapi/setvars.sh` only when `icx` is listed in `COMPILERS`; in that case, the Intel oneAPI installation must be available at that path. It also uses `sudo` to clear `output/` and run benchmarks, so review permission/security implications and back up existing `output/` **before starting**. Unlike single builds, the battery's supported compiler and optimization lists are deliberately limited to the values above.

## Plotting and analysis

The Python plotting utility is documented in [plotter/README.md](plotter/README.md). From the repository root:

```bash
python3 -m pip install -r plotter/requirements.txt
cd plotter
python3 main.py -i ../output -o plots
# For battery results, point -i at a specific output_<optimization> directory:
python3 main.py -i ../results/<run-id>/output_O2 -o plots_O2
```

Use the output root containing compiler subdirectories (`<compiler>/<schedule>/...`); do not pass the top-level `results/<run-id>` directory directly.

## Experimental methodology and reproducibility

For meaningful compiler and runtime comparisons:

1. Record the CPU, operating system, compiler and OpenMP runtime versions, compiler flags, and full `.conf` file.
2. Include `1` in `THREAD_LIST` to establish a scaling baseline.
3. Keep datasets, random seeds, and workload parameters consistent across comparisons.
4. Increase `WARMUP_REPS` and `WORK_REPS` beyond smoke-test values when collecting thesis-quality measurements.
5. Document thread affinity (`OMP_PROC_BIND`, `OMP_PLACES`), background system load, and thermal conditions.
6. Clearly report any differences in compiler optimization levels.
7. Check support and permissions before interpreting energy readings or hardware counter results.

Example with explicit affinity:

```bash
OMP_PROC_BIND=close OMP_PLACES=cores ./build/gcc/main
```

OpenMP affinity settings are interpreted by the runtime and may behave differently depending on the machine topology.

## Project structure

```text
.
├── autotune.sh             # hardware detection and .conf generation
├── build.sh                # configuration-aware compilation
├── default.conf            # conservative default configuration
├── default.battery.conf    # test-battery matrix configuration
├── run_battery.sh          # batch execution across compilers/schedules/flags
├── autotune.conf           # generated configuration (when present)
├── Makefile                # compilation and cleanup
├── lib/
│   ├── common.sh           # shared shell utilities
│   ├── hardware.sh         # hardware resource detection
│   ├── workload.sh         # workload selection
│   ├── build_config.sh     # configuration parsing and validation
│   └── toolchain.sh        # compiler and OpenMP setup
├── include/                # C headers, macros, interfaces
├── src/
│   ├── core/               # planning, dispatch, analysis, reporting
│   ├── data/               # buffers, memory, input generation
│   ├── kernels/            # OpenMP constructs and algorithms
│   ├── profiling/          # timing, energy, hardware counters
│   └── main.c              # benchmark orchestration
├── plotter/                # Python visualization and CSV analysis
├── build/                  # generated binaries and object files
├── output/                 # single-run benchmark results
└── results/                # archived battery runs (generated)
```

## Troubleshooting

**`Permission denied` when running a script:** run `chmod +x build.sh autotune.sh`, or invoke it with `bash ./build.sh`.

**Compiler not found / OpenMP unavailable:** verify `CC`, `COMPILERNAME`, the OpenMP runtime installation, and the relevant flags. Run `VERBOSE=1 ./build.sh` to inspect the build commands.
If you are using intel oneAPI compiler (icc or icx) you need to execute `source /opt/intel/oneapi/setvars.sh` before compilation or insert that line inside your shell startup configuration file.

**Error indicating `SIZE_PER_THREAD` is derived:** replace the old `SIZE_PER_THREAD=...` configuration entry with `SIZE_PER_THREAD_LOG2=...`.

**Proportional workload exceeds the dataset:** decrease `SIZE_PER_THREAD_LOG2`, reduce the maximum thread count, or increase `PROBLEM_LOG2_SIZE` if sufficient RAM is available.

**Configuration edits do not affect execution:** rebuild with `./build.sh file.conf`; the settings are compile-time macros.

**Missing energy readings or hardware counters:** confirm that the CPU, kernel interfaces, and permissions support the requested measurements. `ENABLE_HW_COUNTERS=0` disables the experimental hardware counters, not wall-clock timing.

**Noisy benchmark timings:** increase the repetition count, reduce background activity, pin threads consistently, and repeat measurements under controlled conditions.

## License

The project includes a `LICENSE` file containing the **GNU General Public License version 3** (project source headers specify `GPL-3.0-or-later`). Refer to that file for the complete conditions and redistribution requirements.

---
