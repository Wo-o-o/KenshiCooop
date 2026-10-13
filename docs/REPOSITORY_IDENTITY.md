# Repository identity

## Display name

KenshiCoop — AI-Assisted Stabilization

This is the README display title. The repository slug remains `Wo-o-o/KenshiCooop`;
no repository rename is part of this documentation change.

## GitHub About description

AI-assisted stabilization of KenshiCoop. Improving multiplayer synchronization, inventory integrity and save reliability. Based on the original project by nhoral.

## Scope and attribution

This is an experimental fork of [nhoral/KenshiCoop](https://github.com/nhoral/KenshiCoop),
based on v0.51. AI-assisted work aims to prevent duplication, inventory loss,
desynchronization and save transfer failures. It does not imply upstream
endorsement, complete fixes or readiness for public gameplay.

The [progress report](STABILIZATION_PROGRESS.md) records passing automated
checks and unresolved defects. The modified DLL has not yet been built with the
required v100 toolchain or validated in real gameplay. RE_Kenshi and KenshiLib
remain required; see [integration requirements](RE_KENSHI_INTEGRATION.md).

The original [AGPL-3.0 license](../LICENSE), attribution and upstream installation
guidance are retained. See the [README credits](../README.md#credits) for original
authors and community contributions. This presentation change does not relicense
the project or replace any existing notices.

## Updating About independently of branches

A repository maintainer can open `Wo-o-o/KenshiCooop` on GitHub, use the gear next
to **About**, paste the description above into **Description**, and save it.
This changes repository metadata, not a tracked file: it creates no commit and
does not modify `main`, merge this branch, push local commits or publish a release.
The description applies to the repository as a whole, not only this branch.

GitHub also exposes `description` in its
[Update a repository API](https://docs.github.com/en/rest/repos/repos#update-a-repository).
The metadata update is separate from publishing this branch's README, which
remains a versioned file. No remote metadata update is performed by this commit.
