#!/usr/bin/env python3
"""Compile actual engine function bodies against test-only objects on Linux.

No implementation copies are maintained. SEH becomes C++ exceptions in the generated
file only; these checks do not establish Windows ABI or game-runtime correctness.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    # Ignore braces in comments and strings when locating the function end.
    token = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for match in token.finditer(source, brace):
        if match.group() == "{":
            depth += 1
        elif match.group() == "}":
            depth -= 1
            if not depth:
                return source[start:match.end()] + "\n"
    raise ValueError("Unterminated function: " + signature)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture-only", action="store_true")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    build = ROOT / "build" / "portable-regressions"
    build.mkdir(parents=True, exist_ok=True)
    source = (ROOT / "src/plugin/game/EngineInventory.cpp").read_text()
    names = ["unsigned int readInvItems(", "unsigned int captureContainerContents(", "int removeByKey(",
             "static bool applyToInventory(", "bool applyContainerContents("]
    extracted = "\n".join(function(source, name) for name in names)
    extracted = extracted.replace("__try", "try").replace(
        "__except (EXCEPTION_EXECUTE_HANDLER)", "catch (...)")
    extracted = extracted.replace("bool applyContainerContents(", "bool applyContainerContentsImpl(")
    (build / "Inventory-under-test.inc").write_text(extracted)
    repl = (ROOT / "src/plugin/sync/ReplicatorItems.cpp").read_text()
    (build / "Replicator-under-test.inc").write_text("\n".join(function(repl, name) for name in [
        "void Replicator::applyInventories(", "void Replicator::applyXferAcks(",
        "void Replicator::releaseXferLatch("]))
    compiler = os.environ.get("CXX", "g++")
    exe = build / "inventory-regressions"
    flags = ["-std=c++03", "-O1", "-g", "-D__int64=long long"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    subprocess.run([compiler, *flags, "-I" + str(ROOT), "-I" + str(build),
                    str(ROOT / "scripts/tests/portable/InventoryRegression.cpp"),
                    "-o", str(exe)], check=True)
    subprocess.run([str(exe), *(["capture"] if args.capture_only else [])], check=True)
    if not args.capture_only:
        exe = build / "snapshot-regressions"
        subprocess.run([compiler, *flags, "-I" + str(ROOT),
                        str(ROOT / "scripts/tests/portable/SnapshotRegression.cpp"),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
