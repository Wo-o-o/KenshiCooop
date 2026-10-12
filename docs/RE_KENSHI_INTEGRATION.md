# Mandatory RE_Kenshi runtime integration

KenshiCoop is a RE_Kenshi/KenshiLib plugin, not a standalone replacement. Do not update, rebuild, patch or redistribute RE_Kenshi, KenshiLib or their RVA files as part of stabilization without explicit approval. The source checkouts used for investigation are outside this repository and have not been altered.

## Evidence and supported baseline

The official v0.51 source is `5a761e19a4184b42315e96ea7e92e3c1403d54db`. Its verified release DLL is AMD64 and imports `KenshiLib.dll`, `OgreMain_x64.dll`, `MyGUIEngine_x64.dll`, MSVCP100/MSVCR100 and Windows networking/system libraries. It exports **`?startPlugin@@YAXXZ`**, not a C export. Its KenshiLib imports include the existing `GetRealAddress(void*)` and `AddHook(void*,void*,void**)` interfaces.

| Runtime | Evidence | Validation status |
| --- | --- | --- |
| Kenshi 1.0.65 Steam x64 + RE_Kenshi 0.3.1 or later | KenshiCoop README's documented requirement | Documented target; not run here |
| RE_Kenshi **v0.3.1**, commit `221cbc3aed510296f8818015ae03060f1bf05d9e` | Public loader resolves the same export; its KenshiLib gitlink is `b9eaec0dd155c6ee4d3e5ff9ae6763fe578c7350`, with the required interfaces | Source-level contract checked; full binary/runtime compatibility unverified |
| RE_Kenshi source HEAD `c8dfbc9621b9263863722852f819c4c0f440475a` | Public source reports 0.3.5 prerelease and retains plugin entry contract | Investigated only; not a recommendation to upgrade |
| Other RE_Kenshi 0.3.x releases | “0.3.1+” documentation is not an exhaustive tested matrix | Each installed build needs validation with its matching KenshiLib/RVAs |
| Kenshi GOG 1.0.65 | RE_Kenshi/KenshiLib source recognizes it | KenshiCoop-specific gameplay has not been validated here |
| Native Kenshi 1.0.68 / other architectures / modern-MSVC plugin | No v0.51 runtime evidence | Not established as supported by this work; newer RE installer conversion does not prove KenshiCoop compatibility |

Public sources:

- [RE_Kenshi v0.3.1 loader](https://github.com/BFrizzleFoShizzle/RE_Kenshi/blob/v0.3.1/Plugins.cpp)
- [RE_Kenshi v0.3.1 lifecycle](https://github.com/BFrizzleFoShizzle/RE_Kenshi/blob/v0.3.1/dllmain.cpp)
- [RE_Kenshi v0.3.1 RNG hooks](https://github.com/BFrizzleFoShizzle/RE_Kenshi/blob/v0.3.1/MiscHooks.cpp)
- [KenshiLib hook/address implementation used by that tag](https://github.com/BFrizzleFoShizzle/KenshiLib/blob/b9eaec0dd155c6ee4d3e5ff9ae6763fe578c7350/Source/core/Functions.cpp)

There is no evidence establishing which RE_Kenshi/KenshiLib/RVA binaries the players actually have installed. Record exact versions and SHA-256 on both machines before interpreting live results. A matching API signature does not prove matching object layout, RVA tables or compatibility of other loaded hooks.

## Lifecycle and responsibilities

1. RE_Kenshi initializes KenshiLib and loads the version/platform-specific RVA table. KenshiLib resolves linked function stubs into Kenshi executable addresses and installs detours using its hook implementation. A mismatched RVA table is a runtime blocker, not something KenshiCoop should repair by guessing addresses.
2. RE_Kenshi's mod-load hook calls the original loader, then `Plugins::Postload` once. Enabled mod `RE_Kenshi.json` files provide the `Plugins` list. KenshiCoop's manifest remains `{ "Plugins": ["KenshiCoop.dll"] }`, **not** a preload plugin.
3. `Plugin.cpp::startPlugin` installs crash logging, the original main-loop hook, resolves engine functions/capabilities, installs existing detours and title/UI hooks, then configures networking. Hook return values and capability logs must be checked at runtime.
4. Main-loop/title hooks and engine detours remain KenshiLib hooks. Their original/trampoline calls must be preserved. KenshiCoop owns replication, queues, authority decisions, proxy bookkeeping and coordinated save/load policy. World object memory, inventories, loot generation, save serialization and streaming belong to Kenshi.
5. World reload invalidates cached game pointers and replication state. RE_Kenshi is not a per-session network coordinator; KenshiCoop must clear its own state without unloading/reinitializing RE_Kenshi or bypassing it.
6. Steam P2P uses the existing Steam Networking/ENet tunnel. File transfer reads/writes Kenshi's resolved save directory using Win32 IO. There is no Steam RemoteStorage/Cloud API in the inspected transfer path.

Memory access must remain in the game's main thread. Field reads and virtual calls use KenshiLib's game headers, v100 layout and SEH guards; portable mocks cannot validate any of those ABI assumptions. Keep the v100 x64 plugin toolchain and existing headers/link libraries.

## Interaction analysis

- **Inventory duplication:** confirmed control-flow defects A01/A02/A03 do not require changing RE_Kenshi. Engine-read failures can still be caused by an incompatible KenshiLib/RVA bundle, hook failures, lifecycle timing or unloaded objects; record CAP/FAULT lines before blaming replication alone.
- **Tavern items:** v0.51's spatial scan promotes later-loaded native items into W1, whose factory creates owner=0 proxies. This explains owned red native plus free grey copy directly. RE_Kenshi's optional RNG fix touches CRT random functions and `Building::selectParts`; it can affect generation context, but it is not proof of the duplicated proxy path or identical native loot on both clients. Keep RE settings unchanged; record them on both clients.
- **Drop attribution:** the reviewed PR69 fix depends on KenshiCoop's existing `Inventory::_NV_dropItem` capture hook being installed through KenshiLib and calling its original trampoline. A failed hook must not be mistaken for proof that a new item is native. Validate `[engine]`/drop-hook startup logs and real player/NPC drops on both clients.
- **Save IO:** RE_Kenshi source includes `_fsopen`/`FindFirstFileW` override hooks; inspected SaveXfer uses `CreateFileA`/`FindFirstFileA`, so those hooks do not directly intercept its file writes. Indirect effects through engine serialization, non-ASCII paths, other file hooks or locking remain possible and unproven.
- **Crash/emergency save:** KenshiCoop's crash filter chains the previous handler. RE_Kenshi can request emergency `SaveManager::saveGame`; this is distinct from KenshiCoop's normal `SaveManager::save` detour. Preserve the chain and do not suppress, replace or alter RE's recovery policy.
- **Game speed/UI:** RE provides speed/UI controls; KenshiCoop captures game speed through existing engine hooks. Test with existing settings and do not replace RE's controls.

No inspected defect currently establishes that RE_Kenshi itself must change. If runtime evidence shows failed address resolution or hook incompatibility, the smallest next step is diagnose the **installed matching bundle** and failed operation. Installing a new dependency or patching RE requires explicit approval.

## Required integration validation (not executed here)

Use disposable **copies** of saves and existing known-good installations; never alter the originals to force convergence.

1. On both machines record Kenshi executable hash/version, RE_Kenshi displayed version and DLL hash, KenshiLib hash, platform RVA file hash, KenshiCoop hash/protocol, enabled mods and relevant RE settings. Check RE loaded before renderer and loaded `KenshiCoop.dll` from the expected mod path exactly once.
2. Build plugin Harness and Release with genuine VC++ 2010 v100 x64. Inspect PE imports/exports with `dumpbin`; confirm the decorated `startPlugin` and dependency names. A modern compiler build is not an ABI test.
3. Start Kenshi **with RE_Kenshi and KenshiCoop together**. Require successful RE version recognition/RVA initialization, KenshiCoop main/title hooks and inventory/drop/save capabilities. Record hook installation failures and CAP-MISS/FAULT before proceeding.
4. Connect Host/Join through Steam P2P. Verify F2, roles, owned squads, movement and RE speed controls. Perform bag empty/refill/drop/pickup/direct trade in both directions, including 64-entry boundaries. Count units and confirm ownership and nested placement.
5. Visit a tavern loaded late, reload, return and compare native/proxy identity and theft status. Exercise genuine player and NPC drops to verify the existing capture hook remains functional.
6. Run 90–120 minutes with travel, combat and item transfers. Save/reload and reconnect; verify health/death, inventory, squad membership/names/jobs and absence of orphan proxies. Existing source fixes do not certify this soak test.
7. Transfer 3/8/16 MB saves with paired logs and exact byte fingerprints. Test normal/manual/autosave, delayed delivery, disconnect, IO refusal on a disposable fixture and a non-ASCII user save path. Verify prior save byte integrity on every failure; record whether Steam Cloud has a file lock or reported error before attributing causality.
8. If a crash occurs, preserve KenshiCoop log, RE log, `save.log`, dump and installed binary hashes. Confirm previous crash handler chaining without deliberately crashing a production session.

Linux checks validate local logic and source contracts only. No change is called verified in-game until this combined runtime passes the relevant steps.
