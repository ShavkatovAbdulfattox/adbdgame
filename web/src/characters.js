// Simple humanoid built from primitives, with procedural animation
// (walk cycle, crouch, sword swing, guard, bow aim, death).
// The model faces +Z in its local space; forward(yaw) = (sin yaw, 0, cos yaw).

import * as THREE from '../vendor/three.module.min.js';
import { PALETTE } from './config.js';

export const forward = (yaw) => new THREE.Vector3(Math.sin(yaw), 0, Math.cos(yaw));
export const yawTo = (from, to) => Math.atan2(to.x - from.x, to.z - from.z);
export const angleDiff = (a, b) => Math.atan2(Math.sin(b - a), Math.cos(b - a));

const mat = (color, extra = {}) => new THREE.MeshStandardMaterial({ color, roughness: 0.85, ...extra });

export class CharacterModel {
  /**
   * @param {object} o  { robe, sash, headgear: 'cap'|'helmet', weapon: 'both'|'sword'|'bow' }
   */
  constructor(o) {
    this.root = new THREE.Group();
    this.body = new THREE.Group();
    this.root.add(this.body);

    const robe = mat(o.robe), sash = mat(o.sash), skin = mat(PALETTE.skin), boot = mat(0x2a2018);
    const steel = mat(PALETTE.steel, { metalness: 0.7, roughness: 0.35 }), wood = mat(PALETTE.wood);
    const part = (geo, m, parent, x, y, z) => {
      const mesh = new THREE.Mesh(geo, m);
      mesh.position.set(x, y, z);
      mesh.castShadow = true;
      parent.add(mesh);
      return mesh;
    };

    // Legs (pivot at the hip)
    this.legL = new THREE.Group(); this.legL.position.set(-0.12, 0.95, 0); this.body.add(this.legL);
    this.legR = new THREE.Group(); this.legR.position.set(0.12, 0.95, 0); this.body.add(this.legR);
    for (const leg of [this.legL, this.legR]) {
      part(new THREE.BoxGeometry(0.16, 0.8, 0.18), robe, leg, 0, -0.4, 0);
      part(new THREE.BoxGeometry(0.17, 0.18, 0.28), boot, leg, 0, -0.86, 0.04);
    }

    // Kaftan: torso + flared skirt + sash
    part(new THREE.BoxGeometry(0.44, 0.62, 0.26), robe, this.body, 0, 1.27, 0);
    part(new THREE.CylinderGeometry(0.22, 0.32, 0.5, 10), robe, this.body, 0, 0.85, 0);
    part(new THREE.BoxGeometry(0.46, 0.1, 0.28), sash, this.body, 0, 1.0, 0);

    // Head and headgear
    this.head = part(new THREE.SphereGeometry(0.13, 12, 10), skin, this.body, 0, 1.7, 0);
    if (o.headgear === 'helmet') {
      part(new THREE.ConeGeometry(0.16, 0.32, 10), steel, this.body, 0, 1.9, 0);
      part(new THREE.CylinderGeometry(0.17, 0.2, 0.14, 10, 1, true), mat(0x6b6b6b, { metalness: 0.5, side: THREE.DoubleSide }), this.body, 0, 1.68, 0); // mail aventail
    } else {
      part(new THREE.CylinderGeometry(0.12, 0.14, 0.14, 10), mat(0x3a2e24), this.body, 0, 1.83, 0); // felt cap
    }

    // Arms (pivot at the shoulder)
    this.armL = new THREE.Group(); this.armL.position.set(-0.29, 1.52, 0); this.body.add(this.armL);
    this.armR = new THREE.Group(); this.armR.position.set(0.29, 1.52, 0); this.body.add(this.armR);
    for (const arm of [this.armL, this.armR]) {
      part(new THREE.BoxGeometry(0.12, 0.58, 0.12), robe, arm, 0, -0.29, 0);
      part(new THREE.SphereGeometry(0.06, 8, 6), skin, arm, 0, -0.62, 0);
    }

    // Sabre in the right hand (mildly curved: two angled segments)
    this.sword = new THREE.Group();
    this.sword.position.set(0, -0.62, 0.02);
    this.armR.add(this.sword);
    part(new THREE.BoxGeometry(0.03, 0.035, 0.12), wood, this.sword, 0, 0, -0.04);          // grip
    part(new THREE.BoxGeometry(0.12, 0.03, 0.03), steel, this.sword, 0, 0, 0.03);           // guard
    const blade1 = part(new THREE.BoxGeometry(0.012, 0.04, 0.5), steel, this.sword, 0, 0, 0.28);
    const blade2 = part(new THREE.BoxGeometry(0.012, 0.038, 0.38), steel, this.sword, 0, 0.03, 0.7);
    blade1.rotation.x = -0.03; blade2.rotation.x = -0.14;

    // Composite recurve bow in the left hand
    this.bow = new THREE.Group();
    this.bow.position.set(0, -0.62, 0.05);
    this.armL.add(this.bow);
    const limb = new THREE.Mesh(new THREE.TorusGeometry(0.5, 0.018, 6, 20, Math.PI * 0.85), wood);
    limb.rotation.set(0, Math.PI / 2, Math.PI / 2 + Math.PI * 0.075 + Math.PI);
    limb.position.set(0, 0, -0.38);
    limb.castShadow = true;
    this.bow.add(limb);

    this.setWeapon(o.weapon === 'bow' ? 'bow' : 'sword', o.weapon);
    this.walkPhase = 0;
    this.deathT = -1;
  }

  /** Which weapon is in hand; the other is shown stowed (or hidden). */
  setWeapon(active, owned = 'both') {
    this.active = active;
    this.sword.visible = active === 'sword';
    this.bow.visible = active === 'bow' || owned === 'both';
    if (active !== 'bow' && owned === 'both') {
      // Bow carried on the back.
      this.bow.parent?.remove(this.bow);
      this.body.add(this.bow);
      this.bow.position.set(0.05, 1.25, -0.2);
      this.bow.rotation.set(0, 0, 0.5);
    } else if (active === 'bow') {
      this.bow.parent?.remove(this.bow);
      this.armL.add(this.bow);
      this.bow.position.set(0, -0.62, 0.05);
      this.bow.rotation.set(0, 0, 0);
    }
  }

  /**
   * @param {number} dt
   * @param {object} s  { speed, crouch, attack (0..1 or -1), mirror, guard, aim, draw (0..1), telegraph, dead, stagger }
   */
  animate(dt, s) {
    if (s.dead) {
      if (this.deathT < 0) this.deathT = 0;
      this.deathT = Math.min(1, this.deathT + dt * 2.5);
      this.body.rotation.x = -Math.PI / 2 * this.deathT;
      this.body.position.y = 0.15 * this.deathT;
      return;
    }

    const moving = Math.min(1, s.speed / 4);
    this.walkPhase += dt * (3 + s.speed * 1.6);
    const swing = Math.sin(this.walkPhase) * 0.7 * moving;

    this.legL.rotation.x = swing;
    this.legR.rotation.x = -swing;

    // Crouch: lower and lean forward.
    const targetScale = s.crouch ? 0.68 : 1;
    this.body.scale.y += (targetScale - this.body.scale.y) * Math.min(1, dt * 12);
    this.body.rotation.x = s.crouch ? 0.25 : (s.stagger ? -0.25 : 0.05 * moving);
    this.body.position.y = Math.abs(Math.sin(this.walkPhase)) * 0.04 * moving;

    // Default arm swing.
    let lx = -swing * 0.8, lz = 0.08, rx = swing * 0.8, rz = -0.08, ry = 0;

    if (s.attack >= 0) {
      // Wind back over the shoulder, then cut across.
      const a = s.attack;
      if (a < 0.2) { rx = -2.4 * (a / 0.2); rz = -0.6; }
      else { const k = Math.min(1, (a - 0.2) / 0.45); rx = -2.4 + 3.0 * k; rz = s.mirror ? -0.6 + 1.4 * k : -0.6 + 0.4 * k; ry = (s.mirror ? 1 : -1) * 0.6 * k; }
    } else if (s.telegraph) {
      rx = -2.6; rz = -0.4; // blade raised high: attack coming
    } else if (s.guard) {
      rx = -1.3; rz = 0.9; ry = 0.4;
    }

    if (s.aim) {
      lx = -1.57; lz = 0.0;
      rx = -1.5; rz = 0.25 + 0.5 * (s.draw || 0); ry = -0.3 - 0.4 * (s.draw || 0);
    }

    this.armL.rotation.set(lx, 0, lz);
    this.armR.rotation.set(rx, ry, rz);
  }
}
