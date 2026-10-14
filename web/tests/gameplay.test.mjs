import { chromium } from 'playwright';
const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
const page = await browser.newPage({ viewport: { width: 960, height: 540 } });
const errors = [];
page.on('pageerror', e => errors.push('pageerror: ' + e.message));
page.on('console', m => { if (m.type() === 'error') errors.push('console: ' + m.text()); });
await page.goto('http://localhost:8765/index.html');
await page.waitForFunction(() => window.__game);
await page.click('#btn-play');

const results = await page.evaluate(async () => {
  const g = window.__game, P = window.__P;
  g.paused = true; // stop the render loop from stepping; we step manually
  const sim = (sec, each) => { const dt = 1 / 60; for (let t = 0; t < sec; t += dt) { if (each) each(); g.input.pollGamepad?.(0); g.step(dt); g.updateCamera(dt); g.input.endFrame(); } };
  const out = {};
  const reset = () => { g.state = 'infiltrate'; g.resetMission(); g.paused = true; };
  const place = (n, e, yawDeg) => { g.player.pos.copy(P(n, e)); g.player.yaw = g.player.camYaw = yawDeg * Math.PI / 180; };
  const guard = (i) => g.enemies[i];

  // 1. Standing in the open in front of the hall-door guard (static, faces south) → detected.
  reset(); place(10, 2.5, 0); sim(4);
  out.detectOpen = { state: guard(5).state, detected: g.stats.detected };

  // 2. Crouched in the hay hiding zone 6 m from the same guard's line → not detected.
  reset(); g.input.press('crouch'); sim(0.05); g.input.release('crouch');
  place(12, -6.5, 0); g.player.crouched = true; sim(4);
  out.hidden = { hiddenFlag: g.player.hidden, guard5: guard(5).state, anyCombat: g.enemies.some(e => e.state === 'combat') };

  // 3. Silent takedown from behind the static gate guard (faces south, so stand north of him).
  reset(); const gate = guard(4); place(-H(g) + 5.5, 0, 180); sim(0.1);
  const tdTarget = g.player.takedownTarget();
  g.input.press('interact'); sim(0.05); g.input.release('interact');
  out.takedown = { foundTarget: !!tdTarget, gateDead: gate.dead, takedowns: g.stats.takedowns };

  // 4. Sabre fight: aware guard, hit until dead (blocks are possible).
  reset(); const s = guard(2); s.alertTo(g.player.pos); place(15, -10.5, 0); s.pos.copy(P(15, -12)); s.yaw = 0;
  let swings = 0;
  for (let i = 0; i < 25 && !s.dead; i++) { g.player.stamina = 100; g.player.yaw = Math.atan2(s.pos.x - g.player.pos.x, s.pos.z - g.player.pos.z); g.player.camYaw = g.player.yaw; g.input.press('attack'); sim(0.05); g.input.release('attack'); sim(0.6); swings++; g.player.health = 100; }
  out.sabre = { guardDead: s.dead, swings };

  // 5. Parry: block right before an enemy hit lands → enemy staggered, no damage.
  reset(); const a = guard(1); place(0, -23, 180); a.pos.copy(P(1.5, -23)); a.yaw = 0; a.enterCombat(false);
  g.player.yaw = Math.PI; // face north toward him
  a.startAttack(); const hp0 = g.player.health;
  sim(0.45); g.input.press('alt'); sim(0.4); g.input.release('alt');
  out.parry = { staggered: a.staggered, healthLost: hp0 - g.player.health };

  // 6. Bow: shoot an unaware guard from close range.
  reset(); const b = guard(0); b.patrol = []; b.home.copy(P(-20, -18)); b.pos.copy(P(-20, -18));
  place(-20, -26, 90); g.player.camPitch = 0.05; sim(0.1);
  g.input.press('switch'); sim(0.05); g.input.release('switch');
  g.input.press('alt'); sim(0.2); g.input.press('attack'); sim(1.1); g.input.release('attack'); sim(1.0); g.input.release('alt');
  out.bow = { weapon: g.player.weapon, guardHealth: Math.round(b.health), dead: b.dead };

  // 7. Path-finding: alert a courtyard guard to a spot behind the east rooms; he must walk around obstacles.
  reset(); const c = guard(0); c.alertTo(P(-13, 34)); g.player.pos.copy(P(-60, 60)); // player far away, out of sight
  const startD = c.pos.distanceTo(P(-13, 33)); sim(12);
  out.path = { startDist: Math.round(startD), endDist: Math.round(c.pos.distanceTo(P(-13, 33))), state: c.state };

  // 8. Objective → escape → extraction.
  reset(); place(30.5, 0, 0); sim(0.1);
  const near = g.player.nearDispatch();
  g.input.press('interact'); sim(0.05); g.input.release('interact');
  const afterTake = g.state;
  g.player.pos.copy(g.world.extractionPos); sim(0.1);
  out.mission = { nearDispatch: near, afterTake, final: g.state, rank: g.rank() };

  // 9. Death → failed.
  reset(); g.player.takeDamage(500, { attacker: null, melee: false }); sim(0.1);
  out.death = { state: g.state };

  // 10. Collision: walk east into the west wall north of the breach; must not pass through.
  reset(); place(10, -40, 90); g.input.press('up'); sim(3); g.input.release('up');
  out.wall = { x: +g.player.pos.x.toFixed(2), stoppedBeforeWall: g.player.pos.x < -35.5 };

  // 11. Through the breach the player CAN enter.
  reset(); place(-5.5, -40, 90); g.input.press('up'); sim(3); g.input.release('up');
  out.breach = { x: +g.player.pos.x.toFixed(2), inside: g.player.pos.x > -34 };

  function H(g) { return g.world.H; }
  return out;
});
console.log(JSON.stringify(results, null, 1));
console.log('ERRORS', errors.length ? errors.join('\n') : 'none');
await browser.close();
