#!/usr/bin/env python3
"""Source integration contracts only; never loads or modifies runtime dependencies."""
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
from portable_regressions import function

root = Path(__file__).resolve().parents[2]
plugin = (root / "src/plugin/Plugin.cpp").read_text()
engine = (root / "src/plugin/game/EngineInternal.cpp").read_text()
project = ET.parse(root / "src/plugin/KenshiCoop.vcxproj")
ns = {"ms": "http://schemas.microsoft.com/developer/msbuild/2003"}
manifest = json.loads((root / "dist/mods/KenshiCoop/RE_Kenshi.json").read_text())
toolsets = project.findall(".//ms:PlatformToolset", ns)
checks = {
    "post-load RE_Kenshi plugin manifest preserved": manifest == {"Plugins": ["KenshiCoop.dll"]},
    "decorated C++ startPlugin export preserved": bool(re.search(r'__declspec\(dllexport\)\s+void\s+startPlugin\(\)', plugin)),
    "every DLL configuration uses mandatory v100": bool(toolsets) and all(p.text == "v100" for p in toolsets),
    "main hook uses existing KenshiLib interface": "KenshiLib::AddHook" in function(plugin, "bool installMainLoopHook(") and "_NV_mainLoop_GPUSensitiveStuff" in function(plugin, "bool installMainLoopHook("),
    "drop hook preserves original engine call": "g_dropItemOrig(self, it)" in function(engine, "void __fastcall dropItem_hook("),
    "normal save hook preserves original engine call": "g_saveHookOrig(self, name, autosave)" in function(engine, "void __fastcall saveMgrSave_hook("),
    "game calls stay out of networking worker": "KenshiLib::" not in (root / "src/plugin/net/NetLink.cpp").read_text() and "engine::" not in (root / "src/plugin/net/NetLink.cpp").read_text(),
}
for name, ok in checks.items():
    print(("PASS " if ok else "FAIL ") + name)
print(f"RE_Kenshi source contracts: {sum(checks.values())}/{len(checks)} PASS (no game/runtime test)")
raise SystemExit(0 if all(checks.values()) else 1)
