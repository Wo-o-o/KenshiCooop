# Portable regression checks

Run from the repository root:

```bash
python3 scripts/tests/portable_regressions.py --sanitize --existing-prototest
python3 scripts/tests/check_re_integration.py
pwsh -NoProfile -File scripts/tests/Contract.Tests.ps1
```

Requirements: Python 3, GCC with C++03/ASan/UBSan, PowerShell for existing harness contracts. Generated files/executables live in ignored `build/portable-regressions`.

The runner extracts current production functions, rather than maintaining implementation copies. Engine/filesystem/network surfaces are test-only mocks. Windows SEH becomes ordinary C++ exceptions, `__int64` becomes `long long`, and MSVCRT `%I64u` logging becomes `%llu` **only in generated Linux tests**. Windows `long`, calling conventions, game layouts, hook trampolines and real file IO are not tested by these adaptations.

- Inventory: actual capture, reconciliation, latch adjustment and ACK handling; quantity and parent integrity, completeness, replay/delayed processing, refusal-safe cases.
- Snapshot policy: actual shared C++03 parent-index compaction; orphan/cycle/overflow rejection.
- World items: actual publisher and receiver; late native loading, genuine drop events, echo filtering, repeated and post-pickup delayed snapshots. Engine ownership resolution and genuine drop-hook coverage still require the game.
- Save abort: actual sender failure marker and receiver DONE/teardown; partial 3-MB transfer, stale/replayed markers and preservation of the prior save. Does not test disk integrity, Unicode Windows paths or Steam transport.
- Existing prototest: current pure tests plus actual `Inbound.h` queue lifecycle. A Linux-only `windows.h` shim supplies single-threaded critical-section stubs; it cannot validate locking/races. Win32 SaveXfer disk integration is excluded. A passing subset is not the full v100 test suite.
- RE_Kenshi checks: source-level loader/ABI/toolchain/hook/thread contracts only. Required combined runtime validation is in `docs/RE_KENSHI_INTEGRATION.md`.

`.github/workflows/stabilization.yml` runs compatible checks on Ubuntu. The Windows job is manual and requires a trusted, dedicated self-hosted `kenshi-v100` runner with the already approved dependency bundle. It checks compiler version 16, builds with genuine v100, runs prototest/tunneltest/snapshot policy tests, and inspects PE imports/exports. It does not download, update, deploy or redistribute RE_Kenshi or run gameplay automatically. Keep game/save data outside the CI checkout.
