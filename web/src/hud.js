// HTML overlay HUD: bars, objective + marker, guard awareness icons,
// crosshair, prompts, messages, damage flash.

import * as THREE from '../vendor/three.module.min.js';

const $ = (id) => document.getElementById(id);

export class Hud {
  constructor(game) {
    this.game = game;
    this.el = {
      hp: $('hp-fill'), st: $('st-fill'), weapon: $('weapon'), stance: $('stance'),
      objective: $('objective-text'), marker: $('marker'), markerDist: $('marker-dist'),
      prompt: $('prompt'), toast: $('toast'), crosshair: $('crosshair'), draw: $('draw-fill'),
      indicators: $('indicators'), damage: $('damage'),
    };
    this.pool = [];
    this.toastUntil = 0;
  }

  toast(text, seconds = 1.6) {
    this.el.toast.textContent = text;
    this.el.toast.style.opacity = 1;
    this.toastUntil = performance.now() + seconds * 1000;
  }

  flashDamage() {
    this.el.damage.style.opacity = 0.55;
    setTimeout(() => { this.el.damage.style.opacity = 0; }, 120);
  }

  project(v) {
    const p = v.clone().project(this.game.camera);
    return { x: (p.x * 0.5 + 0.5) * innerWidth, y: (-p.y * 0.5 + 0.5) * innerHeight, behind: p.z > 1 };
  }

  update() {
    const g = this.game, p = g.player, e = this.el;
    e.hp.style.width = `${(p.health / 100) * 100}%`;
    e.st.style.width = `${(p.stamina / 100) * 100}%`;
    const touch = g.input.isTouch;
    e.weapon.textContent = p.weapon === 'sword'
      ? (touch ? 'SABRE — ATK attack · BLK block (just before a hit = parry)' : 'SABRE — LMB attack · hold RMB block (tap just before a hit = parry)')
      : (touch ? 'BOW — hold AIM, hold & release ATK' : 'BOW — hold RMB aim · hold & release LMB to shoot');
    e.stance.textContent = p.hidden ? 'HIDDEN' : (p.crouched ? 'Sneaking' : '');
    e.stance.className = p.hidden ? 'hiding' : '';
    e.objective.textContent = g.objectiveText();
    e.prompt.textContent = p.prompt;
    if (performance.now() > this.toastUntil) e.toast.style.opacity = 0;

    // Crosshair + draw power
    e.crosshair.style.display = p.aiming ? 'block' : 'none';
    e.draw.style.width = `${p.drawAlpha() * 100}%`;

    // Objective marker (clamped to screen edges)
    const target = g.objectivePos();
    if (target && !g.missionOver) {
      const s = this.project(target.clone().add(new THREE.Vector3(0, 2, 0)));
      let x = s.x, y = s.y;
      if (s.behind) { x = innerWidth - x; y = innerHeight - 40; }
      x = Math.min(innerWidth - 30, Math.max(30, x));
      y = Math.min(innerHeight - 30, Math.max(30, y));
      e.marker.style.display = 'block';
      e.marker.style.transform = `translate(${x}px, ${y}px)`;
      e.markerDist.textContent = `${Math.round(g.camera.position.distanceTo(target))} m`;
    } else {
      e.marker.style.display = 'none';
    }

    // Guard awareness indicators
    let used = 0;
    for (const en of g.enemies) {
      if (en.dead || (en.awareness <= 0.01 && en.state === 'patrol')) continue;
      const s = this.project(en.headPos.add(new THREE.Vector3(0, 0.55, 0)));
      if (s.behind || s.x < 0 || s.y < 0 || s.x > innerWidth || s.y > innerHeight) continue;
      const div = this.pool[used] || this.makeIndicator();
      used++;
      const combat = en.state === 'combat';
      let label = combat ? '!' : '?';
      if (en.state === 'search') label = '? searching';
      if (combat && en.telegraphing) label = '!! ATTACK';
      if (en.staggered) label = 'STAGGERED';
      if (g.time - en.lastBlock < 0.6) label = 'blocked';
      div.style.display = 'block';
      div.style.transform = `translate(${s.x}px, ${s.y}px)`;
      div.className = 'indicator' + (combat ? ' combat' : '') + (en.telegraphing ? ' attack' : '');
      div.firstChild.textContent = label;
      div.lastChild.firstChild.style.width = `${en.awareness * 100}%`;
    }
    for (let i = used; i < this.pool.length; i++) this.pool[i].style.display = 'none';
  }

  makeIndicator() {
    const div = document.createElement('div');
    div.className = 'indicator';
    div.innerHTML = '<span></span><div class="meter"><div></div></div>';
    this.el.indicators.appendChild(div);
    this.pool.push(div);
    return div;
  }
}
