#!/usr/bin/env bash
# Stitch chunked run_array outputs back into the canonical files aggregate.py expects.
#
# When the manifest chunks a (map,config) into instance ranges, run_array writes
# base.cfg.pNNNNN.{exp,mvc}.csv per chunk. This merges each set of parts (in
# ascending instance order, one header) into base.cfg.{exp,mvc}.csv and removes
# the parts. Idempotent-ish: safe to run once after the array finishes.
#
#   concat_chunks.sh <results/family-dir>
set -euo pipefail
dir="${1:?usage: concat_chunks.sh <results-family-dir>}"

for kind in exp mvc; do
  # distinct base.cfg stems that have chunk parts
  stems=$(ls "$dir"/*.p[0-9]*."$kind".csv 2>/dev/null \
            | sed -E "s/\.p[0-9]+\.${kind}\.csv$//" | sort -u || true)
  [ -z "$stems" ] && continue
  while IFS= read -r stem; do
    out="${stem}.${kind}.csv"
    # order parts by numeric chunk start
    parts=$(for p in "${stem}".p[0-9]*."$kind".csv; do
              n=$(sed -E 's/.*\.p([0-9]+)\.[a-z]+\.csv$/\1/' <<< "$p")
              printf '%s\t%s\n' "$n" "$p"
            done | sort -n | cut -f2)
    first=1
    : > "$out.tmp"
    while IFS= read -r part; do
      if [ $first -eq 1 ]; then head -1 "$part" >> "$out.tmp"; first=0; fi
      tail -n +2 "$part" >> "$out.tmp"
    done <<< "$parts"
    mv "$out.tmp" "$out"
    echo "merged $(wc -l <<< "$parts") parts -> $(basename "$out")"
  done <<< "$stems"
done

rm -f "$dir"/*.p[0-9]*.exp.csv "$dir"/*.p[0-9]*.mvc.csv
echo "done: chunk parts removed, canonical files in $dir"
