#!/usr/bin/env python3
"""
Seeded shuffle of .3dscen instance order, so any prefix is a representative
sample of the whole map (the benchmark files are difficulty-graded, so the
first-N instances are systematically easy).

Keeps the 2-line header (`version N`, mapname); shuffles only instance lines.
Writes <base>.shuf.3dscen next to each input. Reproducible: same seed -> same
order. Records the seed in <root>/SHUFFLE_SEED.txt.

    shuffle_scens.py <seed> <root_dir>
"""
import sys, os, glob, random

def main():
    seed = int(sys.argv[1]); root = sys.argv[2]
    n = 0
    for scen in sorted(glob.glob(os.path.join(root, "**", "*.3dscen"), recursive=True)):
        if scen.endswith(".shuf.3dscen"):
            continue
        with open(scen) as f:
            lines = f.readlines()
        header, body = lines[:2], lines[2:]
        # per-file deterministic order, seeded by (global seed, filename)
        rng = random.Random(f"{seed}:{os.path.basename(scen)}")
        rng.shuffle(body)
        out = scen[:-len(".3dscen")] + ".shuf.3dscen"
        with open(out, "w") as g:
            g.writelines(header + body)
        n += 1
    with open(os.path.join(root, "SHUFFLE_SEED.txt"), "w") as f:
        f.write(f"seed={seed}\nfiles={n}\n")
    print(f"shuffled {n} scenario files with seed {seed}")

if __name__ == "__main__":
    main()
