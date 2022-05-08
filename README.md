# DPO (dynamic-programming optimizer)
DPO runs in two phases:
- The planning phase constructs a project-join tree for an XOR-CNF formula.
- The execution phase computes the maximum and a maximizer from the constructed join tree.

--------------------------------------------------------------------------------

## Cloning this repository
```bash
git clone https://github.com/vuphan314/DPO
```

--------------------------------------------------------------------------------

## Files
- Dir [`lg/`](./lg/): DPO's planner
- Dir [`dmc/`](./dmc/): DPO's executor
- Dir [`eval/`](./eval/): empirical evaluation

--------------------------------------------------------------------------------

## Acknowledgment
- [ADDMC](https://github.com/vardigroup/ADDMC): Dudek, Phan, Vardi
- [BIRD](https://github.com/meelgroup/approxmc): Soos, Meel
- [Cachet](https://cs.rochester.edu/u/kautz/Cachet): Sang, Beame, Kautz
- [CryptoMiniSat](https://github.com/msoos/cryptominisat): Soos
- [CUDD package](https://github.com/ivmai/cudd): Somenzi
- [CUDD visualization](https://davidkebo.com/cudd#cudd6): Kebo
- [cxxopts](https://github.com/jarro2783/cxxopts): Beck
- [DPMC](https://github.com/vardigroup/DPMC): Dudek, Phan, Vardi
- [FlowCutter](https://github.com/kit-algo/flow-cutter-pace17): Strasser
- [htd](https://github.com/mabseher/htd): Abseher, Musliu, Woltran
- [miniC2D](http://reasoning.cs.ucla.edu/minic2d): Oztok, Darwiche
- [Model Counting Competition](https://mccompetition.org): Hecher, Fichte
- [SlurmQueen](https://github.com/Kasekopf/SlurmQueen): Dudek
- [Sylvan](https://trolando.github.io/sylvan): van Dijk
- [Tamaki](https://github.com/TCS-Meiji/PACE2017-TrackA): Tamaki
- [TensorOrder](https://github.com/vardigroup/TensorOrder): Dudek, Duenas-Osorio, Vardi
- [WAPS](https://github.com/meelgroup/WAPS): Gupta, Sharma, Roy, Meel
