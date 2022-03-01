# DMC (diagram model counter)
Given a project-join tree for an XOR-CNF formula, DMC solves:
- Boolean MPE
- weighted projected model counting (WPMC); tree must be graded

<!-- ####################################################################### -->

## Installation (Linux)

### Prerequisites
- [external libraries](./Singularity):
  - automake 1.16
  - cmake 3.16
  - g++ 10.3
  - gmp 6.2
  - m4ri 20200125
  - sqlite3 3.31
  - make 4.2
  - python3 3.8
- [included libraries](../addmc/libraries/):
  - [cryptominisat 5.8](https://github.com/msoos/cryptominisat)
  - [cudd 3.0](https://github.com/ivmai/cudd)
  - [cxxopts 2.2](https://github.com/jarro2783/cxxopts)
  - [sylvan 1.5](https://github.com/trolando/sylvan)

### Command
#### With Singularity 3.5 (slow)
```bash
sudo make dmc.sif
```
#### Without Singularity (quick)
```bash
make dmc
```

<!-- ####################################################################### -->

## Examples
If you use Singularity, then replace `./dmc --cf=$cnfFile` with `singularity run --bind="/:/host" ./dmc.sif --cf=/host$(realpath $cnfFile)` in the following commands.

### Showing command-line options
#### Command
```bash
./dmc
```
#### Output
```
Diagram Model Counter (reads join tree from stdin)
Usage:
  dmc [OPTION...]

      --cf arg  CNF file path; string (REQUIRED)
      --wc arg  weighted counting: 0, 1; int (default: 0)
      --pc arg  projected counting: 0, 1; int (default: 0)
      --er arg  existential-randomized stochastic satisfiability: 0, 1; int (default: 0)
      --dp arg  diagram package: c/CUDD, s/SYLVAN; string (default: c)
      --lc arg  logarithmic counting [with dp_arg = c]: 0, 1; int (default: 0)
      --lb arg  log10 of bound for existential pruning [with er_arg = 1]; float (default: -inf)
      --tm arg  threshold model for existential pruning [with er_arg = 1]; string (default: "")
      --ep arg  existential pruning using CryptoMiniSat [with er_arg = 1]: 0, 1; int (default: 0)
      --mf arg  maximizer format [with er_arg = 1]: 0/NONE, 1/SHORT, 2/LONG, 3/ALL; int (default: 0)
      --mv arg  maximizer verification [with mf_arg > 0]: 0, 1; int (default: 0)
      --sm arg  substitution-based maximization [with mf_arg > 0]: 0, 1; int (default: 0)
      --pw arg  planner wait duration (in seconds); float (default: 0.0)
      --tc arg  thread count, or 0 for hardware_concurrency value; int (default: 1)
      --ts arg  thread slice count [with dp_arg = c]; int (default: 1)
      --rs arg  random seed; int (default: 0)
      --dv arg  diagram var order heuristic: 0/RANDOM, 1/DECLARATION, 2/MOST_CLAUSES, 3/MIN_FILL, 4/MCS,
                5/LEX_P, 6/LEX_M (negatives for inverse orders); int (default: 4)
      --sv arg  slice var order heuristic [with dp_arg = c]: 0/RANDOM, 1/DECLARATION, 2/MOST_CLAUSES,
                3/MIN_FILL, 4/MCS, 5/LEX_P, 6/LEX_M, 7/BIGGEST_NODE, 8/HIGHEST_NODE (negatives for
                inverse orders); int (default: 7)
      --ms arg  mem sensitivity (in MB) for reporting usage [with dp_arg = c]; float (default: 1e3)
      --mm arg  max mem (in MB) for unique table and cache table combined; float (default: 4e3)
      --tr arg  table ratio [with dp_arg = s]: log2(unique_size/cache_size); int (default: 1)
      --ir arg  init ratio for tables [with dp_arg = s]: log2(max_size/init_size); int (default: 10)
      --mp arg  multiple precision [with dp_arg = s]: 0, 1; int (default: 0)
      --jp arg  join priority: a/ARBITRARY_PAIR, b/BIGGEST_PAIR, s/SMALLEST_PAIR; string (default: s)
      --vc arg  verbose CNF processing: 0, 1, 2; int (default: 0)
      --vj arg  verbose join-tree processing: 0, 1, 2; int (default: 0)
      --vp arg  verbose profiling: 0, 1, 2; int (default: 0)
      --vs arg  verbose solving: 0, 1, 2; int (default: 1)
```

### Solving Boolean MPE given XOR-CNF file and join tree from planner
#### Command
```bash
cnfFile="../examples/chain_n100_k10.xcnf" && ../lg/lg.sif "/solvers/flow-cutter-pace17/flow_cutter_pace17 -p 100" <$cnfFile | ./dmc --cf=$cnfFile --wc=1 --er=1 --lc=1 --mf=2
```
#### Output
```
c processing command-line options...
c cnfFile                       ../examples/chain_n100_k10.xcnf
c weightedCounting              1
c projectedCounting             0
c existRandom                   1
c diagramPackage                CUDD
c logCounting                   1
c maximizerFormat               LONG
c maximizerVerification         0
c plannerWaitSeconds            0
c threadCount                   1
c threadSliceCount              1
c randomSeed                    0
c diagramVarOrderHeuristic      MCS
c sliceVarOrderHeuristic        BIGGEST_NODE
c memSensitivityMegabytes       1000
c maxMemMegabytes               4000
c joinPriority                  SMALLEST_PAIR

c processing CNF formula...

c procressing join tree...
c getting join tree from stdin with 0s timer (end input with 'enter' then 'ctrl d')
c processed join tree ending on line 99
c joinTreeWidth                 10
c plannerSeconds                0.0148414
c getting join tree from stdin: done
c killed planner process with pid 221655

c computing output...
c diagramVarSeconds             0
c thread slice counts: { 1 }
c sliceWidth                    10
c threadMaxMemMegabytes         4000
c thread    1/1 | assignment    1/1: {  }
c thread    1/1 | assignment    1/1 | seconds   0.021000 | solution      -20.604935
c apparentSolution              -20.604934991064254746
c ------------------------------------------------------------------
s SATISFIABLE
c s type wmc
c s log10-estimate -20.604934991064254746
c s exact double prec-sci 0.000000000000000000
c ------------------------------------------------------------------
v 1 -2 3 4 5 6 7 8 -9 -10 11 -12 13 14 15 16 17 18 -19 -20 21 -22 -23 -24 -25 26 -27 -28 29 30 31 32 -33 -34 35 -36 37 -38 -39 -40 41 42 -43 -44 45 46 -47 -48 49 50 51 -52 -53 -54 55 -56 57 -58 59 60 -61 62 -63 64 -65 66 67 -68 69 70 71 72 73 -74 75 76 77 -78 -79 80 -81 -82 -83 84 -85 -86 87 -88 89 90 -91 -92 93 -94 95 96 97 -98 -99 -100
c seconds                       0.206000000000000000
```

### Solving WPMC given CNF file and graded join tree from JT file
#### Command
```bash
./dmc --cf=../examples/phi_mc21.cnf --wc=1 --pc=1 <../examples/phi.jt
```
#### Output
```
c processing command-line options...
c cnfFile                       ../examples/phi_mc21.cnf
c weightedCounting              1
c projectedCounting             1
c existRandom                   0
c diagramPackage                CUDD
c logCounting                   0
c plannerWaitSeconds            0
c threadCount                   1
c threadSliceCount              1
c randomSeed                    0
c diagramVarOrderHeuristic      MCS
c sliceVarOrderHeuristic        BIGGEST_NODE
c memSensitivityMegabytes       1000
c maxMemMegabytes               4000
c joinPriority                  SMALLEST_PAIR

c processing CNF formula...

c procressing join tree...
c getting join tree from stdin with 0s timer (end input with 'enter' then 'ctrl d')
c processed join tree ending on line 28
c joinTreeWidth                 2
c plannerSeconds                0
c getting join tree from stdin: done

c computing output...
c diagramVarSeconds             0
c thread slice counts: { 1 }
c sliceWidth                    2
c threadMaxMemMegabytes         4000
c thread    1/1 | assignment    1/1: {  }
c thread    1/1 | assignment    1/1 | seconds   0.016000 | solution        0.400000
c apparentSolution              0.400000000000000022
c ------------------------------------------------------------------
s SATISFIABLE
c s type pmc
c s log10-estimate -0.397940008672037585
c s exact double prec-sci 0.400000000000000022
c ------------------------------------------------------------------
c seconds                       0.016000000000000000
```
