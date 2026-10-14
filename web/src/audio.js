// All sound is synthesised with the Web Audio API — no audio files.
// Steppe wind with gusts, plus short effects for swords, bows, hits and alerts.

export class Audio {
  constructor() {
    this.ctx = null;
    this.volume = 0.8;
  }

  /** Must be called from a user gesture (browser autoplay rules). */
  start() {
    if (this.ctx) { this.ctx.resume(); return; }
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    this.ctx = new AC();
    this.master = this.ctx.createGain();
    this.master.gain.value = this.volume;
    this.master.connect(this.ctx.destination);

    // Noise buffer shared by wind and effects.
    const len = this.ctx.sampleRate * 3;
    this.noise = this.ctx.createBuffer(1, len, this.ctx.sampleRate);
    const d = this.noise.getChannelData(0);
    for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;

    // Wind: looping noise -> low-pass whose cutoff and level follow slow gusts.
    const src = this.ctx.createBufferSource();
    src.buffer = this.noise; src.loop = true;
    this.windFilter = this.ctx.createBiquadFilter();
    this.windFilter.type = 'lowpass'; this.windFilter.frequency.value = 400; this.windFilter.Q.value = 0.7;
    this.windGain = this.ctx.createGain(); this.windGain.gain.value = 0.15;
    src.connect(this.windFilter).connect(this.windGain).connect(this.master);
    src.start();
    this.gust = 0.5; this.gustTarget = 0.5; this.gustTimer = 0;
  }

  setVolume(v) {
    this.volume = v;
    if (this.master) this.master.gain.value = v;
  }

  update(dt) {
    if (!this.ctx) return;
    this.gustTimer -= dt;
    if (this.gustTimer <= 0) { this.gustTimer = 2.5 + Math.random() * 4.5; this.gustTarget = 0.25 + Math.random() * 0.75; }
    this.gust += (this.gustTarget - this.gust) * Math.min(1, dt * 0.6);
    const t = this.ctx.currentTime;
    this.windFilter.frequency.setTargetAtTime(200 + 900 * this.gust, t, 0.3);
    this.windGain.gain.setTargetAtTime(0.06 + 0.22 * this.gust, t, 0.3);
  }

  noiseBurst({ dur = 0.2, type = 'bandpass', f0 = 800, f1 = 2000, q = 1, gain = 0.4 }) {
    if (!this.ctx) return;
    const t = this.ctx.currentTime;
    const src = this.ctx.createBufferSource(); src.buffer = this.noise;
    const f = this.ctx.createBiquadFilter(); f.type = type; f.Q.value = q;
    f.frequency.setValueAtTime(f0, t); f.frequency.exponentialRampToValueAtTime(f1, t + dur);
    const g = this.ctx.createGain();
    g.gain.setValueAtTime(0.0001, t); g.gain.exponentialRampToValueAtTime(gain, t + 0.02); g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    src.connect(f).connect(g).connect(this.master);
    src.start(t, Math.random() * 2); src.stop(t + dur + 0.05);
  }

  tone({ freq = 440, dur = 0.3, type = 'sine', gain = 0.2, slide = 1 }) {
    if (!this.ctx) return;
    const t = this.ctx.currentTime;
    const o = this.ctx.createOscillator(); o.type = type;
    o.frequency.setValueAtTime(freq, t); o.frequency.exponentialRampToValueAtTime(freq * slide, t + dur);
    const g = this.ctx.createGain();
    g.gain.setValueAtTime(gain, t); g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g).connect(this.master);
    o.start(t); o.stop(t + dur + 0.05);
  }

  swing() { this.noiseBurst({ dur: 0.22, f0: 600, f1: 2400, q: 2, gain: 0.25 }); }
  clash() { [1320, 1870, 2650].forEach((f, i) => this.tone({ freq: f, dur: 0.5 - i * 0.1, type: 'triangle', gain: 0.08 })); this.noiseBurst({ dur: 0.08, type: 'highpass', f0: 3000, f1: 5000, gain: 0.2 }); }
  hit() { this.tone({ freq: 140, dur: 0.15, type: 'sine', gain: 0.35, slide: 0.5 }); this.noiseBurst({ dur: 0.1, type: 'lowpass', f0: 900, f1: 300, gain: 0.3 }); }
  bowRelease() { this.tone({ freq: 220, dur: 0.18, type: 'triangle', gain: 0.18, slide: 0.7 }); this.noiseBurst({ dur: 0.25, f0: 3000, f1: 1200, q: 3, gain: 0.12 }); }
  arrowImpact() { this.noiseBurst({ dur: 0.1, type: 'lowpass', f0: 1500, f1: 400, gain: 0.25 }); }
  alert() { this.tone({ freq: 520, dur: 0.18, type: 'square', gain: 0.06 }); setTimeout(() => this.tone({ freq: 780, dur: 0.25, type: 'square', gain: 0.06 }), 140); }
  pickup() { this.tone({ freq: 660, dur: 0.3, type: 'sine', gain: 0.15, slide: 1.5 }); }
  footstep() { this.noiseBurst({ dur: 0.07, type: 'lowpass', f0: 500, f1: 200, gain: 0.08 }); }
}
