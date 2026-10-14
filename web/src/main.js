// Shadows of Transoxiana — web prototype.
// Same mission as the Unreal version ("The Sealed Dispatch"), playable in a browser.

import * as THREE from '../vendor/three.module.min.js';
import { ENEMY, NOISE } from './config.js';
import { World, P } from './world.js';
import { Player } from './player.js';
import { Enemy } from './enemy.js';
import { Arrows } from './arrows.js';
import { Hud } from './hud.js';
import { Input } from './input.js';
import { Audio } from './audio.js';
import { forward } from './characters.js';

const $ = (id) => document.getElementById(id);

const SETTINGS_KEY = 'transoxiana.settings';
function loadSettings() {
  const defaults = { sensitivity: 1, invertY: false, volume: 0.8, quality: 2 };
  try { return { ...defaults, ...JSON.parse(localStorage.getItem(SETTINGS_KEY) || '{}') }; } catch { return defaults; }
}
function saveSettings(s) { try { localStorage.setItem(SETTINGS_KEY, JSON.stringify(s)); } catch { /* private mode */ } }

class Game {
  constructor() {
    this.settings = loadSettings();
    this.time = 0;
    this.state = 'start';       // start | infiltrate | escape | success | failed
    this.paused = false;

    // Renderer
    this.renderer = new THREE.WebGLRenderer({ canvas: $('game'), antialias: true, powerPreference: 'high-performance' });
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.0;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;

    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(62, innerWidth / innerHeight, 0.1, 600);
    this.setupSkyAndLight();

    this.input = new Input($('game'));
    this.input.onPointerLockLost = () => { if (this.isPlaying() && !this.paused) this.openMenu('pause'); };
    this.audio = new Audio();
    this.audio.setVolume(this.settings.volume);
    this.world = new World(this.scene);
    this.arrows = new Arrows(this);
    this.hud = new Hud(this);
    this.player = new Player(this);
    this.enemies = [];
    this.setupDispatchAndFalconer();
    this.applyQuality();
    this.resetMission();
    this.setupMenus();

    addEventListener('resize', () => this.resize());
    this.resize();
    this.last = performance.now();
    requestAnimationFrame((t) => this.frame(t));
  }

  // ---- scene -----------------------------------------------------------------

  setupSkyAndLight() {
    // Dusk sky dome: warm horizon fading to deep blue, with the sun low in the west.
    const sunDir = new THREE.Vector3(-1, 0.16, 0.12).normalize();
    const sky = new THREE.Mesh(new THREE.SphereGeometry(400, 32, 16), new THREE.ShaderMaterial({
      side: THREE.BackSide, depthWrite: false, fog: false,
      uniforms: { sunDir: { value: sunDir } },
      vertexShader: 'varying vec3 vDir; void main(){ vDir = normalize(position); gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }',
      fragmentShader: `varying vec3 vDir; uniform vec3 sunDir;
        void main(){
          float h = clamp(vDir.y, -0.2, 1.0);
          vec3 horizon = vec3(0.95, 0.62, 0.38);
          vec3 zenith = vec3(0.16, 0.24, 0.45);
          vec3 col = mix(horizon, zenith, pow(max(h, 0.0), 0.55));
          float s = max(dot(normalize(vDir), sunDir), 0.0);
          col += vec3(1.0, 0.7, 0.4) * pow(s, 18.0) * 0.6 + vec3(1.0, 0.85, 0.6) * pow(s, 600.0) * 3.0;
          if (vDir.y < 0.0) col = mix(horizon, vec3(0.55, 0.45, 0.35), clamp(-vDir.y * 5.0, 0.0, 1.0));
          gl_FragColor = vec4(col, 1.0);
        }`,
    }));
    this.scene.add(sky);
    this.sky = sky;
    this.scene.fog = new THREE.Fog(0xc99a72, 60, 260);

    this.scene.add(new THREE.HemisphereLight(0x8fa6c8, 0x6b5238, 0.9));
    const sun = new THREE.DirectionalLight(0xffb27a, 2.6);
    sun.position.copy(sunDir).multiplyScalar(120);
    sun.castShadow = true;
    sun.shadow.camera.left = -75; sun.shadow.camera.right = 75;
    sun.shadow.camera.top = 75; sun.shadow.camera.bottom = -75;
    sun.shadow.camera.near = 1; sun.shadow.camera.far = 300;
    sun.shadow.bias = -0.0005;
    sun.shadow.normalBias = 0.04;
    this.scene.add(sun);
    this.scene.add(sun.target);
    this.sun = sun;
  }

  setupDispatchAndFalconer() {
    // The sealed letter: a small scroll with a red wax seal, gently bobbing and glowing.
    const g = new THREE.Group();
    const scroll = new THREE.Mesh(new THREE.CylinderGeometry(0.05, 0.05, 0.32, 10), new THREE.MeshStandardMaterial({ color: 0xe8dcc0, emissive: 0x332211 }));
    scroll.rotation.z = Math.PI / 2;
    const seal = new THREE.Mesh(new THREE.SphereGeometry(0.05, 8, 6), new THREE.MeshStandardMaterial({ color: 0x8e2f25 }));
    seal.position.set(0, 0, 0.05);
    g.add(scroll, seal);
    const glow = new THREE.PointLight(0xffc070, 3, 4, 2);
    glow.position.y = 0.3;
    g.add(glow);
    g.position.copy(this.world.dispatchPos);
    this.scene.add(g);
    this.dispatch = g;

    // Aybike the falconer waits outside the north-east corner with a lantern.
    const f = new THREE.Group();
    const robe = new THREE.MeshStandardMaterial({ color: 0x1f3f8f, roughness: 0.9 });
    const body = new THREE.Mesh(new THREE.CylinderGeometry(0.22, 0.32, 1.5, 10), robe);
    body.position.y = 0.75;
    const head = new THREE.Mesh(new THREE.SphereGeometry(0.13, 10, 8), new THREE.MeshStandardMaterial({ color: 0xb58a6a }));
    head.position.y = 1.65;
    const bird = new THREE.Mesh(new THREE.SphereGeometry(0.12, 8, 6), new THREE.MeshStandardMaterial({ color: 0x5a3e28 }));
    bird.scale.set(0.8, 1.2, 0.9);
    bird.position.set(0.35, 1.45, 0.1);
    const lantern = new THREE.PointLight(0xff9a50, 8, 12, 2);
    lantern.position.set(-0.3, 1.2, 0.3);
    for (const m of [body, head, bird]) { m.castShadow = true; f.add(m); }
    f.add(lantern);
    f.position.copy(this.world.extractionPos);
    this.scene.add(f);
    this.falconer = f;
  }

  spawnGuards() {
    for (const e of this.enemies) e.dispose();
    const H = this.world.H;
    const yawFacing = (dn, de) => Math.atan2(de, -dn); // facing toward (north, east) delta
    const guards = [
      { type: 'sword', at: [-20, -18], yaw: yawFacing(1, 0), patrol: [[-20, -18], [-20, 12], [8, 12], [8, -18]] },
      { type: 'sword', at: [26, -23], yaw: yawFacing(-1, 0), patrol: [[26, -23], [-15, -23]] },
      { type: 'sword', at: [15, -13], yaw: yawFacing(0, 1), patrol: [[15, -13], [15, 13]] },
      { type: 'sword', at: [-H - 7, H - 5], yaw: yawFacing(0, -1), patrol: [[-H - 7, H - 5], [-H - 7, -H - 7], [25, -H - 7], [-H - 7, -H - 7]] },
      { type: 'sword', at: [-H + 4, 0], yaw: yawFacing(-1, 0), patrol: [] },          // inside the gate
      { type: 'sword', at: [16.5, 2.5], yaw: yawFacing(-1, 0), patrol: [] },          // hall door
      { type: 'archer', at: [H, -H], y: 8, yaw: yawFacing(-1, 1), patrol: [] },        // NW tower
      { type: 'archer', at: [-H, H], y: 8, yaw: yawFacing(1, -1), patrol: [] },        // SE tower
    ];
    this.enemies = guards.map(o => new Enemy(this, {
      type: o.type, yaw: o.yaw, pos: P(o.at[0], o.at[1], o.y || 0), patrol: o.patrol.map(([n, e]) => P(n, e)),
    }));
  }

  characters() { return [this.player, ...this.enemies]; }

  // ---- mission -------------------------------------------------------------

  resetMission() {
    this.arrows.clear();
    this.player.reset();
    this.spawnGuards();
    this.dispatch.visible = true;
    this.stats = { detected: 0, kills: 0, takedowns: 0, start: this.time, end: null };
    if (this.state !== 'start') this.state = 'infiltrate';
    $('end').classList.add('hidden');
    this.briefingUntil = this.time + 14;
    this.snapCamera = true;
  }

  isPlaying() { return this.state === 'infiltrate' || this.state === 'escape'; }
  get missionOver() { return this.state === 'success' || this.state === 'failed'; }

  objectiveText() {
    if (this.state === 'infiltrate') return 'Steal the sealed dispatch from the domed hall';
    if (this.state === 'escape') return 'Escape to Aybike the falconer (north-east, outside the walls)';
    return '';
  }
  objectivePos() {
    if (this.state === 'infiltrate') return this.world.dispatchPos;
    if (this.state === 'escape') return this.world.extractionPos;
    return null;
  }

  takeDispatch() {
    this.dispatch.visible = false;
    this.state = 'escape';
    this.audio.pickup();
    this.hud.toast('Dispatch taken. The garrison is uneasy — get out.', 4);
  }

  reportNoise(at, radius, source) {
    for (const e of this.enemies) {
      if (e === source || e.dead) continue;
      if (e.pos.distanceTo(at) <= radius) e.hearNoise(at);
    }
  }

  onEnemyEnteredCombat(enemy) {
    if (!this.enemies.some(o => o !== enemy && !o.dead && o.state === 'combat')) {
      this.stats.detected++;
      this.audio.alert();
    }
  }

  onEnemyKilled(enemy, takedown) {
    this.stats.kills++;
    if (takedown) this.stats.takedowns++;
  }

  canEnemyAttack() {
    return this.enemies.filter(e => !e.dead && e.attacking).length < ENEMY.maxSimultaneousAttackers;
  }

  onPlayerDied() {
    if (this.missionOver) return;
    this.state = 'failed';
    this.stats.end = this.time;
    setTimeout(() => this.showEnd(), 1200);
  }

  checkExtraction() {
    if (this.state !== 'escape') return;
    const d = Math.hypot(this.player.pos.x - this.world.extractionPos.x, this.player.pos.z - this.world.extractionPos.z);
    if (d < 3) {
      this.state = 'success';
      this.stats.end = this.time;
      this.showEnd();
    }
  }

  rank() {
    const s = this.stats;
    if (s.detected === 0 && s.kills === 0) return 'SHADOW — never seen, no blood spilled';
    if (s.detected === 0) return 'FALCON — never seen';
    return 'SWORD — the hard way';
  }

  showEnd() {
    this.input.unlockPointer();
    const won = this.state === 'success';
    const secs = Math.round(this.stats.end - this.stats.start);
    $('end-title').textContent = won ? 'MISSION COMPLETE' : 'BAHRAM HAS FALLEN';
    $('end-title').className = won ? 'won' : 'lost';
    $('end-story').textContent = won ? "Aybike's falcon carries the dispatch to Samarkand before dawn." : 'The raiders keep their secret. Try another way in.';
    $('end-rank').textContent = won ? this.rank() : '';
    $('end-stats').textContent = `Time ${Math.floor(secs / 60)}:${String(secs % 60).padStart(2, '0')} · Times detected ${this.stats.detected} · Takedowns ${this.stats.takedowns} · Guards killed ${this.stats.kills}`;
    $('end').classList.remove('hidden');
  }

  // ---- menus -----------------------------------------------------------------

  setupMenus() {
    $('btn-play').addEventListener('click', () => this.beginPlay());
    $('btn-resume').addEventListener('click', () => this.closeMenu());
    $('btn-settings').addEventListener('click', () => this.openMenu('settings'));
    $('btn-restart').addEventListener('click', () => { this.closeMenu(); this.resetMission(); });
    $('btn-back').addEventListener('click', () => this.openMenu('pause'));
    $('btn-again').addEventListener('click', () => { this.resetMission(); this.beginPlay(); });
    $('btn-pause-touch').addEventListener('click', () => this.openMenu('pause'));
    // Clicking the game view re-captures the mouse (browsers require a click after Esc).
    $('game').addEventListener('click', () => { if (this.isPlaying() && !this.paused && !this.input.locked) this.input.lockPointer(); });

    const s = this.settings;
    const sens = $('set-sens'), inv = $('set-invert'), vol = $('set-volume'), q = $('set-quality');
    sens.value = s.sensitivity; inv.checked = s.invertY; vol.value = s.volume; q.value = s.quality;
    const sync = () => {
      s.sensitivity = parseFloat(sens.value); s.invertY = inv.checked; s.volume = parseFloat(vol.value); s.quality = parseInt(q.value, 10);
      $('sens-val').textContent = s.sensitivity.toFixed(1);
      $('vol-val').textContent = `${Math.round(s.volume * 100)}%`;
      this.audio.setVolume(s.volume);
      this.applyQuality();
      saveSettings(s);
    };
    for (const el of [sens, inv, vol, q]) el.addEventListener('input', sync);
    sync();
    if (this.input.isTouch) $('start-controls').innerHTML = 'Left side: move (push far = sprint) · Right side: look<br>Buttons: ATK, BLK/AIM, JUMP, CROUCH, E (take / takedown), SWAP weapon';
  }

  beginPlay() {
    this.audio.start();
    $('start').classList.add('hidden');
    $('end').classList.add('hidden');
    if (this.state === 'start') { this.state = 'infiltrate'; this.resetMission(); }
    else if (this.missionOver) this.resetMission();
    this.snapCamera = true;
    this.closeMenu();
  }

  openMenu(which) {
    this.paused = true;
    this.input.unlockPointer();
    $('menu').classList.remove('hidden');
    $('menu-pause').classList.toggle('hidden', which !== 'pause');
    $('menu-settings').classList.toggle('hidden', which !== 'settings');
  }

  closeMenu() {
    this.paused = false;
    $('menu').classList.add('hidden');
    this.input.lockPointer();
  }

  applyQuality() {
    const q = this.settings.quality; // 0 low .. 3 epic
    const ratio = [0.6, 0.85, 1, Math.min(devicePixelRatio, 2)][q];
    this.renderer.setPixelRatio(ratio);
    this.renderer.shadowMap.enabled = q >= 1;
    this.sun.shadow.mapSize.set(q >= 3 ? 4096 : 2048, q >= 3 ? 4096 : 2048);
    if (this.sun.shadow.map) { this.sun.shadow.map.dispose(); this.sun.shadow.map = null; }
    this.scene.traverse(o => { if (o.material) o.material.needsUpdate = true; });
    this.resize();
  }

  resize() {
    this.camera.aspect = innerWidth / innerHeight;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(innerWidth, innerHeight, false);
  }

  // ---- loop ------------------------------------------------------------------

  frame(now) {
    const dt = Math.min(0.05, (now - this.last) / 1000);
    this.last = now;
    this.input.pollGamepad(dt);

    if (this.input.wasPressed('pause') && (this.isPlaying())) {
      if (this.paused) this.closeMenu(); else this.openMenu('pause');
    }
    if (this.missionOver && this.input.wasPressed('confirm') && !$('end').classList.contains('hidden')) {
      this.resetMission(); this.beginPlay();
    }

    const running = this.state !== 'start' && !this.paused;
    if (running) this.step(dt);
    this.updateCamera(dt);
    this.world.update(this.time);
    if (running || this.state === 'start') this.audio.update(dt);
    this.hud.update();
    $('briefing').classList.toggle('hidden', !(this.isPlaying() && this.time < this.briefingUntil));
    $('hud').classList.toggle('hidden', this.state === 'start');

    this.renderer.render(this.scene, this.camera);
    this.input.endFrame();
    requestAnimationFrame((t) => this.frame(t));
  }

  step(dt) {
    this.time += dt;
    this.player.update(dt);
    for (const e of this.enemies) e.update(dt);
    this.arrows.update(dt);
    this.checkExtraction();
    this.dispatch.position.y = this.world.dispatchPos.y + Math.sin(this.time * 2) * 0.08;
    this.dispatch.rotation.y += dt;
  }

  updateCamera(dt) {
    const p = this.player;
    const aim = p.aiming;
    const dist = aim ? 1.9 : 4.4;
    const shoulder = aim ? 0.65 : 0.45;
    const pivot = p.pos.clone().add(new THREE.Vector3(0, p.crouched ? 1.25 : 1.6, 0));
    const yaw = p.camYaw, pitch = p.camPitch;
    const back = forward(yaw).multiplyScalar(-Math.cos(pitch) * dist);
    const side = new THREE.Vector3(-Math.cos(yaw), 0, Math.sin(yaw)).multiplyScalar(shoulder);
    let desired = pivot.clone().add(back).add(side).add(new THREE.Vector3(0, Math.sin(pitch) * dist, 0));
    // Keep the camera out of walls.
    const hit = this.world.raycast(pivot, desired);
    if (hit !== Infinity) desired = pivot.clone().add(desired.clone().sub(pivot).normalize().multiplyScalar(Math.max(0.4, hit - 0.25)));
    desired.y = Math.max(desired.y, 0.3);
    if (this.snapCamera) { this.camera.position.copy(desired); this.snapCamera = false; }
    else this.camera.position.lerp(desired, Math.min(1, dt * 14));
    if (this.state === 'start') {
      // Slow orbit over the outpost on the title screen.
      const t = performance.now() / 1000 * 0.05;
      this.camera.position.set(Math.sin(t) * 75, 32, Math.cos(t) * 75);
      this.camera.lookAt(0, 4, 0);
    } else {
      const lookAt = pivot.clone().add(side).addScaledVector(forward(yaw), 10).add(new THREE.Vector3(0, -Math.sin(pitch) * 10 + 0.3, 0));
      this.camera.lookAt(lookAt);
    }
    this.sun.target.position.set(p.pos.x, 0, p.pos.z);
    this.sun.position.copy(this.sun.target.position).add(new THREE.Vector3(-1, 0.16, 0.12).normalize().multiplyScalar(120));
    this.sky.position.copy(this.camera.position);
  }
}

const game = new Game();
// Exposed for automated tests and debugging in the browser console.
window.__game = game;
window.__P = P;
window.__NOISE = NOISE;
