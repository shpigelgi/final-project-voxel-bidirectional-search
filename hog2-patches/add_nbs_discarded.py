#!/usr/bin/env python3
"""Idempotently add a nodesDiscarded counter to HOG2's NBS.h (gitignored external tree).
Counts the opposite-frontier-closed discard: a consistency-dependent nip that HOG2 only
compiles when ADMISSIBLE is undefined (its default). Run after cloning/rsyncing hog2 so
driver.cpp's `nbs` discard column compiles. Expand()'s #else branch has TWO discard sites
(a node already closed on the opposite frontier, and a fresh node whose reverse is closed);
both are counted, matching the build used for the NBS sub-floor experiment.
Usage: add_nbs_discarded.py [path/to/hog2/generic/NBS.h]"""
import sys, os
f = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "hog2", "generic", "NBS.h")
s = open(f).read()
if "nodesDiscarded" in s:
    print("NBS.h already patched"); sys.exit(0)

# 1. reset all counters together
s = s.replace("nodesExpanded = nodesTouched = 0; counts.clear();",
              "nodesExpanded = nodesTouched = nodesDiscarded = 0; counts.clear();")
# 2. accessor, right after GetNodesExpanded
s = s.replace("\tuint64_t GetNodesExpanded() const { return nodesExpanded; }\n",
              "\tuint64_t GetNodesExpanded() const { return nodesExpanded; }\n"
              "\tuint64_t GetNodesDiscarded() const { return nodesDiscarded; }   // opposite-frontier-closed nips (only compiled when !ADMISSIBLE)\n")
# 3. member declaration
s = s.replace("\tuint64_t nodesTouched, nodesExpanded;\n",
              "\tuint64_t nodesTouched, nodesExpanded, nodesDiscarded;\n")
# 4a. discard site: a node already closed on the opposite frontier is removed from open
s = s.replace("\t\t\t\t\telse if (loc == kClosed)\n\t\t\t\t\t{\n\t\t\t\t\t\tcurrent.Remove(childID);",
              "\t\t\t\t\telse if (loc == kClosed)\n\t\t\t\t\t{\n\t\t\t\t\t\tnodesDiscarded++;\n\t\t\t\t\t\tcurrent.Remove(childID);")
# 4b. discard site: a fresh node whose reverse is already closed is not put on open
s = s.replace("\t\t\t\t\tbreak;\t\t\t//do nothing. do not put this node to open",
              "\t\t\t\t\tnodesDiscarded++;\n\t\t\t\t\tbreak;\t\t\t//do nothing. do not put this node to open")

# Fail loudly if any hunk did not apply — a missing patch must not look like a success.
checks = {
    "reset":     "nodesExpanded = nodesTouched = nodesDiscarded = 0;" in s,
    "accessor":  "uint64_t GetNodesDiscarded() const" in s,
    "member":    "uint64_t nodesTouched, nodesExpanded, nodesDiscarded;" in s,
    "increments (x2)": s.count("nodesDiscarded++;") == 2,
}
missing = [k for k, ok in checks.items() if not ok]
if missing:
    sys.stderr.write("add_nbs_discarded.py: FAILED to apply hunk(s): %s "
                     "(has HOG2's NBS.h layout changed?)\n" % ", ".join(missing))
    sys.exit(1)

open(f, "w").write(s)
print("patched NBS.h with nodesDiscarded counter (2 discard sites)")
