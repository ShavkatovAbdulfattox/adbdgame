// Bahram, the playable scout: movement, stamina, stealth, sabre (combo, block,
// parry), bow (aim, draw, release), dodge and silent takedowns.

import * as THREE from '../vendor/three.module.min.js';
import { PLAYER as C, NOISE, PALETTE } from './config.js';
import { CharacterModel, forward, yawTo } from './characters.js';

export const right = (yaw) => new THREE.Vector3(-Math.cos(yaw), 0, Math.sin(yaw));

export class Player {
  constructor(game) {
    this.game = game;
    this.isEnemy = false;
    this.radius = C.radius;
    this.model = new CharacterModel({ robe: 0x46597a, sash: PALETTE.turquoise, headgear: 'cap', weapon: 'both' });
    game.scene.add(this.model.root);
    this.reset();
  }

  reset() {
    const w = this.game.world;
    this.pos = w.playerStart.clone();
    this.velY = 0;
    this.yaw = Math.PI / 2;        // facing east, toward the breach
    this.camYaw = Math.PI / 2;
    this.camPitch = 0.25;
    this.health = C.maxHealth;
    this.stamina = C.maxStamina;
    this.sinceStamina = 10;
    this.weapon = 'sword';
    this.model.setWeapon('sword');
    this.crouched = false;
    this.exhausted = false;
    this.blocking = false; this.blockStart = -100;
    this.aiming = false; this.drawing = false; this.drawStart = 0;
    this.attacking = false; this.attackT = 0; this.attackHit = false; this.attackQueued = false; this.mirror = false;
    this.dodgeT = 0; this.dodgeDir = new THREE.Vector3();
    this.invulnerableUntil = 0;
    this.noiseTimer = 0;
    this.dead = false;
    this.onGround = true;
    this.speed = 0;
    this.prompt = '';
    this.model.deathT = -1;
    this.model.body.rotation.set(0, 0, 0);
    this.model.body.position.set(0, 0, 0);
  }

  get height() { return this.crouched ? C.crouchHeight : C.height; }
  get hidden() { return this.crouched && this.game.world.isInHidingZone(this.pos); }
  get headPos() { return new THREE.Vector3(this.pos.x, this.pos.y + this.height - 0.1, this.pos.z); }

  /** Multiplier used by guard vision: lower is harder to see. */
  visibility() {
    let v = 1;
    if (this.crouched) { v *= 0.5; if (this.game.world.isInHidingZone(this.pos)) v *= 0.5; }
    if (this.speed > C.runSpeed + 0.5) v *= 1.5;
    else if (this.speed < 0.1) v *= 0.8;
    return v;
  }

  spendStamina(n) {
    if (this.stamina < n) return false;
    this.stamina -= n; this.sinceStamina = 0;
    return true;
  }

  // -------------------------------------------------------------------------

  update(dt) {
    const g = this.game, input = g.input, s = g.settings;
    const blocked = this.dead || g.missionOver;

    // Camera look
    const sens = 0.0025 * s.sensitivity;
    this.camYaw -= input.look.x * sens;
    this.camPitch = THREE.MathUtils.clamp(this.camPitch + input.look.y * sens * (s.invertY ? -1 : 1), -0.6, 1.1);

    if (this.dead) { this.animate(dt); return; }

    // Buttons
    if (!blocked) {
      if (input.wasPressed('switch') && !this.attacking) this.switchWeapon();
      if (input.wasPressed('crouch')) { if (this.crouched) this.crouched = false; else if (this.onGround) this.crouched = true; }
      if (input.wasPressed('jump')) { if (this.crouched) this.crouched = false; else if (this.onGround) { this.velY = C.jumpVelocity; this.onGround = false; } }
      if (input.wasPressed('dodge')) this.tryDodge(input);
      if (input.wasPressed('interact')) this.interact();

      const alt = input.isHeld('alt');
      if (this.weapon === 'sword') {
        if (alt && !this.blocking && !this.attacking) { this.blocking = true; this.blockStart = g.time; }
        if (!alt) this.blocking = false;
        this.aiming = false;
        if (input.wasPressed('attack') && !this.blocking) {
          if (!this.attacking) this.startAttack();
          else if (this.attackT > C.attackDuration * 0.45) this.attackQueued = true;
        }
      } else {
        this.blocking = false;
        this.aiming = alt;
        if (input.wasPressed('attack')) {
          if (this.aiming) { this.drawing = true; this.drawStart = g.time; }
          else g.hud.toast('Hold right mouse to aim the bow');
        }
        if (input.wasReleased('attack') && this.drawing) {
          this.drawing = false;
          if (this.aiming) this.fireArrow();
        }
        if (!this.aiming) this.drawing = false;
      }
      if (input.isHeld('sprint') && this.crouched) this.crouched = false;
    }

    // Movement
    const axes = blocked ? { x: 0, y: 0 } : input.moveAxes();
    const wish = forward(this.camYaw).multiplyScalar(axes.y).add(right(this.camYaw).multiplyScalar(axes.x));
    const moving = wish.lengthSq() > 0.01;
    const wantsSprint = input.isHeld('sprint') && !this.exhausted && !this.aiming && !this.blocking && !this.crouched && moving;
    let speed = C.runSpeed;
    if (this.aiming || this.blocking || this.attacking) speed = C.combatSpeed;
    else if (this.crouched) speed = C.crouchSpeed;
    else if (wantsSprint) speed = C.sprintSpeed;

    let vel = wish.multiplyScalar(speed);
    if (this.dodgeT > 0) {
      this.dodgeT -= dt;
      vel = this.dodgeDir.clone().multiplyScalar(C.dodgeSpeed);
    }
    this.pos.addScaledVector(vel, dt);
    this.speed = vel.length();

    // Facing: strafe while aiming/blocking, otherwise turn toward movement.
    if (this.aiming || this.blocking) this.yaw = this.camYaw;
    else if (moving && !this.attacking && this.dodgeT <= 0) this.yaw = turnToward(this.yaw, Math.atan2(vel.x, vel.z), dt * 10);

    // Gravity, ground, collisions
    this.velY -= C.gravity * dt;
    this.pos.y += this.velY * dt;
    const ground = g.world.groundHeight(this.pos.x, this.pos.z, this.pos.y, this.radius);
    const wasAir = !this.onGround;
    if (this.pos.y <= ground) {
      if (wasAir && this.velY < -4) g.reportNoise(this.pos, NOISE.land, this);
      this.pos.y = ground; this.velY = 0; this.onGround = true;
    } else if (this.pos.y > ground + 0.05) {
      this.onGround = false;
    }
    g.world.resolveCollision(this.pos, this.radius, this.height);
    for (const e of g.enemies) {
      if (e.dead) continue;
      const dx = this.pos.x - e.pos.x, dz = this.pos.z - e.pos.z, d = Math.hypot(dx, dz), min = this.radius + e.radius;
      if (d < min && d > 1e-4) { this.pos.x = e.pos.x + dx / d * min; this.pos.z = e.pos.z + dz / d * min; }
    }

    // Stamina
    this.sinceStamina += dt;
    if (speed === C.sprintSpeed && moving) {
      this.stamina = Math.max(0, this.stamina - C.sprintStaminaPerSec * dt);
      this.sinceStamina = 0;
      if (this.stamina <= 0) this.exhausted = true;
    } else if (!this.blocking && this.sinceStamina > 0.8) {
      this.stamina = Math.min(C.maxStamina, this.stamina + C.staminaRegen * dt);
    }
    if (!input.isHeld('sprint')) this.exhausted = false;

    // Footstep noise (sneaking is silent)
    this.noiseTimer -= dt;
    if (this.noiseTimer <= 0 && this.onGround && this.speed > 0.5 && !this.crouched) {
      g.reportNoise(this.pos, this.speed > C.runSpeed + 0.5 ? NOISE.sprint : NOISE.run, this);
      g.audio.footstep();
      this.noiseTimer = this.speed > C.runSpeed + 0.5 ? 0.3 : 0.45;
    }

    this.updateAttack(dt);
    this.updatePrompt();
    this.animate(dt);
  }

  animate(dt) {
    const m = this.model;
    m.root.position.copy(this.pos);
    m.root.rotation.y = this.yaw;
    m.animate(dt, {
      speed: this.speed, crouch: this.crouched, attack: this.attacking ? this.attackT / C.attackDuration : -1,
      mirror: this.mirror, guard: this.blocking, aim: this.aiming, draw: this.drawAlpha(), dead: this.dead,
    });
  }

  drawAlpha() {
    return this.drawing ? Math.min(1, (this.game.time - this.drawStart) / C.fullDrawTime) : 0;
  }

  switchWeapon() {
    this.weapon = this.weapon === 'sword' ? 'bow' : 'sword';
    this.model.setWeapon(this.weapon);
    this.blocking = this.aiming = this.drawing = false;
    this.game.hud.toast(this.weapon === 'sword' ? 'Sabre drawn' : 'Bow ready');
  }

  tryDodge(input) {
    if (this.attacking || !this.onGround || this.dodgeT > 0) return;
    if (!this.spendStamina(C.dodgeCost)) { this.game.hud.toast('Too tired to dodge'); return; }
    const a = input.moveAxes();
    let dir = forward(this.camYaw).multiplyScalar(a.y).add(right(this.camYaw).multiplyScalar(a.x));
    if (dir.lengthSq() < 0.01) dir = forward(this.yaw).negate();
    this.dodgeDir.copy(dir.normalize());
    this.dodgeT = C.dodgeTime;
    this.crouched = false;
    this.invulnerableUntil = this.game.time + 0.35;
  }

  // ---- sabre ---------------------------------------------------------------

  startAttack() {
    if (!this.spendStamina(C.attackCost)) { this.game.hud.toast('Too tired to attack'); return; }
    this.attacking = true; this.attackT = 0; this.attackHit = false; this.attackQueued = false;
    this.mirror = !this.mirror;
    this.crouched = false;
    const target = this.softLockTarget(4);
    this.yaw = target ? yawTo(this.pos, target.pos) : this.camYaw;
    this.game.audio.swing();
  }

  softLockTarget(maxDist) {
    let best = null, bestScore = Infinity;
    const f = forward(this.camYaw);
    for (const e of this.game.enemies) {
      if (e.dead) continue;
      const to = e.pos.clone().sub(this.pos).setY(0), d = to.length();
      if (d > maxDist) continue;
      const facing = f.dot(to.normalize());
      if (facing < 0.2) continue;
      const score = d * (2 - facing);
      if (score < bestScore) { bestScore = score; best = e; }
    }
    return best;
  }

  updateAttack(dt) {
    if (!this.attacking) return;
    this.attackT += dt;
    const a = this.attackT / C.attackDuration;
    if (!this.attackHit && a >= 0.35) {
      this.attackHit = true;
      const f = forward(this.yaw);
      let hitAny = false;
      for (const e of this.game.enemies) {
        if (e.dead) continue;
        const to = e.pos.clone().sub(this.pos).setY(0);
        const d = to.length();
        if (d <= C.swordReach + 0.4 && f.dot(to.normalize()) > 0.3 && Math.abs(e.pos.y - this.pos.y) < 1.2) {
          e.takeDamage(C.swordDamage, { attacker: this, melee: true });
          hitAny = true;
        }
      }
      if (hitAny) this.game.reportNoise(this.pos, NOISE.combat, this);
    }
    if (this.attackT >= C.attackDuration) {
      this.attacking = false;
      if (this.attackQueued) this.startAttack();
    }
  }

  // ---- bow -----------------------------------------------------------------

  fireArrow() {
    const g = this.game;
    const power = THREE.MathUtils.clamp((g.time - this.drawStart) / C.fullDrawTime, 0.25, 1);
    // Aim at whatever is under the crosshair.
    const cam = g.camera;
    const dir = new THREE.Vector3();
    cam.getWorldDirection(dir);
    const far = cam.position.clone().addScaledVector(dir, 80);
    const hit = g.world.raycast(cam.position, far);
    const aimPoint = hit === Infinity ? far : cam.position.clone().addScaledVector(dir, hit);
    const from = this.pos.clone().add(new THREE.Vector3(0, 1.45, 0)).addScaledVector(forward(this.yaw), 0.6).addScaledVector(right(this.yaw), 0.15);
    const speed = THREE.MathUtils.lerp(25, 60, power);
    const v = aimPoint.clone().sub(from).normalize().multiplyScalar(speed);
    g.arrows.fire(from, v, THREE.MathUtils.lerp(C.arrowMinDamage, C.arrowMaxDamage, power), this, true);
    g.audio.bowRelease();
  }

  // ---- interaction ---------------------------------------------------------

  takedownTarget() {
    for (const e of this.game.enemies) {
      if (!e.canBeTakenDown()) continue;
      const toPlayer = this.pos.clone().sub(e.pos);
      if (Math.abs(toPlayer.y) > 1) continue;
      toPlayer.y = 0;
      if (toPlayer.length() > C.takedownRange) continue;
      if (forward(e.yaw).dot(toPlayer.normalize()) < -0.3) return e; // behind him
    }
    return null;
  }

  nearDispatch() {
    const g = this.game;
    const d = g.world.dispatchPos;
    return g.state === 'infiltrate' && g.dispatch.visible && Math.hypot(this.pos.x - d.x, this.pos.z - d.z) < 1.8;
  }

  interact() {
    const g = this.game;
    if (this.nearDispatch()) { g.takeDispatch(); return; }
    const t = this.takedownTarget();
    if (t) {
      this.yaw = yawTo(this.pos, t.pos);
      t.die(true);
      g.hud.toast('Silent takedown');
    }
  }

  updatePrompt() {
    const touch = this.game.input.isTouch;
    if (this.nearDispatch()) this.prompt = touch ? 'Tap E — take the sealed dispatch' : '[E] Take the sealed dispatch';
    else if (this.takedownTarget()) this.prompt = touch ? 'Tap E — silent takedown' : '[E] Silent takedown';
    else this.prompt = '';
  }

  // ---- damage --------------------------------------------------------------

  takeDamage(amount, { attacker, melee }) {
    const g = this.game;
    if (this.dead || g.time < this.invulnerableUntil || g.missionOver) return;
    if (melee && attacker && this.blocking) {
      const toAttacker = attacker.pos.clone().sub(this.pos).setY(0).normalize();
      if (forward(this.yaw).dot(toAttacker) > 0.3) {
        if (g.time - this.blockStart <= C.parryWindow) {
          attacker.stagger(1.6);
          g.hud.toast('PARRY! Strike now');
          g.audio.clash();
          return;
        }
        if (this.spendStamina(C.blockCost)) { g.hud.toast('Blocked'); g.audio.clash(); return; }
        g.hud.toast('Guard broken!');
        this.blocking = false;
        amount *= 0.5;
      }
    }
    this.health = Math.max(0, this.health - amount);
    g.audio.hit();
    g.hud.flashDamage();
    if (this.health <= 0) {
      this.dead = true;
      this.attacking = this.blocking = this.aiming = false;
      g.onPlayerDied();
    }
  }
}

function turnToward(current, target, maxStep) {
  const d = Math.atan2(Math.sin(target - current), Math.cos(target - current));
  return current + THREE.MathUtils.clamp(d, -maxStep, maxStep);
}
