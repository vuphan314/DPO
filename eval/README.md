# Evaluation (Linux)

--------------------------------------------------------------------------------

## Benchmarks

### Downloading archive to this dir (`eval/`)
```bash
wget https://github.com/vuphan314/dpo/releases/download/v0/benchmarks.zip
```

### Extracting downloaded archive into new dir `eval/benchmarks/`
```bash
unzip benchmarks.zip
```

### Files
All formulas have integer weights.
- Dir `benchmarks/xcnf/chain/`: random chain formulas in XOR-CNF (for DPO)
- Dir `benchmarks/xwcnf/chain/`: random chain formulas in XOR-WCNF (for GaussMaxHS)
- Dir `benchmarks/wcnf/chain/`: random chain formulas in pure WCNF (for MaxHS and UWrMaxSat)
  - Converted from XOR-CNF using [Tseitin transformation](https://pyeda.readthedocs.io/en/latest/expr.html#tseitin-s-encoding)

--------------------------------------------------------------------------------

## Binary solvers

### Downloading archive to this dir (`eval/`)
```bash
wget https://github.com/vuphan314/dpo/releases/download/v0/bin.zip
```

### Extracting downloaded archive into dir `eval/bin/`
```bash
unzip bin.zip
```

### Files
- `bin/lg.sif`: DPO's [planner](../lg/)
- `bin/dmc`: DPO's [executor](../dmc/)
- `bin/gaussmaxhs`: [GaussMaxHS](https://github.com/meelgroup/gaussmaxhs)
- `bin/maxhs`: [MaxHS](https://maxsat-evaluations.github.io/2021/mse21-solver-src/complete/maxhs.zip)
- `bin/uwrmaxsat`: [UWrMaxSat](https://maxsat-evaluations.github.io/2021/mse21-solver-src/complete/uwrmaxsat.zip)
- `bin/runsolver`: [tool to control and measure resource consumption](https://github.com/utpalbora/runsolver)

### DPO
```bash
./wrapper.py --solver=dpmc --cf=../examples/chain_n100_k10.xcnf --wc=1 --er=1 --mf=1 | ./postprocessor.py 2>/dev/null
```
#### Output
```
logsol:183.000000000000000000
model:1011111100101111110010000100111100101000110011001110001010110101011011111011100100010010110010111000
```
- The base-10 logarithm of the maximum is `183`.
- A maximizer is `1011111100101111110010000100111100101000110011001110001010110101011011111011100100010010110010111000` (list of values of variables in declared order).

### GaussMaxHS
```bash
./wrapper.py --solver=gauss --cf=../examples/chain_n100_k10.xwcnf | ./postprocessor.py 2>/dev/null
```

### MaxHS
```bash
./wrapper.py --solver=maxhs --cf=../examples/chain_n100_k10.wcnf | ./postprocessor.py 2>/dev/null
```

### UWrMaxSat
```bash
./wrapper.py --solver=uwr --cf=../examples/chain_n100_k10.wcnf | ./postprocessor.py 2>/dev/null
```

--------------------------------------------------------------------------------

## Data

### Downloading archive to this dir (`eval/`)
```bash
wget https://github.com/vuphan314/dpo/releases/download/v0/data.zip
```

### Extracting downloaded archive into dir `eval/data/`
```bash
unzip data.zip
```

### Files
- Dir `data/chain/*/dpmc/noprune/xor/`: DPO on XOR-CNF benchmarks (with no pruning)
  - Files `*.in`: commands
  - Files `*.log`: raw outputs
  - Files `*.out`: postprocessed outputs
- Dir `data/chain/*/gauss`: GaussMaxHS
- Dir `data/chain/*/maxhs`: MaxHS
- Dir `data/chain/*/uwr`: UWrMaxSat

--------------------------------------------------------------------------------

## [Jupyter notebook](dpo.ipynb)
- At the end of the notebook, there are two figures used in the paper.
- Run all cells again to re-generate these figures from dir `data/`.
