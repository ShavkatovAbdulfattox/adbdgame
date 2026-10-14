// Keyboard + mouse (pointer lock), gamepad, and on-screen touch controls,
// all mapped to the same abstract actions.

const KEYMAP = {
  KeyW: 'up', ArrowUp: 'up', KeyS: 'down', ArrowDown: 'down',
  KeyA: 'left', ArrowLeft: 'left', KeyD: 'right', ArrowRight: 'right',
  ShiftLeft: 'sprint', ShiftRight: 'sprint',
  KeyC: 'crouch', ControlLeft: 'crouch',
  Space: 'jump',
  AltLeft: 'dodge', AltRight: 'dodge', KeyV: 'dodge',
  KeyE: 'interact', KeyF: 'interact',
  KeyQ: 'switch', Tab: 'switch',
  KeyP: 'pause', Escape: 'pause',
  Enter: 'confirm', Backspace: 'back',
};

export class Input {
  constructor(canvas) {
    this.canvas = canvas;
    this.held = new Set();
    this.pressedNow = new Set();
    this.releasedNow = new Set();
    this.look = { x: 0, y: 0 };
    this.touchMove = { x: 0, y: 0 };
    this.locked = false;
    this.isTouch = matchMedia('(pointer: coarse)').matches || 'ontouchstart' in window;
    this.onPointerLockLost = null;

    addEventListener('keydown', (e) => {
      const a = KEYMAP[e.code];
      if (!a) return;
      if (['Tab', 'Space', 'AltLeft', 'AltRight', 'Backspace'].includes(e.code) || e.code.startsWith('Arrow')) e.preventDefault();
      if (e.repeat) return;
      this.press(a);
    });
    addEventListener('keyup', (e) => { const a = KEYMAP[e.code]; if (a) this.release(a); });
    addEventListener('blur', () => { for (const a of [...this.held]) this.release(a); });

    canvas.addEventListener('mousedown', (e) => {
      if (!this.locked) return;
      if (e.button === 0) this.press('attack');
      if (e.button === 2) this.press('alt');
    });
    addEventListener('mouseup', (e) => {
      if (e.button === 0) this.release('attack');
      if (e.button === 2) this.release('alt');
    });
    addEventListener('contextmenu', (e) => e.preventDefault());
    addEventListener('mousemove', (e) => {
      if (!this.locked) return;
      this.look.x += e.movementX;
      this.look.y += e.movementY;
    });
    document.addEventListener('pointerlockchange', () => {
      const was = this.locked;
      this.locked = document.pointerLockElement === canvas;
      if (was && !this.locked && this.onPointerLockLost) this.onPointerLockLost();
    });

    if (this.isTouch) this.setupTouch();
  }

  lockPointer() {
    if (this.isTouch) return;
    try { this.canvas.requestPointerLock(); } catch (e) { /* ignore */ }
  }
  unlockPointer() { if (document.pointerLockElement) document.exitPointerLock(); }

  press(a) { if (!this.held.has(a)) { this.held.add(a); this.pressedNow.add(a); } }
  release(a) { if (this.held.has(a)) { this.held.delete(a); this.releasedNow.add(a); } }

  isHeld(a) { return this.held.has(a); }
  wasPressed(a) { return this.pressedNow.has(a); }
  wasReleased(a) { return this.releasedNow.has(a); }

  /** Movement axes in -1..1 (x = right, y = forward). */
  moveAxes() {
    let x = (this.isHeld('right') ? 1 : 0) - (this.isHeld('left') ? 1 : 0) + this.touchMove.x;
    let y = (this.isHeld('up') ? 1 : 0) - (this.isHeld('down') ? 1 : 0) + this.touchMove.y;
    const pad = this.pad;
    if (pad) {
      const dz = (v) => (Math.abs(v) < 0.18 ? 0 : v);
      x += dz(pad.axes[0] || 0);
      y -= dz(pad.axes[1] || 0);
    }
    const len = Math.hypot(x, y);
    if (len > 1) { x /= len; y /= len; }
    return { x, y };
  }

  /** Call once per frame before reading input. */
  pollGamepad(dt) {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    const pad = [...pads].find(p => p && p.connected);
    this.pad = pad || null;
    if (!pad) return;
    const map = { 0: 'jump', 1: 'dodge', 2: 'interact', 3: 'switch', 6: 'alt', 7: 'attack', 9: 'pause', 10: 'sprint', 11: 'crouch', 12: 'up', 13: 'down', 14: 'left', 15: 'right' };
    this.padHeld = this.padHeld || new Set();
    for (const [i, a] of Object.entries(map)) {
      const b = pad.buttons[i];
      const down = !!b && (b.pressed || b.value > 0.5);
      const key = 'pad' + i;
      if (down && !this.padHeld.has(key)) { this.padHeld.add(key); this.press(a); if (i === '0') this.press('confirm'); if (i === '1') this.press('back'); }
      if (!down && this.padHeld.has(key)) { this.padHeld.delete(key); this.release(a); if (i === '0') this.release('confirm'); if (i === '1') this.release('back'); }
    }
    const dz = (v) => (Math.abs(v) < 0.15 ? 0 : v);
    this.look.x += dz(pad.axes[2] || 0) * 900 * dt;
    this.look.y += dz(pad.axes[3] || 0) * 600 * dt;
  }

  /** Clears per-frame state. Call at the end of each frame. */
  endFrame() {
    this.pressedNow.clear();
    this.releasedNow.clear();
    this.look.x = 0;
    this.look.y = 0;
  }

  // ---- touch controls -------------------------------------------------------

  setupTouch() {
    document.body.classList.add('touch');
    const stick = document.getElementById('stick');
    const knob = document.getElementById('stick-knob');
    let stickId = null, lookId = null, sx = 0, sy = 0, lx = 0, ly = 0;

    const zone = document.getElementById('touch-zone');
    zone.addEventListener('touchstart', (e) => {
      for (const t of e.changedTouches) {
        if (t.clientX < innerWidth * 0.4 && stickId === null) {
          stickId = t.identifier; sx = t.clientX; sy = t.clientY;
          stick.style.left = `${sx - 60}px`; stick.style.top = `${sy - 60}px`; stick.style.display = 'block';
        } else if (lookId === null) {
          lookId = t.identifier; lx = t.clientX; ly = t.clientY;
        }
      }
      e.preventDefault();
    }, { passive: false });
    zone.addEventListener('touchmove', (e) => {
      for (const t of e.changedTouches) {
        if (t.identifier === stickId) {
          let dx = t.clientX - sx, dy = t.clientY - sy;
          const d = Math.hypot(dx, dy), max = 50;
          if (d > max) { dx = dx / d * max; dy = dy / d * max; }
          knob.style.transform = `translate(${dx}px, ${dy}px)`;
          this.touchMove.x = dx / max; this.touchMove.y = -dy / max;
          const sprint = d > max * 0.95;
          if (sprint) this.press('sprint'); else this.release('sprint');
        } else if (t.identifier === lookId) {
          this.look.x += (t.clientX - lx) * 2.2; this.look.y += (t.clientY - ly) * 2.2;
          lx = t.clientX; ly = t.clientY;
        }
      }
      e.preventDefault();
    }, { passive: false });
    const end = (e) => {
      for (const t of e.changedTouches) {
        if (t.identifier === stickId) {
          stickId = null; this.touchMove.x = this.touchMove.y = 0;
          knob.style.transform = ''; stick.style.display = 'none'; this.release('sprint');
        }
        if (t.identifier === lookId) lookId = null;
      }
    };
    zone.addEventListener('touchend', end);
    zone.addEventListener('touchcancel', end);

    for (const btn of document.querySelectorAll('[data-action]')) {
      const a = btn.dataset.action;
      btn.addEventListener('touchstart', (e) => { this.press(a); btn.classList.add('down'); e.preventDefault(); e.stopPropagation(); }, { passive: false });
      const up = (e) => { this.release(a); btn.classList.remove('down'); e.preventDefault(); };
      btn.addEventListener('touchend', up);
      btn.addEventListener('touchcancel', up);
    }
  }
}
