# Deliverables

What gets handed in. Keep curated, final artifacts here — not raw experiment dumps
(those stay in `workspace/`, gitignored).

```
deliverables/
├── report/     # the write-up (PDF/markdown): approach, results, analysis
└── results/    # final tables, plots, and the CSVs behind them
```

## Expected contents

- **Report** — problem statement, what was connected (Warthog ↔ HOG2), which
  algorithms were run (A\*, MM, BAE\*, BiA\*, GBFS + unidirectional, from both sides),
  with/without diagonal movement, and analysis of the results against the theory in
  the papers (Siag, Eckerle, Holte, Alcazar, …).
- **Results** — per-algorithm metrics: nodes expanded, runtime, solution cost,
  optimality, and meeting-point statistics for bidirectional runs.
