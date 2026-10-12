# KenshiCoop v0.51 stabilization — living report

Development branch: `dev/stabilize-v0.51`. No changes are merged into main or released.

Session reviewed on 2026-10-12. [Original audit](TECHNICAL_AUDIT_V051.md) is a frozen baseline finding set; this report records what changed afterward. [Mandatory RE_Kenshi integration](RE_KENSHI_INTEGRATION.md) records source evidence, runtime uncertainty and validation procedures. RE_Kenshi/KenshiLib/RVA dependencies have not been modified, updated or redistributed.

## Verified baseline

- Upstream: https://github.com/nhoral/KenshiCoop/releases/tag/v0.51
- Annotated tag object: `807319ef539e21706e11e9ef6241e5c3b9f47128`.
- Source commit: `5a761e19a4184b42315e96ea7e92e3c1403d54db`. The original audited checkout and selected fork main match this source exactly.
- Release archive SHA-256: `4caa44f55a3d70c1b63a2841e9a60259dc2d8241b54b937e91d9ff91f3aee973`, verified against GitHub's asset digest.
- Release PROVENANCE: protocol 55, Release configuration; DLL SHA-256 `f1ea4c620c18f514649745ec3cdedb9ecefdb3073281bc8e6be287273b617222`.
- This identifies the reference release, not the actual binaries installed on the two players' machines. Both logs/builds should be checked before live validation.
- Protocol layout remains 55 in these changes. Required plugin toolchain remains VC++ 2010 v100 x64; modern MSVC is not an ABI substitute. Kenshi 1.0.65 and RE_Kenshi remain the documented runtime.

## Upstream review

Reviewed release/tag/source, both pages of open Issues and PRs, and relevant individual discussions/comments. GitHub API was blocked by the environment's proxy; public HTML pages and native Git worked. Observed PR state refers to upstream, not forks mentioned in comments.

| Upstream | State / relevance | Decision |
| --- | --- | --- |
| [PR48](https://github.com/nhoral/KenshiCoop/pull/48) | Merged, already in baseline: nested parentIdx, truncation flag, gear conservation, W1 claims and drop bookkeeping | Keep existing fixes; address remaining boundary defects |
| [PR35](https://github.com/nhoral/KenshiCoop/pull/35) | Open, alternative nested inventory keys and disconnect protection; based on older protocol | Do not merge competing wire design |
| [PR16](https://github.com/nhoral/KenshiCoop/pull/16) | Open: empty-transfer/name validation and stranded-save recovery | Review/reuse relevant implementation before save changes; no blind merge |
| [PR68](https://github.com/nhoral/KenshiCoop/pull/68) | Closed without merge; faction-owned item filter | Superseded by PR69; no claim that it is already fixed in v0.51 |
| [PR69](https://github.com/nhoral/KenshiCoop/pull/69) | Open: ownership filter plus scan-only native baseline | Reviewed selected five-file change ported; credited to jayceslesar. No upstream PR merge |
| [PR70](https://github.com/nhoral/KenshiCoop/pull/70) | Open: native pickup notice, protocol 56 | Not included automatically: changes protocol; native pickup remains tracked |
| [PR80](https://github.com/nhoral/KenshiCoop/pull/80) | Draft/open: inventory save fence, protocol 60; fork comments describe a refined variant | Save freshness is separate from byte-transfer integrity; defer wire change pending review |
| PR50 | Open: three-player support | Outside two-player stabilization scope |

Community evidence: [#63](https://github.com/nhoral/KenshiCoop/issues/63) and [#66](https://github.com/nhoral/KenshiCoop/issues/66) describe stolen/native plus free-copy town items; [#44](https://github.com/nhoral/KenshiCoop/issues/44) describes repeated native pickup. [#61](https://github.com/nhoral/KenshiCoop/issues/61), [#77](https://github.com/nhoral/KenshiCoop/issues/77), [#56](https://github.com/nhoral/KenshiCoop/issues/56), [#57](https://github.com/nhoral/KenshiCoop/issues/57) describe backpacks and transfer loss. [#72](https://github.com/nhoral/KenshiCoop/issues/72)/[#82](https://github.com/nhoral/KenshiCoop/issues/82) describe long-session desync. [#74](https://github.com/nhoral/KenshiCoop/issues/74) distinguishes missing squad/job metadata at serialization from clean file transfer and reports crashes around an hour. [#55](https://github.com/nhoral/KenshiCoop/issues/55) includes a non-ASCII save-root hypothesis. These are reports, not reproduced gameplay here.

## Fix status

| ID | State | Evidence / remaining work |
| --- | --- | --- |
| A02 | Implemented, Linux control-flow verified | Continue enumeration past capacity to mark actual omitted children; unavailable section reads are incomplete |
| A01 | Implemented with safety limits | Clear uniquely resolved empty bag only from complete valid snapshots; incomplete local enumeration or ambiguous same-template bags cannot authorize clearing |
| A03 | Implemented, Linux verified | Compact adjusted parent graph atomically; preserve nested units; synthetic bag entries are incomplete |
| A06 | Pending | Nested transfer detection/relocation and distinguishing missing vs refused |
| A07 | Partially fixed | ACCEPT keeps latch until matching snapshot; existing grace expiry and partial rollback still need work |
| A05 | Pending | Failed gear drops/pickups need safe retries, not early dedup completion |
| A09 | Partially fixed, Linux verified | Remember consumed W1 ids until reset, suppress delayed/replayed snapshots. Failed author-side claim deletion/retry remains |
| A11/A04/A12 | Pending | Distinguish read/apply outcomes, translate proxy hands, fence old world data |
| A13/A14 | Pending | Validate on-disk transfer, offsets/lengths, path components and atomic commit |
| A15 | Partially fixed, Linux verified | Notify receiver of sender IO failure with existing DONE failure marker; abort receiver and discard save/load queues on disconnect. Cross-channel ordering and delayed old packets remain |
| A08/A10 | Pending | Item attributes and world-proxy quantity/quality |
| Tavern duplication | Source path addressed; runtime pending | Reviewed PR69 port: later scan-only natives never stream, faction-owned shop items excluded, existing hook-authored real drops preserved |

“Implemented” means the identified control-flow defect is corrected and covered by local tests, not a claim that the full issue is solved in-game. A01 intentionally defers ambiguous same-template empty bags. A11 still includes an unavailable-container read interpreted as empty; this must be resolved before calling all snapshot failure paths safe. A07 still has the existing ten-second grace expiry and partial-ACK rollback limitations. A09 tombstones grow with consumed items until session reset; old-world packets and native simultaneous pickup remain unresolved.

## Changed files and scope

- `EngineInventory.cpp`: A02 enumeration/completeness, A01 safe empty nested reconciliation; reviewed faction ownership scan filter. `Engine.h` documents the scan contract.
- `InventorySnapshot.h`, `ReplicatorItems.cpp`: A03 graph compaction and latch adjustments; A07 ACCEPT retention/aligned packed ACK access; reviewed PR69 drop attribution and baseline; A09 consumed-id protection.
- `Replicator.h`, `ReplicatorCore.cpp`: pending drop and consumed-id state, cleared on world/session reset.
- `SaveXfer.cpp/.h`: sender IO failure diagnostics/notification; receiver teardown preserving the prior save. Existing protocol 55 packet layout is unchanged.
- `Plugin.cpp`, `Inbound.h`: disconnect teardown discards queued save/load handshake; ordinary world reload preserves it. A concurrent rapid reconnect/event-order race remains tracked under A12/A15.
- `src/prototest/main.cpp`, `scripts/tests/portable/*`, `portable_regressions.py`: regression tests against current implementation, including native Windows Inbound tests and Linux-only mock adaptations.
- `check_re_integration.py`, `.github/workflows/stabilization.yml`, `build_snapshot_tests.cmd`: source integration contracts and Ubuntu CI; manual genuine-v100 Windows job requiring an already provisioned trusted runner.
- Documentation and `.gitignore`: living evidence, runtime validation procedures and generated Python cache exclusion.

## Tests

`python3 scripts/tests/portable_regressions.py --sanitize` builds extracted **current production function bodies** with minimal engine mocks, C++03, AddressSanitizer and UBSan. SEH is adapted only in generated test files; this is not a DLL or Win32 ABI test.

- A02 red baseline: 7/13 passing; failed top/section/parent boundaries, destructive incomplete reconcile, unavailable reads. After fix: 13/13.
- A01 red baseline after A02: 16/19 passing; empty nested inventories remained stale. Initial fix passed 19/19.
- Added safety regressions found another failure with incomplete local parent enumeration. Final inventory checks: **22/22 PASS**, sanitizers clean.
- Current inventory checks: **31/31 PASS**; snapshot graph policy: **10/10 PASS**. Cases additionally exercise actual `applyInventories`, synthetic latch parents, top/nested identical templates, delayed/duplicate ACCEPT and rejected ACK. UBSan found an unaligned packed ACK field passed by reference into `map::find`; copying to an aligned local fixed it. Sanitizer runs now fail immediately on errors instead of recovering.
- Reviewed PR69 negative control: original v0.51 publisher fails late-native and 90-minute-later native tests; current publisher passes. World regression suite: **12/12 PASS**, including genuine hook-authored drops, unresolved-hook recovery, peer echo, replay and post-pickup delayed snapshots.
- Save abort suite: **11/11 PASS**, including partial 3-MB failure marker, duplicate/stale markers, teardown, and no prior-save deletion/commit. Filesystem calls are observed mocks, not real Win32 IO.
- Existing pure prototest plus actual Inbound queue logic: **521/521 PASS** (original pure subset 456 plus 65 queue checks). Single-threaded Linux lock stubs do not validate Win32 concurrency. The disconnect-specific tests are also wired into the native v100 prototest.
- Disconnect negative control: replacing only queue cleanup with a no-op produces **513/521 PASS**, failing all eight stale save/load queue checks. Production files were unchanged by this control.
- Final sanitized run: **585/585 logical checks PASS**; RE source integration contracts **7/7 PASS**; existing PowerShell contract fixtures **32/32 PASS**. Sanitizers fail immediately on errors and reported none.
- Packet fault coverage is local replay/delay, omission/incomplete snapshots and queue lifecycle. Actual ENet/Steam loss/reordering, queue pressure, OS locking and concurrent reconnect are not tested. No full transport fault campaign is claimed.

Windows v100 DLL/prototest and two-player Kenshi/Steam tests have **not run**.
The new workflow has been syntax-parsed locally, not executed by GitHub Actions. The Windows job requires genuine compiler version 16/v100, matching approved headers/libraries and a trusted runner; no modern-MSVC fallback or dependency install is provided.

## Gameplay investigations

### Tavern duplicates

Strong source explanation corroborated by PR69: the first-scan baseline covers only the first loaded area. Shop items encountered later are treated as new drops and streamed; `spawnWorldItemProxy` creates them with owner=0. The receiver already has the owned native, so sees a native red/stolen item plus a free grey proxy. Late interior generation can also defeat a time-only baseline. This differs from both engines generating different loot and from duplicate pickup of a shared native. No live trace from the players is available to prove which path occurred in their session.

### Transfer stalls around 3 MB

No fixed 3 MB limit has been established. Sender queues 32 x 4 KiB per 50 ms, while actual ENet/Steam delivery can be slower. Sender completion means queued bytes, not acknowledged reception. Confirmed source defect: sender open/read failure previously stopped only its own state and sent no terminal message; receiver stayed active indefinitely. The fix emits an existing zero-file DONE, which mismatches the nonempty BEGIN and fails safely on the v0.51 receiver. It logs immediate Win32 error, file/offset/length/path. Disconnect closes receive IO and discards queued BEGIN/FILE/DONE/ACK and load requests so stale queued work cannot restart it. Neither proves the observed 3-MB failure was IO-related.

Still pending: receiver timeout/heartbeat, actual transport backlog/ACK measurements, disk-integrity/path hardening, cross-channel order and save freshness. Disconnect receiver cleanup is not complete coordinator teardown: sender/watch state, coordinator pending names and outbound old reliable work also need a reviewed reset/fence before reconnect is certified safe. A stalled main thread cannot process the terminal message promptly. Win32 ANSI paths and file IO refusal are candidates. Steam Cloud is unverified: this transfer uses ENet over Steam Networking, not Steam RemoteStorage; locks could indirectly interfere but need IO error evidence. Do not disable Steam Cloud or attribute the failure to it without that evidence.

## Next implementation priorities

1. A11: distinguish unavailable reads from empty inventories and deferred/failed engine application from convergence. This strengthens A01 before broadening reconciliation.
2. A06/A07: preserve quantity on nested cross-owner transfers and destination refusal; separate missing/read-failed/refused results before fabrication; review partial/rejected rollback and expiry.
3. A05/A09: retry failed engine drops/pickups/claims using confirmed outcomes while deduplicating successful operations. A08/A10 require property-preserving reconciliation.
4. A04/A12: reuse existing live character mapping consistently; fix captured motion/epoch pairing and inspect every world-state channel without dropping the load coordinator.
5. A13/A14/A15: review PR16 validations, reject invalid paths/offsets, verify actual disk contents and stage/commit integrity; then address ordered reception and bounded failure recovery. Reassess PR80 save freshness separately from byte integrity.

Keep changes small and add red regressions before broadening each fix. Wire-format or load-handshake changes need a compatibility decision; no PR is to be merged wholesale.

## Development commits

- `b0773ae`: A02 and conservative A01, portable regression foundation.
- `975d2db`: A03 and partial A07, parent/quantity and ACK regressions.
- `3deedd1`: reviewed PR69 native/drop attribution and partial A09 replay guard.
- `1125031`: receiver teardown, queued save/load discard and sender IO failure notification.
- CI/evidence is a separate subsequent commit on this same branch; see `git log` for its identifier. No push, merge or release was performed in this session.

## Next Windows/two-player checks

1. Build Harness and Release with genuine v100 x64, then run prototest and tunneltest.
2. Empty the last stack from a worn bag in both directions; verify total units and parent identity across save/reload.
3. Repeat at exactly 64 entries, overflow, different nested sections, unloaded bags and duplicate snapshots.
4. Test cross-player drag with nested source, identical templates, delayed ACK and rejected destination.
5. Visit a tavern after a long walk and after reload; compare ownership and `[wi]` attribution before picking anything up.
6. Transfer saves over 3/8/16 MB with both host/client logs, elapsed progress, IO error codes and ENet backlog; test disconnect/reconnect. Confirm previous save remains byte-intact on failure.
