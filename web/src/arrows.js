// Arrows with gravity drop. Damage characters (head hits x2.5), stick into the
// world, and — when fired by the player — make noise that guards investigate.

import * as THREE from '../vendor/three.module.min.js';
import { NOISE, PALETTE } from './config.js';

const GRAVITY = 9.8 * 0.6;
const shaftGeo = new THREE.CylinderGeometry(0.012, 0.012, 0.8, 5);
shaftGeo.rotateX(Math.PI / 2);
const shaftMat = new THREE.MeshStandardMaterial({ color: PALETTE.wood });

export class Arrows {
  constructor(game) {
    this.game = game;
    this.list = [];
  }

  fire(from, velocity, damage, shooter, byPlayer) {
    const mesh = new THREE.Mesh(shaftGeo, shaftMat);
    mesh.position.copy(from);
    mesh.castShadow = true;
    this.game.scene.add(mesh);
    this.list.push({ mesh, pos: from.clone(), vel: velocity.clone(), damage, shooter, byPlayer, stuck: false, life: 12 });
  }

  update(dt) {
    const { world } = this.game;
    for (const a of this.list) {
      a.life -= dt;
      if (a.stuck) continue;
      const next = a.pos.clone().addScaledVector(a.vel, dt);
      next.y -= 0.5 * GRAVITY * dt * dt;
      a.vel.y -= GRAVITY * dt;

      // Characters (approximate each as a vertical capsule).
      const victim = this.findVictim(a, next);
      if (victim) {
        const head = a.pos.y > victim.pos.y + victim.height - 0.35;
        const friendlyFire = !a.byPlayer && victim.isEnemy;
        if (!friendlyFire) victim.takeDamage(a.damage * (head ? 2.5 : 1), { attacker: a.shooter, melee: false, headshot: head });
        a.life = 0;
        continue;
      }

      // World
      const hitDist = world.raycast(a.pos, next);
      if (hitDist !== Infinity || next.y <= 0) {
        if (hitDist !== Infinity) a.pos.addScaledVector(next.clone().sub(a.pos).normalize(), hitDist);
        else a.pos.copy(next).setY(0.05);
        a.stuck = true;
        a.life = Math.min(a.life, 8);
        a.mesh.position.copy(a.pos);
        if (a.byPlayer) {
          this.game.audio.arrowImpact();
          this.game.reportNoise(a.pos, NOISE.arrowImpact, null);
        }
        continue;
      }
      a.pos.copy(next);
      a.mesh.position.copy(a.pos);
      a.mesh.lookAt(a.pos.clone().add(a.vel));
    }
    this.list = this.list.filter(a => {
      if (a.life > 0) return true;
      this.game.scene.remove(a.mesh);
      return false;
    });
  }

  findVictim(a, next) {
    const seg = new THREE.Line3(a.pos, next), closest = new THREE.Vector3();
    for (const c of this.game.characters()) {
      if (c === a.shooter || c.dead) continue;
      const bottom = c.pos.y, top = c.pos.y + c.height;
      seg.closestPointToPoint(new THREE.Vector3(c.pos.x, (a.pos.y + next.y) / 2, c.pos.z), true, closest);
      if (closest.y < bottom || closest.y > top) continue;
      if (Math.hypot(closest.x - c.pos.x, closest.z - c.pos.z) < c.radius + 0.05) return c;
    }
    return null;
  }

  clear() {
    for (const a of this.list) this.game.scene.remove(a.mesh);
    this.list = [];
  }
}
