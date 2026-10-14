# Next Steps — install Unreal and test the game on your PC

_Last updated: 2026-10-14. Written for a complete beginner. Follow the parts in order._

The AI agent wrote the Unreal project files on a cloud machine without Unreal Engine or a GPU,
so **the Unreal version has never been compiled or played yet** (the web version below has been tested). Your PC is the first place it will
run. Expect possibly a few compile errors on the first try — that is normal; Part 9 explains
how to report them so they can be fixed quickly.

Time needed: about 2–4 hours the first time (mostly downloads).

---


## 🌐 Web version — the quickest way to play (no Unreal needed)

A browser version of the same mission lives in `web/`. It runs on your MacBook Air,
any PC, and phones. It looks simpler than the future Unreal version, but the gameplay is
the same (stealth, guards, sabre, bow, mission). It has been tested in a headless
browser: all scripted gameplay checks pass.

### Play it on your Mac (2 minutes)
1. In GitHub Desktop, **Fetch / Pull** so you have the `web` folder.
2. Open **Terminal** and go to the project folder (yours is called `game`):
   ```bash
   cd ~/path/to/game        # the folder that contains Transoxiana.uproject and web/
   cd web
   python3 -m http.server 8000
   ```
3. Open **http://localhost:8000** in Chrome or Safari → **Play**.
4. Click the game to capture the mouse. **P** pauses. Controls are listed on the title
   screen and in `web/README.md`.
5. When finished, press **Ctrl+C** in Terminal.

### Put it online for free (playable on phones) — GitHub Pages
1. Make sure the web version is on `main` (merge the pull request on GitHub).
2. GitHub → your repo → **Settings → Pages** → under *Build and deployment*,
   **Source: GitHub Actions**.
3. GitHub → **Actions** tab → **Deploy web prototype to GitHub Pages** → **Run workflow**
   (it also runs automatically whenever `web/` changes on `main`).
4. After about a minute the link appears in the workflow run, usually
   **https://shavkatovabdulfattox.github.io/adbdgame/** — open it on any device.
   On a phone, hold it sideways; left side = move, right side = look.

---

## 🍎 Using a Mac instead of Windows — read this first

Unreal Engine 5.8 runs on Macs with **Apple Silicon (M1, M2, M3, M4 …)**. Parts 0–4
below are written for Windows; on a Mac do **M0–M4 here instead**, then continue with
Part 5 (it's the same on both).

### M0 — Check your Mac
Apple menu  → **About This Mac**.
| | Works | Recommended |
|---|---|---|
| Chip | Apple **M1** (Lumen lighting works) | **M2 Pro / M3 / M4 or newer** (Nanite + Virtual Shadow Maps need **M2+**) |
| Memory | 16 GB | 32 GB or more |
| macOS | Sonoma 14.5 | latest Sequoia 15 / macOS 26 |
| Free disk | 150 GB | 250 GB+ |

**Intel Macs:** not recommended — very slow, and Nanite is not supported. A Mac with
8 GB memory will struggle badly.

### M1 — Get the project
1. Install **GitHub Desktop for Mac** (<https://desktop.github.com/>), sign in.
2. **File → Clone Repository** → `ShavkatovAbdulfattox/adbdgame` → local path
   `~/Dev/adbdgame` (no spaces) → Clone. Accept Git LFS if asked.
3. **Current Branch** → `claude/game-engine-research-i8iyyq`.

### M2 — Install Xcode (replaces Visual Studio)
1. Install **Xcode 26.1.x** from the Mac App Store or <https://developer.apple.com/download/>.
   Epic lists Xcode **26.0 minimum, 26.1.1 recommended** for UE 5.8 and warns that
   **26.4 is not compatible** — if the App Store gives you a newer version and the build
   fails, install 26.1.1 from developer.apple.com (“More downloads”).
2. Open Xcode once and accept the licence; let it install extra components.
3. Open **Terminal** and run: `xcode-select --install` (installs command-line tools; if it
   says they're already installed, that's fine).

### M3 — Install Unreal Engine 5.8
Same as Part 3: Epic Games Launcher (Mac version) → Unreal Engine → Library → **+** →
**5.8** → Install (untick iOS/Android/Linux in Options). It installs to
`/Users/Shared/Epic Games/UE_5.8`.

### M4 — Build and open the project
The most reliable way on a Mac is to build once from Terminal (you'll see any errors
clearly). Copy-paste these two lines into **Terminal**:

```bash
cd ~/Dev/adbdgame
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" TransoxianaEditor Mac Development -project="$PWD/Transoxiana.uproject"
```

- It ends with **`Result: Succeeded`** → double-click `Transoxiana.uproject` (or open it
  from the Epic Launcher → Library → My Projects → Browse). First launch compiles shaders
  for a while.
- It ends with errors → copy everything from the first `error:` line and send it to the
  agent (Part 9).

### Mac controls and tips
- **Use a real mouse.** On a trackpad, right-click = two-finger click, and holding it
  (block/aim) is awkward.
- **Option (⌥)** is the "Alt" key (dodge). **Control** or **C** = crouch.
- The "**`**" console key is the key left of **1** (on some layouts **§** or **±**).
- Screenshots: **⌘ + Shift + 4**.
- If the game is slow: pause (**P**) → Settings → Graphics quality **Low**, and keep the
  Mac plugged in.

---

## Part 0 — Check your PC (5 min)

1. Press **Win + R**, type `dxdiag`, press Enter.
   - *System* tab: note **Operating System** (need Windows 10/11 64-bit) and **Memory**
     (16 GB minimum, 32 GB recommended).
   - *Display* tab: note the graphics card **Name** and **Display Memory (VRAM)**.
     Recommended: NVIDIA RTX 2060 / AMD RX 6600 or newer, **6 GB VRAM or more**.
2. Open **File Explorer → This PC** and check you have **at least 150 GB free on an SSD**.
3. Compare with the table in `docs/TECHNICAL_ARCHITECTURE.md` §2.1. If your graphics card
   is older or integrated (e.g. "Intel UHD"), you can still try, but set the graphics
   quality to Low in the game's settings menu.

---

## Part 1 — Install Git and get the project (15 min)

The easiest way for a beginner is **GitHub Desktop** (it includes Git LFS, which stores
large game files).

1. Download and install **GitHub Desktop** from <https://desktop.github.com/>. Sign in with
   your GitHub account (the one that owns `ShavkatovAbdulfattox/adbdgame`).
2. In GitHub Desktop: **File → Clone repository → GitHub.com** tab → select
   `ShavkatovAbdulfattox/adbdgame`.
3. **Local path:** choose a short path on your SSD with no spaces or Cyrillic letters, for
   example `C:\Dev\adbdgame`. (Long paths cause Unreal build errors.) Click **Clone**.
4. At the top, click **Current branch** and choose
   **`claude/game-engine-research-i8iyyq`** (this branch has all the new files; later it
   will be merged into `main`).
5. If GitHub Desktop asks to **initialize Git LFS**, click **Yes**.

You should now see `Transoxiana.uproject`, `Config`, `Source` and `docs` in that folder.

> **Your repository is public.** Anything you push can be seen by everyone. Epic's
> mannequin, Fab and Mixamo files may **not** be published publicly, so `.gitignore`
> already keeps those folders out. If you later want to share those assets with
> teammates, make the repository private first (GitHub → repo → Settings → Danger Zone →
> Change visibility).

---

## Part 2 — Install Visual Studio 2026 Community (30–60 min, free)

Unreal needs Visual Studio to compile the game's C++ code. You never have to write C++
yourself.

1. Go to <https://visualstudio.microsoft.com/downloads/> and download **Visual Studio
   Community** (2026). (Visual Studio 2022 version 17.14 or newer also works with UE 5.8.)
2. Run the installer. On the **Workloads** tab tick:
   - **Game development with C++**
   - **Desktop development with C++**
   - **.NET desktop development**
3. On the right, under *Game development with C++ → Optional*, make sure these are ticked:
   - **Unreal Engine installer** (and *IDE support for Unreal Engine* if listed)
   - a **Windows 11 SDK** (or Windows 10 SDK 10.0.22621 or newer)
   - **C++ profiling tools**
4. Click **Install**. Restart the PC when it finishes.

---

## Part 3 — Install Unreal Engine 5.8 (30–90 min)

1. Download the **Epic Games Launcher** from <https://www.unrealengine.com/download>,
   install it and create/sign in to a free Epic Games account.
2. In the launcher click **Unreal Engine** (left) → **Library** tab → the **+** next to
   *Engine Versions* → choose **5.8.x** (latest 5.8) → **Install**.
3. Before it starts, click **Options** and **untick** target platforms you don't need
   (Android, iOS, Linux, etc.) to save tens of GB. Keep *Starter Content* and *Templates
   and Feature Packs* ticked.
4. Wait for the download and install to finish.

> Don't install Unreal Engine 6 (not released) or a different 5.x version for this
> project. If you only have 5.7, see Troubleshooting.

---

## Part 4 — Open (and compile) the project (10–30 min)

1. Open `C:\Dev\adbdgame` in File Explorer.
2. **Right-click `Transoxiana.uproject` → Show more options → Generate Visual Studio
   project files.** Wait until it finishes (a window flashes and closes). A
   `Transoxiana.sln` file appears.
3. **Double-click `Transoxiana.uproject`.**
4. A message says *"The following modules are missing or built with a different engine
   version: Transoxiana. Would you like to rebuild them now?"* → click **Yes**.
5. Wait. The first compile and shader compilation can take 5–30 minutes. The editor then
   opens.
   - **If you see "Transoxiana could not be compiled. Try rebuilding from source
     manually."** → go to **Part 9 (report a compile error)**.

---

## Part 5 — Create the level (5 min, one time only)

The game builds the outpost automatically, so the level only needs to exist.

1. In the editor menu: **File → New Level → Empty Level** (the one *without* "Open
   World") → **Create**.
2. **File → Save Current Level As…** → open the **Content → Maps** folder → name it
   exactly **`L_Outpost`** → **Save**.
3. *(Recommended — lets you see the outpost in the editor and makes guards walk around
   obstacles properly)*
   - Open **Window → Place Actors**. Search **Outpost Builder** and drag it into the
     viewport. In the **Details** panel set its **Location** to `0, 0, 0`.
     You should see the grey/sand outpost appear.
   - In *Place Actors* search **Nav Mesh Bounds Volume**, drag it in, set **Location**
     `0, 0, 0` and **Scale** `65, 65, 10`. Press **P** in the viewport: the walkable
     ground turns green (press P again to hide it).
4. **File → Save All** (Ctrl+Shift+S).

The project is already configured so this level opens on start and uses the game's own
game mode.

---

## Part 6 — Play for the first time (5 min)

1. Click the green **Play** ▶ button in the toolbar (or press **Alt+P**).
2. Click inside the game view to capture the mouse. **Shift+F1** frees the mouse;
   **Esc** stops playing (in the editor), so use **P** to open the pause menu.
3. You should see: a short briefing, a sandy ground, the walled outpost at dusk, and the
   player as a simple **placeholder figure** (no character art yet), plus wind sounds.

### Controls
| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / look | W A S D / mouse | Left stick / right stick |
| Sprint (hold) | Left Shift | Left stick click |
| Crouch / sneak (toggle) | C or Left Ctrl | Right stick click |
| Jump | Space | A |
| Dodge | Left Alt | B |
| Attack (sabre) / draw & release (bow) | Left mouse | Right trigger |
| Block (sabre) / aim (bow) — hold | Right mouse | Left trigger |
| Switch sabre ↔ bow | Q or Tab | Y |
| Take item / silent takedown | E or F | X |
| Pause menu | P (Esc in packaged game) | Menu/Start |
| Menus | Arrow keys, Enter, Backspace | D-pad, A, B |

---

## Part 7 — Add the free Unreal mannequin (5 min)

1. In the **Content Browser** (bottom) click **+ Add → Add Feature or Content Pack…**
2. Choose the **Blueprint** tab → **Third Person** → **Add to Project**.
3. Press **Play** again. The player and guards should now be animated mannequins
   (guards keep a steel cone helmet so you can tell them apart).
4. If they are still placeholders: open **Window → Output Log**. In the Content Browser
   find the skeletal mesh (search `SKM_Manny`), right-click → **Copy Reference**, and paste
   it into `Config/DefaultGame.ini` next to `CharacterMeshPath=` (do the same for the
   animation blueprint `ABP_Unarmed` → `AnimClassPath=` and add `_C` at the end). Restart
   the editor.

These template files stay on your PC only (see the public-repository note in Part 1).

---

## Part 8 — Test checklist (20 min)

Play through and tick each line. Write down anything that fails.

| # | Test | Expected result | ✓ |
|---|---|---|---|
| 1 | Walk, look around, jump | Smooth third-person movement; camera follows | |
| 2 | Hold Shift while moving | Faster; gold stamina bar drains, then refills | |
| 3 | Press C | Character sneaks (slower); "Sneaking" shows | |
| 4 | Crouch next to the rocks outside / in hay or under the shed roof | "HIDDEN" shows | |
| 5 | Stand in a guard's view | A yellow **?** meter fills over his head; he stops and comes to look | |
| 6 | Stay seen | Meter turns red **!**; he and nearby guards attack; "Times detected" counts it | |
| 7 | Break line of sight and hide | After ~5 s they search ("? searching"), then return to patrol | |
| 8 | Sneak behind an unaware guard, press E | "Silent takedown"; guard falls | |
| 9 | Press Q, hold right mouse, hold & release left mouse | Camera zooms over the shoulder; crosshair; power bar; arrow flies with drop | |
| 10 | Shoot an arrow into the ground near a guard | He walks to investigate the noise | |
| 11 | Sabre fight: left click | Swing; guard loses health; some hits are "blocked" | |
| 12 | Hold right mouse just as a guard's **"!! ATTACK"** swing lands | "PARRY!"; guard is STAGGERED and takes double damage | |
| 13 | Archers on the towers | Shoot arrows when they see you | |
| 14 | Enter the domed hall, press E at the glowing letter | Objective changes to "Escape to Aybike…" | |
| 15 | Reach the blue figure outside the north-east corner | **MISSION COMPLETE** screen with time, detections, takedowns, rank | |
| 16 | Die on purpose | "BAHRAM HAS FALLEN"; Enter restarts | |
| 17 | Press P → Settings | Change sensitivity, invert Y, volume, graphics quality; Back; Resume | |
| 18 | Press **`** (key left of 1), type `stat fps` | Frame rate shown — note the number | |

Tip: if looking up/down feels inverted, change **Invert look Y** in the settings menu.

---

## Part 9 — Report results and errors back to the agent

1. **Compile error (Part 4 failed):**
   - Open `Transoxiana.sln` (double-click). At the top choose **Development Editor** and
     **Win64**. In *Solution Explorer* right-click **Transoxiana** → **Build**.
   - In the **Output** window, copy everything from the first `error` line downward.
   - Start a new Claude Code session on this repository and paste: *"Read
     docs/PROJECT_STATUS.md. The project fails to compile on UE 5.8 with these errors:
     …"*. (Or open a GitHub issue with the errors.)
2. **Game runs:** send the checklist results (which numbers failed and what you saw),
   your FPS number, and 1–2 screenshots (**F9** in the editor viewport, or Win+Shift+S).
3. **Save your work to GitHub** (GitHub Desktop): you'll see `Content/Maps/L_Outpost.umap`
   as a change. Write a summary like "Add outpost level", click **Commit**, then
   **Push origin**. Do not commit `Binaries`, `Intermediate`, `Saved` (already ignored).

---

## Troubleshooting

| Problem | Fix |
|---|---|
| "Could not be compiled" | Part 9, step 1. Most common cause: a Visual Studio workload missing — re-run the VS Installer (Part 2). |
| "This project was made with a different version of Unreal" / you only have 5.7 | Right-click `Transoxiana.uproject` → **Switch Unreal Engine version…** → pick yours. The code targets 5.8; small fixes may be needed on older versions. |
| Editor opens an empty "Untitled" level | You haven't created `L_Outpost` yet — do Part 5. |
| Black or very dark screen | Your level has its own Directional Light that's turned off, or Lumen isn't supported by your GPU. Delete extra lights, or set Graphics quality to Low. |
| Everything is plain white/grey | The engine material colour parameter wasn't found. Gameplay still works; report it. |
| Guards walk into walls | Add the Nav Mesh Bounds Volume (Part 5 step 3). |
| Mouse leaves the window | Click inside the game view; Shift+F1 releases it on purpose. |
| Very low FPS | Pause → Settings → Graphics quality Low; close other programs; check Part 0. |

---

## After M1 works — what comes next (agent + you)

1. **M1 fixes:** fix anything from the checklist; tune detection/combat numbers.
2. **M2 — animation:** retarget sword/bow animations (Mixamo / Game Animation Sample),
   attach the sabre to the hand, real attack montages.
3. **M3 — look and sound:** modular mud-brick and tile kit, CC0 materials (Poly Haven /
   ambientCG), torches/fire/dust effects, real footstep/weapon sounds.
4. **M4 — vertical slice:** second mission, small explorable area, UI polish, Uzbek text.
5. **M5 — horse.**

See `docs/GAME_DESIGN.md` §7 for the full roadmap.
