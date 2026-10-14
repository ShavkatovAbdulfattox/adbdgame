// A raider guard: body (health, telegraphed attacks, blocking, stagger, arrows)
// and brain (the same Patrol → Suspicious → Combat → Search state machine as the
// Unreal version). Movement follows A* paths on the world's navigation grid.

import * as THREE from '../vendor/three.module.min.js';
import { ENEMY as C, NOISE, PALETTE } from './config.js';
import { CharacterModel, forward, yawTo, angleDiff } from './characters.js';

export class Enemy {
  /**
   * @param {object} o { type: 'sword'|'archer', pos: Vector3, yaw, patrol: Vector3[] }
   */
  constructor(game, o) {
    this.game = game;
    this.isEnemy = true;
    this.type = o.type;
    this.radius = C.radius;
    this.height = 1.85;
    this.pos = o.pos.clone();
    this.yaw = o.yaw;
    this.home = o.pos.clone();
    this.homeYaw = o.yaw;
    this.patrol = o.patrol || [];
    this.maxHealth = this.type === 'archer' ? C.archerHealth : C.maxHealth;
    this.health = this.maxHealth;
    this.dead = false;

    this.state = 'patrol';
    this.awareness = 0;
    this.stateT = 0;
    this.seesPlayer = false;
    this.sinceSeen = 0;
    this.lastKnown = new THREE.Vector3();
    this.investigate = new THREE.Vector3();
    this.arrived = false;
    this.heightened = false;
    this.patrolIndex = 0;
    this.patrolWait = 0;
    this.attackCooldown = 0.5 + Math.random();
    this.arrowCooldown = 1 + Math.random();
    this.attacking = false; this.attackT = 0; this.attackHit = false;
    this.staggerUntil = 0;
    this.lastBlock = -100;
    this.path = null; this.pathGoal = null; this.repathT = 0;
    this.speed = 0;
    this.lookYaw = null;

    this.model = new CharacterModel({ robe: PALETTE.madder, sash: 0x3a2a1a, headgear: 'helmet', weapon: this.type === 'archer' ? 'bow' : 'sword' });
    if (this.type === 'archer') this.model.setWeapon('bow', 'bow');
    game.scene.add(this.model.root);
    this.sync(0);
  }

  get headPos() { return new THREE.Vector3(this.pos.x, this.pos.y + 1.7, this.pos.z); }
  get staggered() { return this.game.time < this.staggerUntil; }
  get telegraphing() { return this.attacking && this.attackT < C.attackWindup; }
  canBeTakenDown() { return !this.dead && (this.state !== 'combat' || this.staggered); }

  // ---- body ----------------------------------------------------------------

  takeDamage(amount, { attacker, melee }) {
    if (this.dead) return;
    const g = this.game;
    const aware = this.state === 'combat';
    if (melee && aware && this.type === 'sword' && !this.attacking && !this.staggered && attacker) {
      const to = attacker.pos.clone().sub(this.pos).setY(0).normalize();
      if (forward(this.yaw).dot(to) > 0.5 && Math.random() < C.blockChance) {
        this.lastBlock = g.time;
        g.audio.clash();
        return;
      }
    }
    // Staggered (parried) guards take double damage; unaware ones triple.
    const mult = this.staggered ? 2 : (aware ? 1 : 3);
    this.health -= amount * mult;
    g.audio.hit();
    if (this.health <= 0) { this.die(false); return; }
    if (attacker) this.lastKnown.copy(attacker.pos);
    if (this.state !== 'combat') this.enterCombat(true);
  }

  stagger(seconds) {
    this.attacking = false;
    this.staggerUntil = this.game.time + seconds;
  }

  die(takedown) {
    if (this.dead) return;
    const wasCombat = this.state === 'combat';
    this.dead = true;
    this.state = 'dead';
    this.attacking = false;
    this.game.onEnemyKilled(this, takedown);
    if (!takedown && !wasCombat) this.game.reportNoise(this.pos, NOISE.death, this);
  }

  startAttack() {
    if (this.attacking || this.staggered || this.dead) return;
    this.attacking = true; this.attackT = 0; this.attackHit = false;
  }

  shootAt(target) {
    const from = this.pos.clone().add(new THREE.Vector3(0, 1.5, 0)).addScaledVector(forward(this.yaw), 0.5);
    const aim = target.pos.clone().add(new THREE.Vector3(0, 1.1, 0));
    const speed = 38, gravity = 9.8 * 0.6;
    const t = from.distanceTo(aim) / speed;
    aim.y += 0.5 * gravity * t * t;
    const spread = Math.min(1.2, from.distanceTo(aim) * 0.03);
    aim.add(new THREE.Vector3((Math.random() - 0.5) * 2 * spread, (Math.random() - 0.5) * 2 * spread, (Math.random() - 0.5) * 2 * spread));
    this.game.arrows.fire(from, aim.sub(from).normalize().multiplyScalar(speed), C.arrowDamage, this, false);
    this.game.audio.bowRelease();
  }

  // ---- brain ---------------------------------------------------------------

  update(dt) {
    const g = this.game;
    if (this.dead || g.missionOver) { this.speed = 0; this.sync(dt); return; }
    this.stateT += dt;
    this.updateAttack(dt);
    this.perceive(dt);
    this.lookYaw = null;

    if (this.staggered) { this.speed = 0; this.sync(dt); return; }

    if (this.state === 'patrol') this.doPatrol(dt);
    else if (this.state === 'suspicious') this.doSuspicious(dt);
    else if (this.state === 'combat') this.doCombat(dt);
    else if (this.state === 'search') this.doSearch(dt);

    this.sync(dt);
  }

  canSeePlayer() {
    const p = this.game.player;
    if (!p || p.dead) return { seen: false };
    const eye = this.headPos;
    const target = p.pos.clone().add(new THREE.Vector3(0, p.height * 0.6, 0));
    const to = target.clone().sub(eye);
    const dist = to.length();
    const flat = to.clone().setY(0).normalize();
    const dot = forward(this.yaw).dot(flat);
    const range = this.type === 'archer' ? C.archerSightRange : C.sightRange;
    const close = dist <= C.closeSense && !p.crouched;
    if (!close) {
      if (dist > range || dot < C.viewConeCos) return { seen: false };
      if (p.hidden && dist > C.hidingRevealDistance) return { seen: false };
    }
    const w = this.game.world;
    const visible = w.hasLineOfSight(eye, p.headPos) || w.hasLineOfSight(eye, p.pos.clone().add(new THREE.Vector3(0, 0.3, 0)));
    return { seen: visible, dist, dot };
  }

  perceive(dt) {
    const g = this.game, p = g.player;
    const v = this.canSeePlayer();
    this.seesPlayer = v.seen;
    if (v.seen) {
      this.lastKnown.copy(p.pos);
      this.sinceSeen = 0;
      if (this.state !== 'combat') {
        const range = this.type === 'archer' ? C.archerSightRange : C.sightRange;
        const distF = THREE.MathUtils.lerp(3, 0.4, THREE.MathUtils.clamp(v.dist / range, 0, 1));
        const angF = THREE.MathUtils.lerp(0.5, 1, THREE.MathUtils.clamp((v.dot - C.viewConeCos) / (1 - C.viewConeCos), 0, 1));
        const alertF = (this.heightened || g.state !== 'infiltrate' ? 1.5 : 1) * (this.state !== 'patrol' ? 1.3 : 1);
        this.awareness = Math.min(1, this.awareness + C.gainRate * distF * angF * alertF * p.visibility() * dt);
      }
    } else {
      this.sinceSeen += dt;
      if (this.state === 'patrol' || this.state === 'suspicious') this.awareness = Math.max(0, this.awareness - C.decayRate * dt);
    }
    if (this.state !== 'combat' && this.awareness >= 1) this.enterCombat(true);
    else if (this.state === 'patrol' && this.awareness >= C.suspiciousThreshold) {
      this.investigate.copy(this.lastKnown);
      this.enter('suspicious');
    }
  }

  hearNoise(at) {
    if (this.dead || this.state === 'combat') return;
    this.awareness = Math.max(this.awareness, 0.5);
    this.investigate.copy(at);
    if (this.state === 'search') { this.arrived = false; this.stateT = 0; return; }
    this.enter('suspicious');
  }

  alertTo(at) {
    if (this.dead || this.state === 'combat') return;
    this.lastKnown.copy(at);
    this.enterCombat(false);
  }

  enter(state) {
    this.state = state;
    this.stateT = 0;
    this.arrived = false;
    this.path = null;
  }

  enterCombat(share) {
    const g = this.game;
    const was = this.state === 'combat';
    this.enter('combat');
    this.awareness = 1;
    this.heightened = true;
    this.sinceSeen = 0;
    if (was) return;
    g.onEnemyEnteredCombat(this);
    if (share) {
      for (const o of g.enemies) {
        if (o !== this && !o.dead && o.pos.distanceTo(this.pos) <= C.alertShareRadius) o.alertTo(this.lastKnown);
      }
    }
  }

  doPatrol(dt) {
    if (this.patrol.length === 0) {
      if (this.moveTo(this.home, 0.5, C.patrolSpeed, dt)) this.lookYaw = this.homeYaw;
      return;
    }
    const goal = this.patrol[this.patrolIndex % this.patrol.length];
    if (this.moveTo(goal, 0.8, C.patrolSpeed, dt)) {
      this.patrolWait += dt;
      if (this.patrolWait >= C.patrolWait) { this.patrolWait = 0; this.patrolIndex = (this.patrolIndex + 1) % this.patrol.length; }
    }
  }

  doSuspicious(dt) {
    this.lookYaw = yawTo(this.pos, this.investigate);
    if (this.stateT < 1.2) { this.speed = 0; return; } // "What was that?"
    if (!this.arrived) {
      if (this.moveTo(this.investigate, 1.2, C.patrolSpeed, dt) || this.stateT > 15) { this.arrived = true; this.stateT = 1.2; }
      return;
    }
    this.lookAround();
    if (this.stateT > 5 && this.awareness < C.suspiciousThreshold) {
      this.awareness = Math.min(this.awareness, 0.2);
      this.enter('patrol');
    }
  }

  doCombat(dt) {
    const g = this.game, p = g.player;
    if (!p || p.dead) { this.speed = 0; return; }
    if (this.sinceSeen > C.loseTargetTime) { this.investigate.copy(this.lastKnown); this.enter('search'); return; }
    const target = this.seesPlayer ? p.pos : this.lastKnown;
    this.lookYaw = yawTo(this.pos, target);
    const dist = Math.hypot(p.pos.x - this.pos.x, p.pos.z - this.pos.z);

    if (this.type === 'archer') {
      this.speed = 0;
      if (this.seesPlayer) {
        this.arrowCooldown -= dt;
        if (this.arrowCooldown <= 0) { this.shootAt(p); this.arrowCooldown = 2 + Math.random(); }
      }
      return;
    }

    if (dist > C.attackReach * 0.75 || !this.seesPlayer) this.moveTo(target, 1.1, this.attacking ? 0.6 : C.chaseSpeed, dt);
    else this.speed = 0;

    this.attackCooldown -= dt;
    const facing = Math.abs(angleDiff(this.yaw, yawTo(this.pos, p.pos))) < 0.8;
    if (this.seesPlayer && dist <= C.attackReach && facing && this.attackCooldown <= 0 && !this.attacking && g.canEnemyAttack()) {
      this.startAttack();
      this.attackCooldown = 1.4 + Math.random();
    }
  }

  doSearch(dt) {
    if (this.seesPlayer) this.awareness = Math.max(this.awareness, 0.8);
    if (!this.arrived) {
      this.lookYaw = yawTo(this.pos, this.investigate);
      if (this.moveTo(this.investigate, 1.2, C.chaseSpeed * 0.7, dt) || this.stateT > 15) { this.arrived = true; this.stateT = 0; }
      return;
    }
    this.lookAround();
    if (this.stateT > C.searchDuration) { this.awareness = 0.25; this.enter('patrol'); }
  }

  lookAround() {
    this.speed = 0;
    this.lookYaw = this.homeYaw + this.stateT * 1.2;
  }

  updateAttack(dt) {
    if (!this.attacking) return;
    this.attackT += dt;
    if (!this.attackHit && this.attackT >= C.attackWindup + 0.08) {
      this.attackHit = true;
      const p = this.game.player;
      const to = p.pos.clone().sub(this.pos);
      const flatDist = Math.hypot(to.x, to.z);
      if (flatDist <= C.attackReach && Math.abs(to.y) < 1.2 && forward(this.yaw).dot(to.setY(0).normalize()) > 0.4) {
        p.takeDamage(C.swordDamage, { attacker: this, melee: true });
      } else {
        this.game.audio.swing();
      }
    }
    if (this.attackT >= C.attackWindup + C.attackRecovery) this.attacking = false;
  }

  /** Walks toward goal along an A* path. Returns true when within `accept` metres. */
  moveTo(goal, accept, speed, dt) {
    const flat = Math.hypot(goal.x - this.pos.x, goal.z - this.pos.z);
    if (flat <= accept) { this.speed = 0; this.path = null; return true; }
    this.repathT -= dt;
    if (!this.path || !this.pathGoal || this.pathGoal.distanceTo(goal) > 1 || this.repathT <= 0) {
      this.path = this.game.world.findPath(this.pos, goal) || [goal.clone().setY(0)];
      this.pathGoal = goal.clone();
      this.repathT = this.state === 'combat' ? 0.5 : 3;
    }
    while (this.path.length > 1 && Math.hypot(this.path[0].x - this.pos.x, this.path[0].z - this.pos.z) < 0.4) this.path.shift();
    const wp = this.path[0];
    const dir = new THREE.Vector3(wp.x - this.pos.x, 0, wp.z - this.pos.z);
    const len = dir.length();
    if (len > 1e-4) dir.divideScalar(len);
    const step = Math.min(len, speed * dt);
    this.pos.addScaledVector(dir, step);
    this.speed = speed;
    if (this.lookYaw === null || this.state !== 'combat') this.moveYaw = Math.atan2(dir.x, dir.z);
    return false;
  }

  sync(dt) {
    if (!this.dead) {
      // Keep on the ground (archers stay on their tower top), and separate from others.
      const w = this.game.world;
      if (this.type !== 'archer') {
        w.resolveCollision(this.pos, this.radius, this.height);
        this.pos.y = w.groundHeight(this.pos.x, this.pos.z, this.pos.y + 0.3, this.radius);
      }
      const want = this.lookYaw !== null ? this.lookYaw : (this.speed > 0 && this.moveYaw !== undefined ? this.moveYaw : this.yaw);
      const d = angleDiff(this.yaw, want);
      this.yaw += THREE.MathUtils.clamp(d, -5 * dt, 5 * dt);
    }
    const m = this.model;
    m.root.position.copy(this.pos);
    m.root.rotation.y = this.yaw;
    m.animate(dt, {
      speed: this.speed, crouch: false,
      attack: this.attacking && this.attackT >= C.attackWindup ? Math.min(1, (this.attackT - C.attackWindup) / 0.4) : -1,
      telegraph: this.telegraphing, guard: this.type === 'sword' && this.state === 'combat' && !this.staggered,
      aim: this.type === 'archer' && this.state === 'combat', draw: this.type === 'archer' ? 1 - Math.min(1, this.arrowCooldown / 2) : 0,
      dead: this.dead, stagger: this.staggered,
    });
  }

  dispose() { this.game.scene.remove(this.model.root); }
}
