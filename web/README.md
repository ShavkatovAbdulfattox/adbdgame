# Shadows of Transoxiana — web prototype

The same first mission as the Unreal project ("The Sealed Dispatch"), playable in a
browser with Three.js. No install, no build step. Works on Mac, Windows, Linux, and on
phones/tablets (touch controls).

## Play on your computer
Browsers block game files opened directly from disk, so start a tiny local web server.
On a Mac, open **Terminal** in the project folder and run:

```bash
cd web
python3 -m http.server 8000
```

Then open **http://localhost:8000** in Chrome, Safari or Firefox. Press **Ctrl+C** in
Terminal to stop the server.

## Play on a phone
Host it for free with GitHub Pages (see `docs/NEXT_STEPS.md`, "Web version"), then open
the link on the phone. Hold the phone sideways.

## Controls
| Action | Keyboard / mouse | Touch |
|---|---|---|
| Move / look | WASD / mouse (click the game to capture it) | left side drag / right side drag |
| Sprint | Shift | push the stick to the edge |
| Crouch | C or Ctrl | CROUCH |
| Jump | Space | JUMP |
| Dodge | Alt (Option ⌥ on Mac) or V | DODGE |
| Attack / shoot | Left mouse | ATK |
| Block / aim | Right mouse (hold) | BLK/AIM (hold) |
| Switch sabre ↔ bow | Q or Tab | SWAP |
| Take item / takedown | E or F | E |
| Pause | P or Esc | II |

A gamepad also works (Xbox/PlayStation layout).

## Code layout
| File | What it does |
|---|---|
| `src/main.js` | Game loop, mission flow, menus, camera, sky and lighting |
| `src/world.js` | The rabat outpost, collision, line of sight, A* path-finding |
| `src/player.js` | Bahram: movement, stamina, stealth, sabre, bow, takedowns |
| `src/enemy.js` | Guards: body + Patrol → Suspicious → Combat → Search AI |
| `src/arrows.js` | Arrows with gravity, headshots, noise on impact |
| `src/characters.js` | Primitive character models with procedural animation |
| `src/hud.js` | HTML HUD: bars, objective marker, guard awareness icons |
| `src/input.js` | Keyboard, mouse, gamepad, touch |
| `src/audio.js` | Synthesised wind and sound effects (no audio files) |
| `src/config.js` | All tuning numbers in one place |
| `vendor/` | Three.js r160 (MIT licence) |

## Automated test (for developers)
`tests/gameplay.test.mjs` drives the real game in headless Chromium (Playwright) and
checks detection, hiding, takedown, sabre, parry, bow, path-finding, objective,
extraction, death, walls and the breach. Run with a local server on port 8765:

```bash
python3 -m http.server 8765 --directory web &
npx playwright install chromium   # first time only
node web/tests/gameplay.test.mjs
```
