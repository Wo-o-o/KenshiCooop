# KenshiCoop v0.51 stabilization — living report

Development branch: `dev/stabilize-v0.51`. No changes are merged into main or released.

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
| [PR69](https://github.com/nhoral/KenshiCoop/pull/69) | Open: ownership filter plus scan-only native baseline | Candidate for narrow reviewed port, credited to author |
| [PR70](https://github.com/nhoral/KenshiCoop/pull/70) | Open: native pickup notice, protocol 56 | Not included automatically: changes protocol; native pickup remains tracked |
| [PR80](https://github.com/nhoral/KenshiCoop/pull/80) | Draft/open: inventory save fence, protocol 60; fork comments describe a refined variant | Save freshness is separate from byte-transfer integrity; defer wire change pending review |
| PR50 | Open: three-player support | Outside two-player stabilization scope |

Community evidence: [#63](https://github.com/nhoral/KenshiCoop/issues/63) and [#66](https://github.com/nhoral/KenshiCoop/issues/66) describe stolen/native plus free-copy town items; [#44](https://github.com/nhoral/KenshiCoop/issues/44) describes repeated native pickup. [#61](https://github.com/nhoral/KenshiCoop/issues/61), [#77](https://github.com/nhoral/KenshiCoop/issues/77), [#56](https://github.com/nhoral/KenshiCoop/issues/56), [#57](https://github.com/nhoral/KenshiCoop/issues/57) describe backpacks and transfer loss. [#72](https://github.com/nhoral/KenshiCoop/issues/72)/[#82](https://github.com/nhoral/KenshiCoop/issues/82) describe long-session desync. [#74](https://github.com/nhoral/KenshiCoop/issues/74) distinguishes missing squad/job metadata at serialization from clean file transfer and reports crashes around an hour. [#55](https://github.com/nhoral/KenshiCoop/issues/55) includes a non-ASCII save-root hypothesis. These are reports, not reproduced gameplay here.

## Fix status

| ID | State | Evidence / remaining work |
| --- | --- | --- |
| A02 | Implemented, Linux control-flow verified | Continue enumeration past capacity to mark actual omitted children; unavailable section reads are incomplete |
| A01 | Implemented with safety limits | Clear uniquely resolved empty bag only from complete valid snapshots; incomplete local enumeration or ambiguous same-template bags cannot authorize clearing |
| A03 | Pending | Remap parentIdx after latch adjustments; synthetic bag entries must not authorize empty clearing |
| A06/A07 | Pending | Nested transfer detection/relocation and ACK/snapshot causal ordering |
| A05/A09 | Pending | Deferred drops, consumed world ids and claim retry |
| A11/A04/A12 | Pending | Distinguish read/apply outcomes, translate proxy hands, fence old world data |
| A13/A14/A15 | Pending | Validate on-disk transfer, paths, ordering and receiver lifecycle |
| A08/A10 | Pending | Item attributes and world-proxy quantity/quality |

## Tests

`python3 scripts/tests/portable_regressions.py --sanitize` builds extracted **current production function bodies** with minimal engine mocks, C++03, AddressSanitizer and UBSan. SEH is adapted only in generated test files; this is not a DLL or Win32 ABI test.

- A02 red baseline: 7/13 passing; failed top/section/parent boundaries, destructive incomplete reconcile, unavailable reads. After fix: 13/13.
- A01 red baseline after A02: 16/19 passing; empty nested inventories remained stale. Initial fix passed 19/19.
- Added safety regressions found another failure with incomplete local parent enumeration. Final inventory checks: **22/22 PASS**, sanitizers clean.
- Cases include real 64-entry capture limits, exact-full complete output, top-only reader semantics, read fault, incomplete desired/local snapshots, idempotent empty replay, two distinct bags, ambiguous same-template bags and invalid parent graph.
- Initial audit's compatible prototest subset: 456/456; PowerShell contract fixtures: 32/32. These must be rerun after the final session changes; old results do not validate new code.

Windows v100 DLL/prototest and two-player Kenshi/Steam tests have **not run**.

## Gameplay investigations

### Tavern duplicates

Strong source explanation corroborated by PR69: the first-scan baseline covers only the first loaded area. Shop items encountered later are treated as new drops and streamed; `spawnWorldItemProxy` creates them with owner=0. The receiver already has the owned native, so sees a native red/stolen item plus a free grey proxy. Late interior generation can also defeat a time-only baseline. This differs from both engines generating different loot and from duplicate pickup of a shared native. No live trace from the players is available to prove which path occurred in their session.

### Transfer stalls around 3 MB

No fixed 3 MB limit has been established. Sender queues 32 x 4 KiB per 50 ms, while actual ENet/Steam delivery can be slower. Sender completion means queued bytes, not acknowledged reception; progress/errors/abort/receiver timeouts need further investigation. Win32 ANSI paths and IO failure are additional candidates. Steam Cloud is unverified: this transfer uses ENet over Steam Networking, not Steam RemoteStorage; file locks could indirectly interfere but need IO error evidence. Do not disable Steam Cloud or attribute the failure to it without that evidence.

## Next Windows/two-player checks

1. Build Harness and Release with genuine v100 x64, then run prototest and tunneltest.
2. Empty the last stack from a worn bag in both directions; verify total units and parent identity across save/reload.
3. Repeat at exactly 64 entries, overflow, different nested sections, unloaded bags and duplicate snapshots.
4. Test cross-player drag with nested source, identical templates, delayed ACK and rejected destination.
5. Visit a tavern after a long walk and after reload; compare ownership and `[wi]` attribution before picking anything up.
6. Transfer saves over 3/8/16 MB with both host/client logs, elapsed progress, IO error codes and ENet backlog; test disconnect/reconnect. Confirm previous save remains byte-intact on failure.
