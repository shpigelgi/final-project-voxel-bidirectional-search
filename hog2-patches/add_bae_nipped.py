#!/usr/bin/env python3
"""Idempotently add a nodesNipped counter to HOG2's BAE.h (gitignored external tree).
Run after cloning/rsyncing hog2 so driver.cpp's `bae` nip column compiles.
Usage: add_bae_nipped.py [path/to/hog2/generic/BAE.h]"""
import sys, os
f = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "hog2", "generic", "BAE.h")
s = open(f).read()
if "nodesNipped" in s:
    print("BAE.h already patched"); sys.exit(0)
s = s.replace("nodesExpanded = nodesTouched = uniqueNodesExpanded = 0;",
              "nodesExpanded = nodesTouched = uniqueNodesExpanded = nodesNipped = 0;")
s = s.replace("uint64_t GetNodesExpanded() const { return nodesExpanded; }",
              "uint64_t GetNodesExpanded() const { return nodesExpanded; }\n    uint64_t GetNodesNipped() const { return nodesNipped; }")
s = s.replace("uint64_t nodesTouched, nodesExpanded, uniqueNodesExpanded;",
              "uint64_t nodesTouched, nodesExpanded, uniqueNodesExpanded, nodesNipped = 0;")
s = s.replace("            success = true;\n            break;\n        }\n    }\n\n    // This can only fail",
              "            success = true;\n            break;\n        }\n        nodesNipped++;   // closed but already closed on the opposite side: nipped, not expanded\n    }\n\n    // This can only fail")
open(f, "w").write(s)
print("patched BAE.h with nodesNipped counter")
