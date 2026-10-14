# Project Status — Shadows of Transoxiana

_Last updated: 2026-10-14. Read this first before starting any new milestone._

## Current milestone: **M1 — Playable prototype ("The Sealed Dispatch")**
**State: code written, NOT yet compiled or played.** Waiting for the first local test on
the owner's PC (`docs/NEXT_STEPS.md`).

---

## Web prototype (added 2026-10-14) — **playable and tested**
Because Unreal needs a strong PC/Mac and the owner is on a MacBook Air, the M1 mission was
also built as a **browser game** (`web/`, Three.js r160, no build step).
- Same mission and systems: movement/sprint/crouch/jump/dodge, stamina, hiding zones,
  noise, guard AI (Patrol → Suspicious → Combat → Search, shared alerts, A* path-finding),
  sabre combo/block/parry/takedown, bow with draw power and gravity, tower archers,
  objective → escape → end screen with rank, death/restart, pause + settings (saved),
  synthesised wind and sound effects, keyboard/mouse, gamepad and **touch controls**.
- **Verified** in headless Chromium (software WebGL): no console errors; 11 scripted
  gameplay checks pass (`web/tests/gameplay.test.mjs`): detection in the open, hidden in
  hay, takedown, sabre kill, parry stagger, bow kill, 51 m path around walls, dispatch
  pickup → extraction success, death → failed, wall collision, entry through the breach.
  Title, gameplay, aiming, end-screen and phone-layout screenshots checked visually.
- Not yet verified: real-GPU frame rate, real touch devices, Safari specifically.
- Hosting: `.github/workflows/pages.yml` deploys `web/` to GitHub Pages once Pages is set
  to "GitHub Actions" in the repo settings.
- Known web limitations: primitive characters with procedural animation; no navmesh-level
  smoothing (guards may hug corners); infinite arrows.

The Unreal project remains the long-term target (realistic visuals, horses, battles).

---

## 1. Environment assessment (2026-10-14)

The agent works on a **cloud Linux machine**, not on your PC.

| Item | Finding | Impact |
|---|---|---|
| OS | Ubuntu 24.04 (x86_64), headless | No desktop, no editor window |
| CPU / RAM | 4 cores / 15 GB RAM | Far below what Unreal Editor needs |
| GPU | **None** | Unreal / Unity editors cannot run |
| Game engine | **None installed** | Cannot compile or play the game here |
| Tools present | git, git-lfs 3.4, clang, gcc, Python 3, Node 22 | Can write and lint text files only |

**Consequence:** every gameplay feature below is *implemented in code but untested*.
Nothing is reported as working until it has been compiled and played in Unreal.

---

## 2. Done

| Item | Where | Verified? |
|---|---|---|
| Research (history, references, assets, engine) | `docs/*.md` | Sources gathered from search results; museum/UNESCO links still need click-through checking |
| Game design (concept, timeline, protagonist, first mission, roadmap, risks, fact-check process) | `docs/GAME_DESIGN.md` | — |
| Visual direction with historical references | `docs/VISUAL_DIRECTION.md` | — |
| Asset manifest (sources, licences, import plan) | `docs/ASSET_MANIFEST.md` | Licences need re-reading on each page before import |
| Engine recommendation (UE 5.8), hardware, architecture | `docs/TECHNICAL_ARCHITECTURE.md` | — |
| Beginner install & test guide | `docs/NEXT_STEPS.md` | — |
| UE 5.8 C++ project skeleton (uproject, Config, targets, Git LFS, .gitignore) | repo root | **Not opened in Unreal yet** |
| M1 gameplay code (~4,400 lines C++) | `Source/Transoxiana/` | **Not compiled.** Statically reviewed twice (author + independent API review) |

### M1 features implemented (all untested)
- Third-person player: walk/run, sprint + stamina, crouch-sneak, jump, dodge.
- Sabre: 2-hit combo, block, **parry window** → enemy stagger, silent takedown from behind.
- Composite bow: hold-to-aim camera, draw power, gravity drop, headshots; arrows make noise (distraction).
- Stealth: hiding zones (hay, shade, awnings), visibility by stance/speed, noise system.
- Guard AI: Patrol → Suspicious → Combat → Search state machine; vision cone + line of sight; awareness meter; hearing; shared alerts; max 2 attackers; swordsman and tower archer types; NavMesh with direct-steering fallback.
- Procedural rabat outpost (walls, towers, gate, breach, domed hall, rooms, shed, tents, hay, poplars), dusk lighting if none exists.
- Mission: steal the dispatch → escape to the falconer → end screen with stats and rank; death → restart.
- HUD: health/stamina, weapon help, objective + marker, guard awareness icons, crosshair, prompts, briefing.
- Pause menu (resume, settings, restart, quit) and settings (sensitivity, invert Y, ambient volume, graphics quality — saved).
- Procedural ambient wind audio (no sound files).
- Automatic switch to the UE mannequin when the Third Person pack is added.

---

## 3. Known issues and limitations

| # | Issue | Severity | Plan |
|---|---|---|---|
| 1 | Code has never been compiled; compile errors are possible | High | First local build (NEXT_STEPS Part 4/9) |
| 2 | No attack/bow/crouch animations: sabre swings procedurally beside the body, not in the hand | Medium | M2 |
| 3 | Characters are placeholders until the Third Person pack is added; enemies use the same mannequin + cone helmet | Medium | M2/M3 |
| 4 | Pause/settings menus are keyboard/gamepad only (no mouse clicking) | Low | UMG menus in M4 |
| 5 | Without a Nav Mesh Bounds Volume guards steer directly and can snag on walls | Medium | NEXT_STEPS Part 5 step 3 |
| 6 | Look-up/down direction may feel inverted depending on engine input-scale settings | Low | "Invert look Y" setting |
| 7 | Infinite arrows; no healing; no body discovery by guards | Low | M3 |
| 8 | No horse (milestone M5, as planned) | — | M5 |
| 9 | All combat/detection numbers are first guesses | Medium | Tune after playtest |
| 10 | Possible MSVC double→float warnings (not errors) | Low | Clean up after first build |
| 11 | An Outpost Builder placed in a level builds its geometry in `OnConstruction`; packaged (cooked) builds rely on the saved instances — fine for PIE, check before packaging | Low | Verify in M4 packaging test |
| 12 | Repository is **public** — Epic/Fab/Mixamo files must stay out (`.gitignore` handles the Third Person pack folders) | Legal | Make repo private before sharing licensed assets |

---

## 4. Next actions
1. **Owner:** follow `docs/NEXT_STEPS.md` Parts 0–8 and report results (Part 9).
2. **Agent (next session):** fix compile/runtime errors from the report, then tune M1.
3. Then start M2 (animation) per `docs/GAME_DESIGN.md` §7.
