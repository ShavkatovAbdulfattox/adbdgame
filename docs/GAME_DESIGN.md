# Game Design Document — Shadows of Transoxiana

_Working title. Last updated: 2026-10-14. Version 0.1 (pre-production)._

> **Labels used in this document**
> - **[H]** = documented history (source in §3 or `VISUAL_DIRECTION.md`).
> - **[F]** = fiction invented for the game.
> - **[V]** = plausible but **must be verified** before it appears in-game (see §9).

---

## 1. One-page concept

**Pitch.** A grounded third-person stealth-action game set in Transoxiana in the
1380s–1390s, while Amir Temur builds his empire. You are **Bahram**, a scout and
intelligence courier from a village in the Zarafshan valley, working for the network that
carries messages and secrets between Temur's commanders. You infiltrate caravan
stations, steppe camps and city quarters, steal and carry intelligence, and fight with
sabre and composite bow — and slowly learn what the empire's victories cost the people
you grew up with.

| | |
|---|---|
| Genre | Third-person historical action-adventure: stealth, melee, archery, light RPG |
| Inspirations (feel only) | Assassin's Creed (city infiltration), Ghost of Tsushima (cinematic landscapes, readable duels), Kingdom Come: Deliverance (grounded history). Nothing is copied. |
| Player fantasy | "I am the unseen rider who decides what the emirs know — and what they never learn." |
| Core loop | **Scout** (observe patrols, find routes) → **Infiltrate** (shadows, distraction, disguise) → **Act** (steal / rescue / eliminate) → **Escape** (on foot, later on horseback) → **Report** (deliver intelligence; choices change who trusts you). |
| Pillars | 1. **Information is the weapon** — knowing beats killing. 2. **Weighty, readable combat** — few enemies at once, telegraphed attacks, parries matter. 3. **A real place** — every location grounded in researched Central Asian history. 4. **Small scale, deep detail** — dense areas, not empty maps. |
| Platform | Windows PC (keyboard/mouse + gamepad). Engine: Unreal Engine 5.8. |
| Scope | Campaign target 15–25 h **long-term**. Current goal: a 10–15 min playable mission (M1), then a 30–45 min vertical slice (M4). |
| Languages | English and Uzbek UI/subtitles at vertical slice; Russian later. Dialogue in-world uses period-appropriate names and terms (see §3.4). |
| Rating intent | Mature (violence), no gratuitous gore, no sexual content. |

---

## 2. Setting choice

**Primary period: 1386–1391** — within the brief's recommended window (1370–1405).

Why this window:
- **[H]** While Temur campaigned in Iran in the late 1380s, **Tokhtamysh** of the Golden
  Horde, once his protégé, turned against him; **[V]** Golden Horde forces raided
  Transoxiana in **1387–88** and the Sufi rulers of **Khwarazm** sided with Tokhtamysh.
- **[H]** Temur's response: Urgench (Khwarazm) was razed in **1388**; in **1391** he led
  an army deep into the steppe and defeated Tokhtamysh (battle often named Kondurcha,
  18 June 1391 **[V]**).
- This gives the game: a homeland under raid (personal stakes), a betrayed alliance
  (intrigue), a besieged city (Bukhara/Urgench), a long steppe march (scouting,
  horseback), and a climactic real battle where a scout's information matters — without
  pretending one man won it.

**Other eras** appear only as clearly marked content:
- *Flashbacks / "memories of the city"* — Samanid Bukhara (9th–10th c.), the Mongol
  sack of 1220–21, told through storytellers, ruins and inscriptions.
- *Epilogue / possible DLC* — Samarkand 1404 (Clavijo's embassy, Bibi-Khanym under
  construction), Ulugh Beg's Samarkand (1420s) only as an explicitly later chapter.

### 2.1 Timeline used by the game

| Year | Event | Status |
|---|---|---|
| 892–907 | Ismail Samani rules from Bukhara | [H] flashback only |
| 1219–1221 | Mongol conquest; Bukhara, Samarkand, Urgench sacked | [H] flashback only |
| 1336 | Temur born near Kesh (Shahrisabz) | [H] |
| 1370 | Temur defeats Amir Husayn at Balkh; rules as *amir* through a Chinggisid puppet khan | [H] |
| 1371 | Samarkand made capital; new walls and citadel with the Kok Saray | [H] |
| 1372–1379 | Campaigns against the Sufi dynasty of Khwarazm; Urgench taken 1379 | [H]/[V] exact years |
| 1380 | Ak-Saray palace begun at Shahrisabz | [H] |
| 1386–1388 | Temur in Iran; Tokhtamysh raids Transoxiana; Khwarazm revolts | [H]/[V] details |
| **1387 (autumn)** | **Game begins: Bahram's village is raided [F]** | [F] |
| 1388 | Urgench razed after the revolt | [H] |
| 1389–1390 | Pursuits into Moghulistan / the steppe | [V] |
| **1391** | **Steppe campaign against Tokhtamysh — campaign finale** | [H], battle date [V] |
| 1395 | Second Tokhtamysh war (Terek), Sarai sacked | [H] epilogue reference |
| 1399–1404 | Bibi-Khanym mosque built | [H] epilogue only |
| 1403–1404 | Gur-e Amir built; Clavijo reaches Samarkand Sept 1404 | [H] epilogue only |
| 19 Feb 1405 | Temur dies at Otrar | [H] epilogue only |
| 1417–1420 | Ulugh Beg madrasa on the Registan | [H] **not in main game** |

**Anachronism guardrails** (things that must *not* appear in 1387–91 scenes): the three-
madrasa Registan (17th c. buildings; Ulugh Beg's is 1417–20), Bibi-Khanym, Gur-e Amir,
Ulugh Beg's observatory, mirror armour (*char-aina*, 16th–17th c.), firearms, strongly
curved Safavid *shamshir*.

---

## 3. World and people

### 3.1 Regions (long-term)
| Region | Role in game | Highlights |
|---|---|---|
| **Samarkand** (capital) | Intrigue hub: court, garden palaces, markets | Kok Saray citadel (reconstruction **[F]** — nothing survives), Shah-i Zinda tombs (many 1370s–80s), garden palaces outside the walls |
| **Bukhara** | Scholarly city under threat of siege | Kalyan minaret (1127), Samanid mausoleum, dense mahallas, caravanserais |
| **Zarafshan valley** | Home region; villages, orchards, canals | Tutorial and emotional anchor |
| **Kyzylkum / Khwarazm road** | Caravans, rabats, desert survival | Ancient fortress ruins (Ayaz-Kala etc., already ruins by 1387) |
| **Urgench (Khwarazm)** | Siege act | Pre-1388 city; Turabek-Khanum mausoleum (c. 1360–80) |
| **Steppe north of the Syr Darya** | Finale: army on the march, scouting | Grassland, river crossings, nomad camps |
| **Kesh / Shahrisabz** | Temur's home town; Ak-Saray under construction | Mid-game hub |

### 3.2 Protagonist — Bahram [F]
- **Origin:** born c. 1364 in a Persian-speaking farming village near Samarkand; his
  mother's family are Turkic-speaking herders. Bilingual (Persian and Chagatai Turkic),
  which is why the network recruits him.
- **Skills:** rider, archer, tracker; trained in the sword by a veteran of Temur's
  Khwarazm campaigns; taught reading and ciphers by a Bukharan scribe.
- **Personality:** observant, dry humour, loyal to people rather than causes; impatient
  with ceremony. **Flaw:** believes he can protect everyone alone; keeps secrets from
  those who love him.
- **Want vs need:** wants revenge on the raiders who burned his village; needs to see that
  the empire which protects him also burns villages.
- **Arc:** Survivor → loyal instrument → doubter → his own man. Final choice (§4) decides
  whether he stays in the network, exposes a commander, or leaves with the people he
  saved.
- **Relationships [F]:** *Gulnor* (sister, survives, becomes a weaver in Samarkand);
  *Ustad Malik* (old swordsman, mentor); *Mirza Sa'id* (network handler, charming,
  ruthless); *Aybike* (Khwarazmian falconer, rival agent turned ally).

### 3.3 Factions
| Faction | Type | Notes |
|---|---|---|
| Amir Temur and the Barlas emirs | [H] | Temur appears rarely, at a distance, as a historical figure; his words are paraphrased from sources or clearly invented in private scenes only. |
| "The Falconers" — courier and intelligence network | **[F]** | Built on the documented Mongol-era relay/post system (*yam*) **[V]**; the secret network itself is invented. |
| Sufi dynasty of Khwarazm | [H] | Rulers of Urgench; antagonists in Act 2 but shown with their own reasons. |
| Tokhtamysh and the Golden Horde | [H] | The external threat; individual commanders are [F]. |
| Merchants and caravan houses | [F] | Reputation system; information brokers. |
| Townsfolk, farmers, craftsmen | [F] | The people the final choice is about. |

### 3.4 Language and names
- **[H]** Persian was the language of administration and literature; Chagatai Turkic the
  language of the Turko-Mongol military elite. Modern Uzbek is a later standard.
- Game approach: the **UI and subtitles** are in modern Uzbek/English for players;
  **in-world terms** use period forms (*amir*, *tümen*, *ming*, *rabat*, *khil'at*,
  *mahalla*), explained in a glossary. Voice-over (future) uses Persian/Chagatai flavour
  lines with subtitles, reviewed by a specialist.

---

## 4. Story outline (campaign, long-term)

Fictional mission concepts from the brief, mapped onto the timeline. All are **[F]**
unless marked.

| Act | Mission | Summary |
|---|---|---|
| 1 — 1387 | 1. Ashes of the Homeland | Raiders burn Bahram's village; escape with Gulnor (tutorial). |
| | 2. City of Secrets | Samarkand: Mirza Sa'id recruits Bahram; first palace-quarter infiltration. |
| | **3. The Sealed Dispatch** (**prototype mission, §5**) | Steal a dispatch from a captured rabat on the Bukhara road. |
| 2 — 1388 | 4. The Silent Caravan | Caravans vanish on the Khwarazm road. |
| | 5. Siege Preparations | Infiltrate the camp outside Urgench [H: siege/razing 1388]. |
| | 6. The Price of Loyalty | Choose: warn a suburb before the assault, or obey orders. |
| 3 — 1389–90 | 7. The Shadow at Court | A commander sells information to Tokhtamysh. |
| | 8. The Broken Alliance | Two emirs feud; Bahram must decide whose evidence to deliver. |
| 4 — 1391 | 9. The Battle Before Dawn | Scout the Horde's formation and sabotage supply before the steppe battle [H battle, F details]. |
| | 10. The Fate of Transoxiana | Final choice → **3 endings**: *The Emir's Shadow* (stay), *The Witness* (expose the commander, exile), *The Homecoming* (leave the network, rebuild the village). |

Side content: character quests (Gulnor, Aybike, Ustad Malik), investigation contracts,
"historical discoveries" (inscriptions and ruins unlock codex entries with sources),
hunting and tracking, caravan escort.

---

## 5. First mission (prototype M1) — "The Sealed Dispatch"

**Date/place:** autumn 1387 **[F event]**, a fortified **rabat** (caravan station) on the
road between Bukhara and Samarkand, seized by raiders allied to the Horde.

**Briefing (text):** _"The raiders who hold the Karmana rabat carry a sealed letter
naming who in Bukhara will open a gate to them. Take it before dawn. Kill only if you
must — a quiet theft tells us more than a massacre."_ (Karmana is a real town on that road
**[V location of any rabat]**; the rabat and its garrison are fictional.)

**Layout (procedurally built, ~70 × 70 m):**
- Square mud-brick enclosure, 6 m walls, round corner towers, one **main gate** (south,
  guarded) and a **collapsed section** of the west wall (player's entry).
- Courtyard: well, hay stacks and cloth awnings (**hiding spots**), raiders' tents,
  stacked loot.
- **Domed hall** (north side) with a turquoise-glazed dome — the commander's quarters,
  where the **dispatch** lies on a chest.
- **Extraction:** Aybike the falconer waits beyond the **north-east** corner; the player
  leaves through the gate area or over the breach and reaches her.

**Enemies:** 6 swordsmen (4 patrolling routes, 2 static guards at gate and hall) and
2 archers on the towers.

**Flow and approaches:**
1. **Observe** from the ridge outside the breach. HUD objective: *"Steal the sealed
   dispatch from the domed hall."*
2. **Infiltrate:** crouch through hay/awning shadows; time patrols; shoot an arrow into
   the ground to make noise and pull a guard away (distraction); or take down a lone
   guard from behind (silent takedown).
3. **Take the dispatch** (interact). Objective changes to *"Escape to Aybike (north-east)."*
   All guards become **heightened** (faster detection).
4. **Escape:** sneak out, or fight. Being detected does not fail the mission; it makes
   it harder.
5. **Ending screen:** *Mission complete* with stats — time, times detected, takedowns,
   kills — and a rank: **Shadow** (never detected, no kills), **Falcon** (never in
   combat), **Sword** (everything else). Failure = Bahram dies → restart.

**Teaches:** movement, crouch-stealth, detection meters, distraction, takedown, sword
duel with block/parry, bow aim/shot, objective → escape loop.

**Explicitly not in M1:** horses, disguise, dialogue, voice-over, save system.

---

## 6. Gameplay systems

### 6.1 Movement
Walk, run, **sprint** (stamina), **crouch/sneak** (quiet, slow, lower visibility),
**jump**, **dodge** (short invulnerable step, stamina). Later: climbing low walls,
ladders, rooftops (M3), horse (M5).

### 6.2 Stealth
- **Awareness meter** per enemy: 0 → *Suspicious* (investigates) → full → *Alert*
  (combat). Fill rate depends on distance, view angle, player stance (crouch ×0.5),
  movement, and **hiding zones** (crouched in shade/hay ×0.25).
- **Noise:** sprinting, landing, combat and arrow impacts emit noise; guards
  investigate the source.
- **Search:** after losing the player, guards search the last known position, then
  return to patrol at **heightened** awareness.
- **Takedown:** from behind an unaware enemy within 2 m — silent kill.
- Later: disguise, body discovery, group alerts, light-level stealth.

### 6.3 Combat
- **Sabre:** light attack combo (2 hits), **block** (hold; costs stamina when hit),
  **parry** (block in the last 0.25 s before a hit → enemy staggered, open to a heavy
  counter), **dodge**.
- **Composite bow:** hold to aim (over-the-shoulder camera), draw for power, release;
  arrows drop with gravity; headshots on unaware enemies kill.
- **Enemies telegraph** attacks (wind-up ~0.6 s). At most 2 attack the player at once
  (token system, M3).
- **Archetypes:** swordsman, archer (M1); spearman with shield, elite heavy cavalry,
  dog handler (later).
- Weapons by setting **[V]**: mildly curved sabres and straight swords, composite
  recurve bows with thumb-ring draw, maces, spears, round shields (*kalkan*), lamellar
  and mail. See `VISUAL_DIRECTION.md`.

### 6.4 Horseback (milestone M5)
Call horse, mount/dismount, three gaits, mounted bow, chase/escape, caravan escort.
Horse is a separate pawn with animation-driven locomotion.

### 6.5 Warfare (later)
Scouting formations (mark and report), sabotage (supply carts, horse lines),
small-squad command (2–4 soldiers, simple orders), set-piece battles where the player's
missions change the battle's starting conditions — never a one-man army.

### 6.6 Progression and economy (later)
- Skill trees: **Shadow** (stealth), **Sabre** (melee), **Bow**, **Rider**, **Ear**
  (intelligence: reveal patrols, read ciphers, contacts).
- Equipment: crafted/bought from smiths; period-appropriate only.
- **Reputation** with Falconers, emirs' households, merchants, townsfolk; gates access
  to missions and endings.
- Information as a resource: rumours and contacts reveal optional routes and targets.
- No microtransactions, no loot boxes, no checklist collectibles.

---

## 7. Development roadmap

| Milestone | Goal | Exit criteria | Where |
|---|---|---|---|
| **M0 Pre-production** | Docs, research, engine choice, project skeleton | All docs in `docs/`, project opens and compiles on your PC | **This session** (docs + files); you verify compile |
| **M1 Playable prototype** | Grey-box "Sealed Dispatch" mission | Mission completable start→end; sword, bow, stealth, AI states, HUD, pause/settings all work in PIE | Code written this session — **untested**; you test |
| M2 Characters & animation | Mannequin → retargeted animations; sword/bow montages; enemy variety | Attacks animated; hit windows from anim notifies | |
| M3 Outpost art & audio | Modular mud-brick/tile kit, Megascans/Poly Haven materials, Lumen lighting, real SFX | Screenshots match `VISUAL_DIRECTION.md`; 60 FPS on recommended PC | |
| **M4 Vertical slice** | Polished mission + second mission (village/escape) + small explorable area; UI polish; Uzbek localisation | 30–45 min playable, external playtest | |
| M5 Horse | Mount, ride, mounted archery, chase | Ride across a 1 km² landscape | |
| M6+ Expansion | Samarkand quarter, quest system, factions, battles | Per-feature tests | |

---

## 8. Risk assessment

| Risk | Area | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| Project too big for a beginner/solo team | Scope | **High** | High | Vertical slice first; strict milestones; cut features before quality. |
| C++ written without the editor fails to compile | Tech | Medium | Medium | Small, conventional code; you send compile errors; agent fixes. Blueprint fallback for content. |
| PC too weak for Lumen/Nanite | Performance | Medium | High | Scalability presets, instanced meshes, budgets in `TECHNICAL_ARCHITECTURE.md`; profile every milestone. |
| Animation quality (sword/bow/horse) | Animation | **High** | High | Use free GASP/mannequin/Mixamo animations; procedural placeholders until then; horse deferred to M5. |
| AI feels unfair or dumb | AI | Medium | High | Visible awareness meters, telegraphs, tunable values; playtest each milestone. |
| Historical errors / anachronisms | History | **High** | High | Fact-check process (§9), [H]/[F]/[V] labels, anachronism guardrails (§2.1). |
| Cultural misrepresentation or exoticism | History/Art | Medium | High | Consult Uzbek historians/cultural reviewers before VS; avoid stereotypes; respectful depiction of religious spaces. |
| Portraying Temur (a controversial, revered figure) | Narrative | Medium | High | Show documented actions; present multiple perspectives; avoid inventing atrocities or glorifying violence. |
| Asset licences (Fab, Mixamo, CC-BY) | Legal | Medium | High | `ASSET_MANIFEST.md` records every licence; Fab/Mixamo assets kept out of public repo. |
| Binary asset merge conflicts | Pipeline | Medium | Medium | Git LFS, one owner per map/asset, small commits. |

---

## 9. Historical fact-checking process

1. Every place, date, object or person used in content gets a **[H]/[F]/[V]** label in
   the design docs.
2. **[V]** items must be confirmed against **two independent reputable sources**
   (Encyclopaedia Iranica, Britannica, museum catalogues, UNESCO, peer-reviewed works such
   as B. F. Manz, *The Rise and Rule of Tamerlane*, 1989) before shipping.
3. Primary-source bias is noted: Ibn Arabshah is hostile to Temur; Sharaf al-Din Yazdi's
   *Zafarnama* is a panegyric; Clavijo is an outsider eyewitness (1404).
4. Visual evidence from **later** (1430s–1490s Shiraz/Herat) paintings is labelled as
   *later evidence* when used for 1387–91 designs.
5. An in-game **codex** shows sources and marks fictional elements, so players can tell
   history from invention.
6. Before the vertical slice: review by at least one specialist in Timurid history and
   one Uzbek cultural reviewer.
