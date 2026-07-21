---
name: bgu-cluster
description: >
  Run this project's voxel bidirectional-search benchmarks on the BGU CIS/ISE/CS
  HPC Slurm cluster (slurm.bgu.ac.il). Use this skill WHENEVER the user wants to
  build, submit, monitor, or collect results from cluster runs — e.g. "run the
  benchmarks on the cluster", "submit the jobs", "sbatch", "how's my job doing",
  "aggregate the cluster results", or mentions the BGU cluster, Slurm, sbatch,
  squeue, or the login node. It encodes the cluster's specific quirks (no system
  C++ compiler, GitHub blocked, dedicated CPU partition) that generic Slurm advice
  will get wrong.
---

# Running the voxel benchmarks on the BGU HPC cluster

The cluster is a Slurm cluster. You connect by SSH to the **login/manager node**
`slurm.bgu.ac.il` (key-based auth is already set up for user `shpigelm`; never type a
password). **Never run computation on the login node** — it is shared and only for
building/submitting/monitoring. Compute happens on nodes via `sbatch`.

## Cluster-specific quirks (these bite if you assume a normal Slurm box)

1. **No C++ compiler anywhere.** Login *and* compute nodes have `gcc` but **no `g++`,
   no `cc1plus`, no compiler modules**. You MUST build with a **conda** toolchain and
   **static-link** so the binary runs on compute nodes without the conda env:
   ```bash
   module load anaconda
   conda create -y -n voxbuild -c conda-forge gxx_linux-64
   source activate voxbuild
   CXX="$(command -v x86_64-conda-linux-gnu-g++)" \
     XFLAGS="-static-libstdc++ -static-libgcc" \
     bash workspace/src/build.sh workspace/bin
   ```
   `build.sh` honors `$CXX`/`$XFLAGS`. Verify with `file workspace/bin/voxdriver` and a
   quick `workspace/bin/voxdriver` (no args prints usage).
2. **GitHub is blocked** from the cluster; **Bitbucket is reachable**. So:
   - Ship **code + the HOG2 clone** from the local machine with `rsync` (don't `git
     clone` on the cluster). `scripts/setup.sh` (which clones HOG2 from GitHub) will
     NOT work here.
   - **Benchmarks CAN be fetched on the cluster** via `scripts/fetch-benchmarks.sh`
     (it pulls from Bitbucket) — no need to upload the multi-GB map files.
3. **Use the `cpu` partition** — our jobs are CPU-only (no GPU). It has ~80 CPU nodes;
   using it leaves the GPU nodes for GPU users. (`main` also works but mixes GPU nodes.)
   Never add `--gpus`. Do not use `qos`/other partitions unless a GPU is needed.
4. **Etiquette (enforced culture):** allocate **minimum RAM** (our jobs fit in ~8G;
   `--mem=8G`), 1 CPU per task, cancel stray jobs (`scancel`), and prefer **job arrays
   over thousands of tiny jobs** (the scheduler and filesystem choke on rapid-fire tiny
   jobs — our harness already batches one array task per map×config).

## One-time setup (from the local repo, `hws/hw-01/`)

```bash
REMOTE=shpigelm@slurm.bgu.ac.il ; DEST=voxel-bihs
# 1. Ship code (small) — exclude the huge/gitignored trees; HOG2 shipped separately.
rsync -az --delete --exclude '.git' --exclude 'workspace/hog2' \
  --exclude 'workspace/benchmarks' --exclude 'workspace/bin' \
  --exclude 'workspace/results' ./ "$REMOTE:$DEST/"
# 2. Ship the HOG2 source tree (GitHub is blocked on the cluster).
rsync -az --delete --exclude '.git' workspace/hog2/ "$REMOTE:$DEST/workspace/hog2/"
```
Then on the cluster: build (quirk #1) and fetch benchmarks:
```bash
ssh $REMOTE
cd voxel-bihs/workspace
./scripts/fetch-benchmarks.sh industrial-plants        # small; add sandstone / all later
#  ... build via the conda block above ...
```

## Submit (job array — the whole benchmark sweep)

`cluster/submit.sh` builds (if a compiler is on PATH), generates the manifest
(one unit per map × {diag,nodiag}), and submits a Slurm array. On the cluster, build
first (quirk #1), then submit with the CPU partition and sensible caps:
```bash
cd ~/voxel-bihs/workspace
PARTITION=cpu TIMEOUT=60 MEMMB=8000 CONC=16 LIMIT=200 \
  ./cluster/submit.sh industrial-plants     # first, bounded validation run
# full sweep later: drop LIMIT, add families:  ./cluster/submit.sh   (all present)
```
Tunables (env): `TIMEOUT` per-instance seconds, `MEMMB` per-child cap (keep < `--mem`),
`ALGS`, `LIMIT` max instances/map, `CONC` max concurrent array tasks, `PARTITION`.
Each `(instance,algorithm)` runs in a forked child with a wall-clock timeout and an
RLIMIT_AS memory cap, and rows are flushed incrementally, so a hung/huge instance can't
stall the node and partial results survive a time-limit kill.

## Monitor

```bash
squeue --me                                  # my queue
sacct -j <jobid> --format=JobID,JobName,State,Elapsed,MaxRSS,ExitCode
tail -f ~/voxel-bihs/workspace/cluster/logs/<arrayJobId>_<taskId>.out
scancel <jobid>            # stop a job;  scancel -t PENDING -u shpigelm  # all my pending
sinfo -p cpu -o "%.6D %.4c %.8m %.6t"        # CPU partition availability
```

## Collect results

Raw per-map CSVs land in `workspace/results/<family>/`. Aggregate + join with the
must-expand floor:
```bash
python3 ~/voxel-bihs/workspace/cluster/aggregate.py ~/voxel-bihs/workspace/results
```
It writes `results/aggregated/{combined_long.csv,summary.csv}` and prints per
(family,config,algorithm): %ok, %timeout, mean expanded, mean/median expanded-over-MVC,
and a cross-algorithm optimality-consistency check. Copy `summary.csv` back to the local
`deliverables/results/` for the report:
`rsync -az $REMOTE:voxel-bihs/workspace/results/aggregated/ deliverables/results/aggregated/`

## Reference
Full cluster runbook (BGU-agnostic details, resource tables): `workspace/cluster/RUNBOOK.md`.
Cluster user guide PDF quirks are all captured above; if something here conflicts with a
newer guide, the guide wins — re-verify partitions with `sinfo` and compiler with
`command -v g++`.
