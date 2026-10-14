# Project Status — Shadows of Transoxiana

_Last updated: 2026-10-14_

## 1. Environment assessment (FIRST ACTION)

This work is being done by an AI agent on a **cloud Linux machine**, not on your PC.

| Item | Finding | Impact |
|---|---|---|
| OS | Ubuntu 24.04 (x86_64), headless | No desktop, no editor window |
| CPU / RAM | 4 cores / 15 GB RAM | Far below what Unreal Editor needs |
| GPU | **None** (no `/dev/dri`, no `nvidia-smi`) | Unreal / Unity editors cannot run |
| Game engine | **None installed** (no Unreal, Unity or Godot) | Cannot compile or play the game here |
| Tools present | git, git-lfs 3.4, clang, gcc, Python 3, Node 22 | Can write and lint text files only |
| Repository | Only `Prompt.md` (the master brief) | Starting from zero |

**Conclusion:** in this environment I can research, write the design documents, and
create the Unreal Engine project *source files* (C++ and config). I **cannot** open,
compile, or play the project. Everything gameplay-related must be built and tested on
your own Windows PC by following `docs/NEXT_STEPS.md`.

## 2. Plan for this session

1. Environment assessment (this file).
2. Research + write `docs/GAME_DESIGN.md`, `docs/VISUAL_DIRECTION.md`,
   `docs/ASSET_MANIFEST.md`, `docs/TECHNICAL_ARCHITECTURE.md` (includes engine
   recommendation and hardware), `docs/NEXT_STEPS.md`.
3. Create the Unreal Engine 5 C++ project skeleton with first-prototype gameplay code.
4. Update this status file with what is done, what is untested, and known issues.
