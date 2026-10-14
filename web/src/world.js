// The "Karmana rabat": a small walled caravan station built from simple shapes,
// plus collision, line-of-sight raycasts and grid path-finding for the guards.
//
// Layout is written as (north, east) in metres, like the Unreal version.
// Three.js axes: +x = east, +y = up, -z = north.

import * as THREE from '../vendor/three.module.min.js';
import { PALETTE } from './config.js';

export const P = (n, e, y = 0) => new THREE.Vector3(e, y, -n);

// ---------------------------------------------------------------------------
// Procedural textures (no image files needed)
// ---------------------------------------------------------------------------

function makeCanvas(size, draw) {
  const c = document.createElement('canvas');
  c.width = c.height = size;
  draw(c.getContext('2d'), size);
  const tex = new THREE.CanvasTexture(c);
  tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
  tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 4;
  return tex;
}

function speckle(ctx, size, count, alpha, light) {
  for (let i = 0; i < count; i++) {
    const v = light ? 255 : 0;
    ctx.fillStyle = `rgba(${v},${v},${v},${Math.random() * alpha})`;
    const s = Math.random() * 3 + 1;
    ctx.fillRect(Math.random() * size, Math.random() * size, s, s);
  }
}

function mudBrickTexture(base) {
  return makeCanvas(256, (ctx, s) => {
    ctx.fillStyle = base;
    ctx.fillRect(0, 0, s, s);
    // Courses of sun-dried bricks with irregular mortar joints.
    const rows = 8, h = s / rows;
    ctx.strokeStyle = 'rgba(60,40,20,0.28)';
    ctx.lineWidth = 2;
    for (let r = 0; r < rows; r++) {
      ctx.beginPath(); ctx.moveTo(0, r * h); ctx.lineTo(s, r * h + (Math.random() - 0.5) * 2); ctx.stroke();
      const off = (r % 2) * (s / 8);
      for (let x = off; x < s; x += s / 4) {
        ctx.beginPath(); ctx.moveTo(x, r * h); ctx.lineTo(x + (Math.random() - 0.5) * 3, (r + 1) * h); ctx.stroke();
      }
    }
    speckle(ctx, s, 2500, 0.12, false);
    speckle(ctx, s, 1500, 0.10, true);
  });
}

function groundTexture() {
  return makeCanvas(512, (ctx, s) => {
    ctx.fillStyle = '#a8936e';
    ctx.fillRect(0, 0, s, s);
    for (let i = 0; i < 400; i++) {
      ctx.fillStyle = `rgba(${120 + Math.random() * 60},${100 + Math.random() * 40},${70 + Math.random() * 30},0.25)`;
      const r = Math.random() * 30 + 5;
      ctx.beginPath(); ctx.arc(Math.random() * s, Math.random() * s, r, 0, Math.PI * 2); ctx.fill();
    }
    speckle(ctx, s, 9000, 0.15, false);
    speckle(ctx, s, 5000, 0.12, true);
  });
}

function tileTexture() {
  // Simple banna'i-style pattern: glazed turquoise and cobalt with unglazed brick.
  return makeCanvas(128, (ctx, s) => {
    const n = 8, c = s / n;
    for (let y = 0; y < n; y++) {
      for (let x = 0; x < n; x++) {
        const k = (x + y) % 4;
        ctx.fillStyle = k === 0 ? '#1f3f8f' : (k === 2 ? '#9c6b48' : '#2e9c9a');
        ctx.fillRect(x * c, y * c, c - 1, c - 1);
      }
    }
  });
}

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

export class World {
  constructor(scene) {
    this.scene = scene;
    this.colliders = [];   // { type:'box'|'circle', min, max (Vector3) | cx, cz, r, minY, maxY }
    this.torches = [];
    this.H = 35;           // half size of the enclosure

    const mud = mudBrickTexture('#b89a72');
    const mudDark = mudBrickTexture('#a5835a');
    this.mats = {
      ground: new THREE.MeshStandardMaterial({ map: groundTexture(), roughness: 1 }),
      mud: new THREE.MeshStandardMaterial({ map: mud, roughness: 0.95 }),
      mudDark: new THREE.MeshStandardMaterial({ map: mudDark, roughness: 0.95 }),
      fired: new THREE.MeshStandardMaterial({ color: PALETTE.firedBrick, roughness: 0.85 }),
      plaster: new THREE.MeshStandardMaterial({ color: PALETTE.plaster, roughness: 0.9 }),
      tile: new THREE.MeshStandardMaterial({ map: tileTexture(), roughness: 0.35, metalness: 0.05 }),
      dome: new THREE.MeshStandardMaterial({ color: PALETTE.turquoise, roughness: 0.3, metalness: 0.05 }),
      wood: new THREE.MeshStandardMaterial({ color: PALETTE.wood, roughness: 0.9 }),
      hay: new THREE.MeshStandardMaterial({ color: PALETTE.hay, roughness: 1 }),
      cloth: new THREE.MeshStandardMaterial({ color: PALETTE.madder, roughness: 0.95, side: THREE.DoubleSide }),
      rock: new THREE.MeshStandardMaterial({ color: 0x8f7a5c, roughness: 1, flatShading: true }),
      poplar: new THREE.MeshStandardMaterial({ color: PALETTE.poplar, roughness: 1, flatShading: true }),
    };
    mud.repeat.set(1, 1);

    this.build();
    this.buildNavGrid();
  }

  // ---- primitive helpers ---------------------------------------------------

  add(mesh, cast = true, receive = true) {
    mesh.castShadow = cast;
    mesh.receiveShadow = receive;
    this.scene.add(mesh);
    return mesh;
  }

  /** Box centred at (n, e, y) with size (sizeN, sizeE, sizeY). */
  box(n, e, y, sn, se, sy, mat, { solid = true, yaw = 0, texScale = 0 } = {}) {
    const geo = new THREE.BoxGeometry(se, sy, sn);
    if (texScale > 0 && mat.map) {
      // Scale UVs so brick texture density stays constant.
      const uv = geo.attributes.uv;
      const norm = geo.attributes.normal;
      for (let i = 0; i < uv.count; i++) {
        const nx = Math.abs(norm.getX(i)), ny = Math.abs(norm.getY(i));
        const w = nx > 0.5 ? sn : se;
        const h = ny > 0.5 ? sn : sy;
        uv.setXY(i, uv.getX(i) * w / texScale, uv.getY(i) * h / texScale);
      }
    }
    const m = new THREE.Mesh(geo, mat);
    m.position.copy(P(n, e, y));
    m.rotation.y = yaw * Math.PI / 180;
    this.add(m);
    if (solid) {
      this.colliders.push({
        type: 'box',
        min: new THREE.Vector3(e - se / 2, y - sy / 2, -n - sn / 2),
        max: new THREE.Vector3(e + se / 2, y + sy / 2, -n + sn / 2),
      });
    }
    return m;
  }

  cylinder(n, e, y, diameter, height, mat, { solid = true, segments = 24 } = {}) {
    const r = diameter / 2;
    const m = new THREE.Mesh(new THREE.CylinderGeometry(r, r, height, segments), mat);
    m.position.copy(P(n, e, y));
    this.add(m);
    if (solid) this.colliders.push({ type: 'circle', cx: e, cz: -n, r, minY: y - height / 2, maxY: y + height / 2 });
    return m;
  }

  cone(n, e, height, diameter, mat) {
    const r = diameter / 2;
    const m = new THREE.Mesh(new THREE.ConeGeometry(r, height, 10), mat);
    m.position.copy(P(n, e, height / 2));
    this.add(m);
    this.colliders.push({ type: 'circle', cx: e, cz: -n, r: r * 0.8, minY: 0, maxY: height });
    return m;
  }

  // ---- layout --------------------------------------------------------------

  build() {
    const M = this.mats, H = this.H, T = 1.2, WH = 6, Z = WH / 2;

    // Ground
    const groundGeo = new THREE.PlaneGeometry(400, 400);
    groundGeo.attributes.uv.array.forEach((v, i, a) => { a[i] = v * 60; });
    const ground = new THREE.Mesh(groundGeo, M.ground);
    ground.rotation.x = -Math.PI / 2;
    this.add(ground, false, true);

    const wall = (n, e, sn, se) => this.box(n, e, Z, sn, se, WH, M.mud, { texScale: 3 });
    // North + east walls (solid)
    wall(H, 0, T, 2 * H);
    wall(0, H, 2 * H, T);
    // South wall with the gate gap (|east| < 3)
    wall(-H, (-H - 3) / 2, T, H - 3);
    wall(-H, (H + 3) / 2, T, H - 3);
    // West wall with the collapsed breach between north -9 and -2 (player's way in)
    wall((-H - 9) / 2, -H, H - 9, T);
    wall((H - 2) / 2, -H, H + 2, T);
    this.box(-8.5, -H, 0.9, 1, 1.6, 1.8, M.mud, { texScale: 3, yaw: 8 });
    this.box(-2.5, -H, 0.7, 1, 1.6, 1.4, M.mud, { texScale: 3, yaw: -6 });
    this.box(-6, -H + 1.6, 0.15, 0.9, 0.6, 0.3, M.rock);
    this.box(-4.8, -H - 1.7, 0.15, 0.7, 0.8, 0.3, M.rock);

    // Crenellations along the wall tops (instanced for speed)
    this.addMerlons(H, T, WH);

    // Gate portal (fired brick) and glazed band
    this.box(-H, -4.2, 4, 2.6, 2.4, 8, M.fired);
    this.box(-H, 4.2, 4, 2.6, 2.4, 8, M.fired);
    this.box(-H, 0, 7.25, 2.6, 6, 1.5, M.fired);
    this.box(-H - 1.35, 0, 7.6, 0.2, 5.6, 0.5, M.tile, { solid: false });

    // Corner towers (archers stand on top at y = 8)
    for (const [n, e] of [[H, H], [H, -H], [-H, H], [-H, -H]]) {
      this.cylinder(n, e, 4, 7, 8, M.mudDark, { segments: 28 });
      this.cylinder(n, e, 8.3, 7.4, 0.6, M.mudDark, { solid: false, segments: 28 });
    }

    // Domed hall (commander's quarters) against the north wall
    const hallMin = 19, hallMax = H - T / 2, hc = (hallMin + hallMax) / 2, hl = hallMax - hallMin, W = 8;
    this.box(hc, -W, 3.5, hl, 0.8, 7, M.plaster);
    this.box(hc, W, 3.5, hl, 0.8, 7, M.plaster);
    this.box(hallMin, -4.75, 3.5, 0.8, 6.5, 7, M.plaster);
    this.box(hallMin, 4.75, 3.5, 0.8, 6.5, 7, M.plaster);
    this.box(hallMin, 0, 5, 0.8, 3, 4, M.plaster);              // over the 3 m door
    this.box(hc, 0, 7.2, hl, 2 * W + 0.8, 0.4, M.plaster);      // roof
    this.cylinder(hc, 0, 9.65, 10.4, 4.5, M.fired, { solid: false, segments: 32 });
    const dome = new THREE.Mesh(new THREE.SphereGeometry(5, 32, 16, 0, Math.PI * 2, 0, Math.PI / 2), M.dome);
    dome.position.copy(P(hc, 0, 11.9));
    dome.scale.y = 1.1;
    this.add(dome);
    this.box(hallMin - 0.6, -3, 4.5, 1.2, 1.6, 9, M.fired);
    this.box(hallMin - 0.6, 3, 4.5, 1.2, 1.6, 9, M.fired);
    this.box(hallMin - 0.6, 0, 8.7, 1.2, 7.6, 1.2, M.fired);
    this.box(hallMin - 1.25, 0, 9.4, 0.2, 7.6, 0.6, M.tile, { solid: false });
    this.box(32, 0, 0.4, 1, 1.6, 0.8, M.wood);                  // chest with the dispatch
    this.dispatchPos = P(32, 0, 1.1);

    // Rooms along the east wall with shady alleys between them
    for (const n of [-28, -18, -8, 2, 12]) this.box(n, H - 3.6, 1.75, 8, 6, 3.5, M.plaster);

    // Open shed against the west wall
    for (const n of [4, 13, 22]) this.box(n, -27, 1.5, 0.3, 0.3, 3, M.wood);
    this.box(13, -30.7, 3.1, 20, 8, 0.2, M.wood, { solid: false });
    this.box(8, -32, 0.9, 3, 2, 1.8, M.hay);
    this.box(18, -32.5, 0.9, 3, 2, 1.8, M.hay);

    // Red cloth awning inside the gate
    for (const e of [9, 18]) this.box(-29.8, e, 1.4, 0.3, 0.3, 2.8, M.wood);
    this.box(-32, 13.5, 2.9, 5, 12, 0.1, M.cloth, { solid: false });

    // Courtyard: hay stacks (hiding spots), tents, well, crates, brazier
    for (const [n, e] of [[-26, -24], [-12, -4], [3, 16], [12, -6]]) this.box(n, e, 0.9, 3.2, 2.2, 1.8, M.hay);
    for (const [n, e] of [[-18, 16], [-6, 20], [-26.5, 7]]) this.cone(n, e, 4.5, 6, M.cloth);
    this.cylinder(0, -12, 0.5, 2.6, 1, M.mudDark);
    this.box(-13, 7.5, 0.5, 1, 1, 1, M.wood);
    this.box(-12, 8.4, 0.5, 1, 1, 1, M.wood, { yaw: 20 });
    this.box(-12.5, 7.9, 1.5, 1, 1, 1, M.wood, { yaw: 40, solid: false });
    this.box(-3, 3, 0.5, 0.8, 0.8, 1, M.fired);

    // Outside: approach rocks and a row of poplars along an irrigation ditch
    this.box(-7, -H - 12, 0.6, 2, 3, 1.2, M.rock, { yaw: 25 });
    this.box(-2, -H - 9, 0.5, 2, 2.5, 1, M.rock, { yaw: -10 });
    this.box(-9, -H - 26, 0.8, 3, 4, 1.6, M.rock, { yaw: 5 });
    const ditch = new THREE.Mesh(new THREE.PlaneGeometry(70, 1.2), new THREE.MeshStandardMaterial({ color: 0x4a5a5a, roughness: 0.2 }));
    ditch.rotation.x = -Math.PI / 2;
    ditch.position.copy(P(-H - 13.5, 0, 0.02));
    this.add(ditch, false, true);
    for (let e = -30; e <= 30; e += 6) {
      this.box(-H - 15, e, 2, 0.4, 0.4, 4, M.wood);
      const crown = new THREE.Mesh(new THREE.SphereGeometry(1.1, 8, 6), M.poplar);
      crown.scale.y = 4;
      crown.position.copy(P(-H - 15, e, 7.5));
      this.add(crown);
    }

    // Torches (warm point lights that flicker)
    for (const [n, e, y] of [[hc - 3, 0, 4], [-H + 2, -4.2, 3.5], [-H + 2, 4.2, 3.5], [-3, 3, 1.6]]) {
      const light = new THREE.PointLight(0xff8c40, 12, 14, 2);
      light.position.copy(P(n, e, y));
      this.scene.add(light);
      const flame = new THREE.Mesh(new THREE.SphereGeometry(0.12, 8, 6), new THREE.MeshBasicMaterial({ color: 0xffb060 }));
      flame.position.copy(light.position);
      this.scene.add(flame);
      this.torches.push({ light, base: 12, phase: Math.random() * 10 });
    }

    // Gameplay locations
    this.playerStart = P(-5, -H - 22, 0);
    this.extractionPos = P(H + 15, H + 15, 0);
    this.hidingZones = [
      [-26, -24, 2.6, 2.0], [-12, -4, 2.6, 2.0], [3, 16, 2.6, 2.0], [12, -6, 2.6, 2.0],
      [13, -30.7, 9.5, 3.5],              // under the shed roof
      [-32, 13.5, 2.3, 5.8],              // under the awning
      [-23, H - 3.6, 1, 3], [-13, H - 3.6, 1, 3], [-3, H - 3.6, 1, 3], [7, H - 3.6, 1, 3], // alleys
      [-7, -H - 12, 2.6, 2.2],            // rocks outside the breach
    ].map(([n, e, hn, he]) => ({ minX: e - he, maxX: e + he, minZ: -n - hn, maxZ: -n + hn }));
  }

  addMerlons(H, T, WH) {
    const spots = [];
    for (let d = -H + 1; d <= H - 1; d += 2) {
      spots.push(P(H, d), P(d, H));                           // north + east
      if (Math.abs(d) > 4) spots.push(P(-H, d));               // south (skip gate)
      if (d < -9 || d > -2) spots.push(P(d, -H));              // west (skip breach)
    }
    const mesh = new THREE.InstancedMesh(new THREE.BoxGeometry(0.9, 0.7, 0.9), this.mats.mud, spots.length);
    const m4 = new THREE.Matrix4();
    spots.forEach((p, i) => { m4.makeTranslation(p.x, WH + 0.35, p.z); mesh.setMatrixAt(i, m4); });
    this.add(mesh);
  }

  update(t) {
    for (const torch of this.torches) {
      torch.light.intensity = torch.base * (0.8 + 0.2 * Math.sin(t * 9 + torch.phase) * Math.sin(t * 4.3 + torch.phase * 2));
    }
  }

  // ---- queries -----------------------------------------------------------

  isInHidingZone(pos) {
    return this.hidingZones.some(z => pos.x >= z.minX && pos.x <= z.maxX && pos.z >= z.minZ && pos.z <= z.maxZ);
  }

  /** Highest walkable surface under a circle whose feet are at footY (step height 0.45 m). */
  groundHeight(x, z, footY, radius) {
    let h = 0;
    for (const c of this.colliders) {
      if (c.type === 'box') {
        if (x + radius * 0.5 < c.min.x || x - radius * 0.5 > c.max.x || z + radius * 0.5 < c.min.z || z - radius * 0.5 > c.max.z) continue;
        if (c.max.y <= footY + 0.45 && c.max.y > h) h = c.max.y;
      } else {
        const dx = x - c.cx, dz = z - c.cz;
        if (dx * dx + dz * dz > c.r * c.r) continue;
        if (c.maxY <= footY + 0.45 && c.maxY > h) h = c.maxY;
      }
    }
    return h;
  }

  /** Pushes a vertical capsule (feet at pos.y) out of obstacles it overlaps horizontally. */
  resolveCollision(pos, radius, height) {
    for (const c of this.colliders) {
      const minY = c.type === 'box' ? c.min.y : c.minY;
      const maxY = c.type === 'box' ? c.max.y : c.maxY;
      if (maxY <= pos.y + 0.45 || minY >= pos.y + height) continue; // step over / walk under
      if (c.type === 'box') {
        const cx = Math.max(c.min.x, Math.min(pos.x, c.max.x));
        const cz = Math.max(c.min.z, Math.min(pos.z, c.max.z));
        let dx = pos.x - cx, dz = pos.z - cz;
        const d2 = dx * dx + dz * dz;
        if (d2 >= radius * radius) continue;
        if (d2 > 1e-8) {
          const d = Math.sqrt(d2), push = radius - d;
          pos.x += dx / d * push; pos.z += dz / d * push;
        } else {
          // Centre inside the box: push out along the shallowest axis.
          const pushes = [pos.x - c.min.x + radius, c.max.x - pos.x + radius, pos.z - c.min.z + radius, c.max.z - pos.z + radius];
          const i = pushes.indexOf(Math.min(...pushes));
          if (i === 0) pos.x -= pushes[0]; else if (i === 1) pos.x += pushes[1]; else if (i === 2) pos.z -= pushes[2]; else pos.z += pushes[3];
        }
      } else {
        const dx = pos.x - c.cx, dz = pos.z - c.cz;
        const d = Math.hypot(dx, dz), min = c.r + radius;
        if (d >= min) continue;
        if (d < 1e-6) { pos.x += min; continue; }
        pos.x = c.cx + dx / d * min; pos.z = c.cz + dz / d * min;
      }
    }
  }

  /** Distance along the segment a→b to the first obstacle, or Infinity. */
  raycast(a, b) {
    const dir = new THREE.Vector3().subVectors(b, a);
    const len = dir.length();
    if (len < 1e-6) return Infinity;
    dir.divideScalar(len);
    let best = Infinity;
    for (const c of this.colliders) {
      const min = c.type === 'box' ? c.min : _v1.set(c.cx - c.r, c.minY, c.cz - c.r);
      const max = c.type === 'box' ? c.max : _v2.set(c.cx + c.r, c.maxY, c.cz + c.r);
      const t = rayBox(a, dir, min, max);
      if (t < best && t <= len) best = t;
    }
    return best;
  }

  hasLineOfSight(a, b) {
    return this.raycast(a, b) === Infinity;
  }

  // ---- path-finding grid (A*) ----------------------------------------------

  buildNavGrid() {
    this.cell = 0.5;
    this.gridMin = -70;
    this.gridSize = 280; // 140 m square
    const N = this.gridSize, blocked = new Uint8Array(N * N), inflate = 0.45;
    for (const c of this.colliders) {
      const minY = c.type === 'box' ? c.min.y : c.minY;
      const maxY = c.type === 'box' ? c.max.y : c.maxY;
      if (maxY <= 0.45 || minY >= 1.8) continue;
      const x0 = c.type === 'box' ? c.min.x : c.cx - c.r, x1 = c.type === 'box' ? c.max.x : c.cx + c.r;
      const z0 = c.type === 'box' ? c.min.z : c.cz - c.r, z1 = c.type === 'box' ? c.max.z : c.cz + c.r;
      const i0 = this.toCell(x0 - inflate), i1 = this.toCell(x1 + inflate);
      const j0 = this.toCell(z0 - inflate), j1 = this.toCell(z1 + inflate);
      for (let i = i0; i <= i1; i++) for (let j = j0; j <= j1; j++) {
        if (i < 0 || j < 0 || i >= N || j >= N) continue;
        if (c.type === 'circle') {
          const x = this.fromCell(i), z = this.fromCell(j);
          if (Math.hypot(x - c.cx, z - c.cz) > c.r + inflate) continue;
        }
        blocked[j * N + i] = 1;
      }
    }
    this.blocked = blocked;
  }

  toCell(v) { return Math.floor((v - this.gridMin) / this.cell); }
  fromCell(i) { return this.gridMin + (i + 0.5) * this.cell; }
  isBlocked(i, j) {
    const N = this.gridSize;
    return i < 0 || j < 0 || i >= N || j >= N || this.blocked[j * N + i] === 1;
  }

  nearestOpen(i, j) {
    if (!this.isBlocked(i, j)) return [i, j];
    for (let r = 1; r < 8; r++) {
      for (let di = -r; di <= r; di++) for (let dj = -r; dj <= r; dj++) {
        if (!this.isBlocked(i + di, j + dj)) return [i + di, j + dj];
      }
    }
    return null;
  }

  /** Returns a list of Vector3 waypoints (y = 0) from a to b, or null. */
  findPath(a, b) {
    const N = this.gridSize;
    const s = this.nearestOpen(this.toCell(a.x), this.toCell(a.z));
    const g = this.nearestOpen(this.toCell(b.x), this.toCell(b.z));
    if (!s || !g) return null;
    const start = s[1] * N + s[0], goal = g[1] * N + g[0];
    const gScore = new Float32Array(N * N).fill(Infinity);
    const came = new Int32Array(N * N).fill(-1);
    const closed = new Uint8Array(N * N);
    const heap = new MinHeap();
    const h = (k) => { const dx = Math.abs((k % N) - g[0]), dz = Math.abs(((k / N) | 0) - g[1]); return Math.max(dx, dz) + 0.414 * Math.min(dx, dz); };
    gScore[start] = 0;
    heap.push(start, h(start));
    let expanded = 0;
    while (heap.size > 0 && expanded < 30000) {
      const k = heap.pop();
      if (k === goal) break;
      if (closed[k]) continue;
      closed[k] = 1;
      expanded++;
      const ci = k % N, cj = (k / N) | 0;
      for (let di = -1; di <= 1; di++) for (let dj = -1; dj <= 1; dj++) {
        if (!di && !dj) continue;
        const ni = ci + di, nj = cj + dj;
        if (this.isBlocked(ni, nj)) continue;
        if (di && dj && (this.isBlocked(ci + di, cj) || this.isBlocked(ci, cj + dj))) continue; // no corner cutting
        const nk = nj * N + ni;
        const cost = gScore[k] + (di && dj ? 1.414 : 1);
        if (cost < gScore[nk]) {
          gScore[nk] = cost;
          came[nk] = k;
          heap.push(nk, cost + h(nk));
        }
      }
    }
    if (came[goal] === -1 && goal !== start) return null;
    const cells = [];
    for (let k = goal; k !== -1; k = came[k]) { cells.push(k); if (k === start) break; }
    cells.reverse();
    // Keep only corners where line of walkable cells breaks (string pulling).
    const pts = cells.map(k => new THREE.Vector3(this.fromCell(k % N), 0, this.fromCell((k / N) | 0)));
    const out = [];
    let anchor = 0;
    for (let i = 2; i < pts.length; i++) {
      if (!this.gridLineClear(pts[anchor], pts[i])) { out.push(pts[i - 1]); anchor = i - 1; }
    }
    out.push(new THREE.Vector3(b.x, 0, b.z));
    return out;
  }

  gridLineClear(a, b) {
    const d = a.distanceTo(b), steps = Math.ceil(d / (this.cell * 0.5));
    for (let s = 1; s < steps; s++) {
      const t = s / steps;
      if (this.isBlocked(this.toCell(a.x + (b.x - a.x) * t), this.toCell(a.z + (b.z - a.z) * t))) return false;
    }
    return true;
  }
}

const _v1 = new THREE.Vector3(), _v2 = new THREE.Vector3();

function rayBox(o, d, min, max) {
  let tmin = 0, tmax = Infinity;
  for (const ax of ['x', 'y', 'z']) {
    if (Math.abs(d[ax]) < 1e-9) {
      if (o[ax] < min[ax] || o[ax] > max[ax]) return Infinity;
    } else {
      let t1 = (min[ax] - o[ax]) / d[ax], t2 = (max[ax] - o[ax]) / d[ax];
      if (t1 > t2) [t1, t2] = [t2, t1];
      tmin = Math.max(tmin, t1); tmax = Math.min(tmax, t2);
      if (tmin > tmax) return Infinity;
    }
  }
  return tmin;
}

class MinHeap {
  constructor() { this.keys = []; this.pri = []; }
  get size() { return this.keys.length; }
  push(k, p) {
    const K = this.keys, Pr = this.pri;
    K.push(k); Pr.push(p);
    let i = K.length - 1;
    while (i > 0) {
      const parent = (i - 1) >> 1;
      if (Pr[parent] <= Pr[i]) break;
      [K[i], K[parent]] = [K[parent], K[i]]; [Pr[i], Pr[parent]] = [Pr[parent], Pr[i]];
      i = parent;
    }
  }
  pop() {
    const K = this.keys, Pr = this.pri, top = K[0];
    const lk = K.pop(), lp = Pr.pop();
    if (K.length) {
      K[0] = lk; Pr[0] = lp;
      let i = 0;
      for (;;) {
        const l = 2 * i + 1, r = l + 1;
        let m = i;
        if (l < K.length && Pr[l] < Pr[m]) m = l;
        if (r < K.length && Pr[r] < Pr[m]) m = r;
        if (m === i) break;
        [K[i], K[m]] = [K[m], K[i]]; [Pr[i], Pr[m]] = [Pr[m], Pr[i]];
        i = m;
      }
    }
    return top;
  }
}
