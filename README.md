# DPO (dynamic-programming optimizer)
DPO runs in two phases:
- The planning phase constructs a project-join tree for an XOR-CNF formula.
- The execution phase computes the maximum and a maximizer from the constructed join tree.

<!-- ####################################################################### -->

## Cloning this repository
```bash
git clone https://github.com/vuphan314/dpo
```

<!-- ####################################################################### -->

## Files
- Dir [`lg/`](./lg/): DPO's planner
- Dir [`dmc/`](./dmc/): DPO's executor
- Dir [`eval/`](./eval/): empirical evaluation
- File [`ACKNOWLEDGMENT.md`](./ACKNOWLEDGMENT.md)
