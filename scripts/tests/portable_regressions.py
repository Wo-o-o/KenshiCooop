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


def existing_prototest(build, compiler, flags):
    """Run pure tests and actual Inbound queues with single-threaded lock stubs."""
    source = (ROOT / "src/prototest/main.cpp").read_text()
    headers = ["netproto/Wire.h", "netproto/ContentHash.h", "plugin/sync/Interp.h",
               "plugin/core/OwnRanks.h", "plugin/core/SteamId.h", "plugin/core/WorkPose.h",
               "plugin/core/DeathLatch.h", "plugin/game/EngineFaults.h",
               "plugin/game/EngineCaps.h", "plugin/sync/ChangeGate.h", "plugin/core/Inbound.h"]
    prelude = '#include <cstdio>\n#include <cstring>\n#include <set>\n#include <string>\n#include <vector>\n'
    prelude += ''.join('#include "src/' + header + '"\n' for header in headers)
    prelude += 'using namespace coop;\n'
    prelude += source[source.index("static int g_failed"):source.index("// ---- 1.")]
    helpers = ["template <typename T>\nstatic void fillPattern(",
               "template <typename T>\nstatic void roundTrip(", "static InvItemEntry makeEntry(",
               "static EntityState entAt(", "static bool ranksAre("]
    tests = ["testSizes", "testObjectHandLayout", "testEngineFaults", "testEngineCaps",
             "testChangeGate", "testRoundTrips", "testFraming", "testSaveCrc",
             "testFolderFingerprint", "testContentHash", "testInterp", "testOwnRanks",
             "testSteamIdParse", "testWorkPoseMatch", "testTaskClear", "testDeathRekey",
             "testInboundLifecycle", "testFlushWorldStateContract", "testSaveLoadDisconnect"]
    text = prelude + '\n'.join(function(source, name) for name in helpers)
    text += '\n'.join(function(source, "static void " + name + "(") for name in tests)
    text += '\nint main(){' + ''.join(name + '();' for name in tests)
    text += 'printf("portable existing prototest: %d/%d PASS\\n",g_total-g_failed,g_total);return g_failed?1:0;}\n'
    generated = build / "existing-prototest.cpp"
    generated.write_text(text)
    exe = build / "existing-prototest"
    subprocess.run([compiler, *flags, "-I" + str(ROOT / "scripts/tests/portable/win32-shim"),
                    "-I" + str(ROOT), str(generated),
                    str(ROOT / "src/plugin/sync/Interp.cpp"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture-only", action="store_true")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--existing-prototest", action="store_true")
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
    (build / "World-under-test.inc").write_text("\n".join(function(repl, name) for name in [
        "static u32 worldTrackHash(", "RootObject* Replicator::liveWorldProxy(",
        "void Replicator::publishWorldItems(", "void Replicator::applyWorldItems("]))
    save = (ROOT / "src/plugin/sync/SaveXfer.cpp").read_text()
    save_functions = "\n".join(function(save, name) for name in [
        "void sendAbort(", "void sendAbortAndNotify(", "void abortReceive(", "int onSaveDone("])
    # Adapt only MSVCRT's 64-bit printf spelling in generated Linux test code.
    (build / "Save-abort-under-test.inc").write_text(save_functions.replace("%I64u", "%llu"))
    compiler = os.environ.get("CXX", "g++")
    exe = build / "inventory-regressions"
    flags = ["-std=c++03", "-O1", "-g", "-D__int64=long long"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    if args.existing_prototest:
        existing_prototest(build, compiler, flags)
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
        exe = build / "save-abort-regressions"
        subprocess.run([compiler, *flags, "-I" + str(ROOT), "-I" + str(build),
                        str(ROOT / "scripts/tests/portable/SaveAbortRegression.cpp"),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
        exe = build / "world-regressions"
        subprocess.run([compiler, *flags, "-I" + str(ROOT), "-I" + str(build),
                        str(ROOT / "scripts/tests/portable/WorldRegression.cpp"),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
