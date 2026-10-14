# Technical Architecture — Shadows of Transoxiana

_Last updated: 2026-10-14. Status of every item below is tracked in `PROJECT_STATUS.md`._

---

## 1. Engine choice

### 1.1 What was evaluated

| Criterion (from the brief) | Unreal Engine 5.8 | Unity 6 | Browser (Three.js) |
|---|---|---|---|
| Realistic third-person look (sunlit stone, dust, torchlight) | **Best.** Lumen real-time GI, Nanite geometry, Virtual Shadow Maps, volumetric fog/clouds out of the box | Good with HDRP, but HDRP needs more manual setup | Weak. No real-time GI, limited shadows, browser memory limits |
| Third-person character + animation | **Best.** Free Third Person template, free mannequins, free *Game Animation Sample* (motion matching), retargeting built in | Good (Starter Assets), animation tooling less complete | Must build almost everything by hand |
| Large outdoor world, horses, crowds | **Best.** World Partition, PCG framework, Mass AI crowds, landscape tools | Possible, but more third-party tools needed | Not realistic at this scale |
| Stealth/enemy AI | Built-in Behavior Trees, State Tree, AI Perception, NavMesh | Built-in NavMesh; behaviour trees need a package | Write from scratch |
| Free realistic content | Fab (free monthly assets), MetaHuman, Megascans (some free), Mixamo works | Asset Store (much is paid), Mixamo works | Mostly CC0 models only |
| Cost | Free until US$1M lifetime gross per title, then 5% royalty | Personal free under US$200k/yr revenue | Free |
| Beginner friendliness | Steep, but **Blueprints** let you build without C++; huge tutorial base | Moderate (C#) | Easy to code, hard to make it look AAA |
| Can it run on this cloud machine? | No | No | Yes — **but that is not a reason to pick it** (brief §A) |

### 1.2 Recommendation: **Unreal Engine 5.8** (C++ core + Blueprints for content)

**Why:** your target is a *realistic* Assassin's-Creed-style game with horses, stealth,
sword combat and large battles. Unreal's lighting (Lumen), geometry (Nanite), open-world
streaming (World Partition), procedural placement (PCG), animation (motion matching via the
free Game Animation Sample) and AI toolset are the closest match available for free.
Most AAA third-person action games in this genre are built on engines with this feature
set. Unity could work but needs more assembly; a browser engine cannot reach the visual
target.

**Version policy:** UE 5.8 is the current stable release (June 2026). Epic has announced
Unreal Engine 6, but it is not released. **Stay on 5.8 for the whole prototype and
vertical slice.** Do not upgrade the engine in the middle of a milestone — upgrade only
after committing, on a separate branch, and only if a feature you need requires it.

**Why C++ and not Blueprint-only?** Blueprints are binary `.uasset` files that can only be
made inside the editor. Because the AI agent works without the editor, it writes gameplay
systems in C++ (plain text it can create and review). You — the designer — will then
use Blueprints and the editor for content: placing actors, tuning numbers, swapping
meshes, adding animations. This is the same split most professional Unreal teams use.

---

## 2. What you need on your PC

### 2.1 Hardware

| | Minimum (it will run, slowly) | **Recommended (target this)** | Comfortable |
|---|---|---|---|
| OS | Windows 10 64-bit (22H2) | **Windows 11 64-bit** | Windows 11 64-bit |
| CPU | 4 cores, 2.5 GHz+ | **8 cores** (e.g. Ryzen 7 5700X / Core i5-13600K or newer) | 12–16 cores (Ryzen 9 / Core i7–i9) |
| RAM | 16 GB | **32 GB** (Epic's own recommendation) | 64 GB |
| GPU | DirectX 12 card with 6 GB VRAM, Shader Model 6 (GeForce RTX 2060 / Radeon RX 6600) | **8–12 GB VRAM** (GeForce RTX 3060 12 GB / RTX 4060 Ti / Radeon RX 7600 XT or better) | RTX 4070 / 4080 class, 12–16 GB |
| Storage | 150 GB free **SSD** | **250 GB+ free NVMe SSD** | 1 TB NVMe SSD dedicated to projects |
| Display/input | 1080p monitor, mouse + keyboard | + Xbox/PlayStation controller (optional) | |

Notes for a beginner:
- **The GPU matters most.** Lumen and Nanite need a DirectX 12, Shader Model 6 card.
  On older or integrated graphics (e.g. Intel UHD) the editor may open but the game will
  be very slow; use *Settings → Engine Scalability → Low* as a fallback.
- **Use an SSD.** Unreal on a hard disk is painfully slow (shader compile, loading).
- **RAM:** 16 GB works for this small prototype; 32 GB avoids crashes when compiling
  C++ while the editor is open and when the world grows.
- Space breakdown (approx.): engine ~60–80 GB, Visual Studio ~20–40 GB, project
  + derived data cache ~20–60 GB as it grows.
- Laptops: fine if they have a dedicated RTX/Radeon GPU and stay plugged in.

### 2.1b Mac (Apple Silicon)
UE 5.8 supports macOS with Apple Silicon (Epic's requirements: macOS Sonoma 14.5 minimum,
Sequoia recommended; Xcode 26.0 minimum, 26.1.1 recommended, 26.4 incompatible).
Lumen software ray tracing works on M1+; **Nanite and Virtual Shadow Maps need M2+**
(beta on Mac). Recommended: M2 Pro / M3 / M4 or newer with **32 GB** unified memory;
16 GB is the practical minimum; Intel Macs are not recommended. On Mac, **Xcode replaces
Visual Studio**. Source: [Epic — macOS development requirements](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine).
Note: the final game is aimed at Windows PC players; developing on a Mac is fine, but test
Windows builds before release.

### 2.2 Software (all free)

| Software | Why | Version |
|---|---|---|
| Epic Games Launcher | Installs Unreal Engine | latest |
| **Unreal Engine 5.8** | The engine | 5.8.x (latest hotfix) |
| **Visual Studio 2026 Community** (preferred) or VS 2022 Community ≥ 17.14 | Compiles the C++ code | Workloads: *Game development with C++*, *Desktop development with C++*, *.NET desktop development*; component: Windows 10/11 SDK; *Unreal Engine installer* |
| Git for Windows + Git LFS | Version control; LFS stores large binary assets | latest |
| GitHub Desktop (optional) | Easier Git for beginners | latest |
| Blender (optional, later) | Editing/creating 3D models | 4.x |

Exact click-by-click steps are in `NEXT_STEPS.md`.

---

## 3. Workflow

```
 AI agent (cloud)                         You (Windows PC)
 ─────────────────                        ───────────────────────────────
 writes C++ + config + docs  ──git push──▶ git pull
                                          double-click Transoxiana.uproject
                                          (Unreal compiles the C++ module)
                                          import assets, build levels, Play
 reads your bug report       ◀─git push── commit levels/assets (Git LFS),
 fixes code                               update PROJECT_STATUS.md / issues
```

Rules:
1. **One milestone at a time.** Each milestone ends with: compiles, runs in Play-In-Editor
   (PIE), the checklist in `NEXT_STEPS.md` passes, and `PROJECT_STATUS.md` is updated.
2. **Binary assets go through Git LFS** (`.gitattributes` is set up). Never commit
   `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` (ignored by `.gitignore`).
3. **Only one person edits a given `.umap`/`.uasset` at a time** — binary files cannot
   be merged.
4. **Third-party assets from Fab must not be pushed to a public repository** (Fab
   licence forbids redistributing source assets). Keep the repo private, or keep those
   assets out of Git (see `ASSET_MANIFEST.md`).

---

## 4. Project structure

```
adbdgame/
├─ Transoxiana.uproject          # double-click to open (Unreal 5.8)
├─ Config/                       # engine, game, input, rendering settings
├─ Source/
│  ├─ Transoxiana.Target.cs      # game build target
│  ├─ TransoxianaEditor.Target.cs
│  └─ Transoxiana/               # the C++ gameplay module
│     ├─ Core/        GameMode, PlayerController (settings, pause)
│     ├─ Player/      Third-person character, combat, stealth
│     ├─ Combat/      Arrow projectile, damage types
│     ├─ AI/          Enemy character + AI controller (patrol/detect/alert/search/combat)
│     ├─ World/       Procedural outpost builder, stealth zones, ambient wind synth
│     ├─ Mission/     Dispatch pickup, extraction zone, mission state
│     └─ UI/          HUD (health, stamina, objective, detection, pause & settings menus)
├─ Content/                      # created by the editor (maps, imported assets) — Git LFS
└─ docs/                         # design and status documents
```

---

## 5. Gameplay systems (prototype, milestone M1)

The prototype is deliberately **asset-independent**: it runs with only engine-provided
primitives, then automatically upgrades to the free Unreal mannequin when you add the
*Third Person* content pack. This lets the gameplay be tested before any art exists.

| System | Class | Approach |
|---|---|---|
| Game rules & mission | `ATransoxianaGameMode` | Mission state machine: *Infiltrate → Steal the dispatch → Escape → Success / Failed*. Spawns the outpost if the level does not contain one. Collects stats (detections, takedowns). |
| Settings & pause | `ATransoxianaPlayerController` | Pause menu and settings (mouse sensitivity, invert Y, ambient volume, graphics quality) driven by keyboard/gamepad; saved to `GameUserSettings.ini`. |
| Input | Enhanced Input, created **in C++ at runtime** | No input assets needed. Keyboard/mouse and gamepad mapped. |
| Player | `ATransoxianaCharacter` | Spring-arm third-person camera, walk/sprint/crouch-sneak/jump/dodge, stamina, health. Sword: light attack, block, **parry window**, stealth takedown from behind. Bow: hold to aim (over-the-shoulder), draw, release arrow. Emits *noise* when sprinting/landing. |
| Arrows | `AArrowProjectile` | `UProjectileMovementComponent` with gravity; damages pawns; impact makes noise (used to **distract** guards). |
| Enemies | `AEnemyCharacter` + `AEnemyAIController` | Hand-written state machine (easy to read and debug): **Patrol → Suspicious → Alert/Combat → Search → Patrol**. Vision cone + line-of-sight trace; detection meter fills faster when the player is close, standing, moving, or outside hiding zones. Hears noise events. Swordsman (telegraphed attack, can be parried) and Archer (ranged) archetypes. Uses NavMesh when present, falls back to direct steering. |
| Stealth | `AStealthZone` | Volumes representing deep shade, hay, cloth awnings: crouching inside cuts visibility to 25 %. |
| Level | `AOutpostBuilder` | Procedurally builds a small walled **rabat (fortified caravan station)** from engine primitives in instanced meshes: mud-brick walls, corner towers, gate, courtyard rooms, a domed hall with turquoise dome, tents, stealth zones, patrol routes, the objective and the exit. Also adds sun, sky, fog and sky light if the level has none. Visible in the editor (`OnConstruction`). |
| Ambient audio | `UWindSynthComponent` | **Procedural** gusting steppe wind generated in code (no audio files), so the prototype has ambience before sound assets are imported. |
| HUD | `ATransoxianaHUD` | Canvas-drawn: health/stamina bars, weapon, objective text, per-enemy awareness indicators, crosshair when aiming, mission end screen, pause/settings menu. |

### 5.1 Why a hand-written AI state machine instead of Behavior Trees?
Behavior Trees and State Trees are editor assets (binary). A C++ state machine is
readable, testable and version-controllable as text. When the vertical slice needs more
complex behaviour, migrate the logic to **State Tree** assets inside the editor
(milestone M3), keeping the same perception code.

### 5.2 Performance budgets (vertical slice, 1080p, recommended PC)
- 60 FPS target in the outpost; 16.6 ms frame.
- ≤ 20 active AI characters; AI tick ≤ 1 ms total.
- Primitives are instanced (`UInstancedStaticMeshComponent`) to keep draw calls low.
- Check with `stat fps`, `stat unit`, `stat gpu` (console, key `` ` ``).

---

## 6. Later architecture (planned, not built)

| Milestone | Addition |
|---|---|
| M2 | Replace placeholder animation with Game Animation Sample / Mixamo retargeted sword & bow montages; anim notifies drive hit windows instead of timers. |
| M3 | State Tree AI, AI Perception component, body discovery, group alerts; Gameplay Ability System if combat grows. |
| M4 | Horse: separate `AHorse` pawn with mount/dismount, mounted bow; Chaos Vehicles are **not** used (animals need animation-driven locomotion). |
| M5 | World Partition landscape, PCG for orchards/fields/villages, data-driven quests (Data Assets), save system, localisation (English/Uzbek, String Tables). |
