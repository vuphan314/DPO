# HTB (heuristic tree builder)
HTB constructs join trees for XOR-CNF formulas.

--------------------------------------------------------------------------------

## Installation (Linux)

### Prerequisites
#### External libraries
- boost 1.66
- g++ 13.3
- gmp 6.2
- make 4.2
#### [Included libraries](../addmc/libraries/)
- [cxxopts 2.2](https://github.com/jarro2783/cxxopts)

### Command
```bash
make htb
```

--------------------------------------------------------------------------------

## Examples

### Showing options
#### Command
```bash
./htb
```
#### Output
```
Heuristic Tree Builder
Usage:
  htb [OPTION...]

      --cf arg  CNF file path; string (REQUIRED)
      --pc arg  projected counting: 0, 1; int (default: 0)
      --rs arg  random seed; int (default: 0)
      --cv arg  cluster var order heuristic: 0/RANDOM, 1/DECLARATION, 2/MOST_CLAUSES, 3/MIN_FILL, 4/MCS,
                5/LEX_P, 6/LEX_M (negatives for inverse orders); int (default: 5)
      --ch arg  clustering heuristic: bel/BUCKET_ELIM_LIST, bet/BUCKET_ELIM_TREE,
                bml/BOUQUET_METHOD_LIST, bmt/BOUQUET_METHOD_TREE; string (default: bmt)
      --vc arg  verbose CNF processing: 0, 1, 2; int (default: 0)
      --vs arg  verbose solving: 0, 1, 2; int (default: 1)
```

### Finding join tree given CNF formula from file
#### Command
```bash
./htb --cf=../examples/s27_3_2.wpcnf
```
#### Output
```
c htb process:
c pid 24797

c processing command-line options...
c cnfFile                       ../examples/s27_3_2.wpcnf
c projectedCounting             0
c randomSeed                    0
c clusterVarOrderHeuristic      LEX_P
c clusteringHeuristic           BOUQUET_METHOD_TREE

c processing CNF formula...

c computing output...
c ------------------------------------------------------------------
p jt 20 43 105
72 29 e
73 30 e
74 31 e
75 32 e
76 33 e
70 27 e
71 28 e
44 1 e
45 2 e
52 9 e
54 11 e
65 22 e
66 23 e
46 3 e
48 5 e
59 16 e
60 17 e
58 15 e
88 58 e
61 18 e
62 19 e
63 20 e
49 6 e
87 49 e
50 7 e
51 8 e
89 50 51 e 4
90 61 62 63 87 89 e 10
91 59 60 88 90 e 13
47 4 e
92 47 e
93 46 48 91 92 e 6 9
64 21 e
94 64 e
95 65 66 93 94 e 5 14
55 12 e
56 13 e
57 14 e
96 55 56 57 e 3
53 10 e
97 53 e
98 52 54 95 96 97 e 2 11 7
67 24 e
68 25 e
69 26 e
99 67 68 69 e 16
100 44 45 98 99 e 1 8
101 70 71 100 e 15
82 39 e
83 40 e
84 41 e
85 42 e
86 43 e
102 82 83 84 85 86 e 20
77 34 e
78 35 e
79 36 e
80 37 e
81 38 e
103 77 78 79 80 81 e 19
104 72 73 74 75 76 101 102 103 e 12 18 17
105 104 e
c ------------------------------------------------------------------
c joinTreeWidth                 6
c seconds                       0.004
```
