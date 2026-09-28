/**
 * ══════════════════════════════════════════════════════════════════
 *  AeroHome — Neumorphic Smart Home Control Engine
 *  Tactile Component Logic, Rotary Dimmer, & Responsive State
 * ══════════════════════════════════════════════════════════════════
 */

// Application State
const SmartState = {
  unit: 'F', // 'F' or 'C'
  temperatureF: 72,
  temperatureC: 22,
  hvacMode: 'cool',
  dimmerLevel: 75,
  isDimmerDragging: false,
  
  // Media Player State
  isPlaying: false,
  trackIndex: 0,
  tracks: [
    { title: 'Midnight City', artist: "M83 • Hurry Up, We're Dreaming", duration: 244 },
    { title: 'Resonance', artist: 'HOME • Odyssey', duration: 212 },
    { title: 'Get Lucky', artist: 'Daft Punk • RAM', duration: 248 },
    { title: 'Starboy', artist: 'The Weeknd • Starboy', duration: 230 }
  ],
  trackCurrentSec: 102,
  trackInterval: null,
  volume: 65,

  // Devices & Blinds
  blindsPercent: 80,
  isLocked: true,

  // Active Scene
  activeScene: 'cinema',
  activeRoom: 'all'
};

// DOM References
const Dom = {
  toastContainer: document.getElementById('toast-container'),
  activeCount: document.getElementById('txt-active-count'),

  // Climate
  txtTemperature: document.getElementById('txt-temperature'),
  txtClimateMode: document.getElementById('txt-climate-mode'),
  btnTempMinus: document.getElementById('btn-temp-minus'),
  btnTempPlus: document.getElementById('btn-temp-plus'),
  btnDegF: document.getElementById('btn-deg-f'),
  btnDegC: document.getElementById('btn-deg-c'),
  tempSliderTrack: document.getElementById('temp-slider-track'),
  tempSliderFill: document.getElementById('temp-slider-fill'),
  tempSliderThumb: document.getElementById('temp-slider-thumb'),
  hvacChips: document.querySelectorAll('.neu-chip-btn'),

  // Lighting & Dimmer
  lightToggles: document.querySelectorAll('.light-switch-stack .toggle-input'),
  btnLightsAll: document.getElementById('btn-lights-all'),
  rotaryDial: document.getElementById('rotary-dial'),
  rotaryKnob: document.getElementById('rotary-knob'),
  dialGlowArc: document.getElementById('dial-glow-arc'),
  knobNotch: document.getElementById('knob-notch'),
  valDimPct: document.getElementById('val-dim-pct'),
  dimPresets: document.querySelectorAll('.neu-preset-btn'),

  // Devices
  toggleTv: document.getElementById('toggle-tv'),
  toggleLock: document.getElementById('toggle-lock'),
  txtLockStatus: document.getElementById('txt-lock-status'),
  iconLock: document.getElementById('icon-lock'),
  btnBlindsDown: document.getElementById('btn-blinds-down'),
  btnBlindsUp: document.getElementById('btn-blinds-up'),
  txtBlindsStatus: document.getElementById('txt-blinds-status'),

  // Music Player
  btnPlayPause: document.getElementById('btn-play-pause'),
  iconPlayState: document.getElementById('icon-play-state'),
  vinylDisc: document.getElementById('vinyl-disc'),
  trackTitle: document.getElementById('track-title'),
  trackArtist: document.getElementById('track-artist'),
  txtTimeCur: document.getElementById('txt-time-cur'),
  txtTimeDur: document.getElementById('txt-time-dur'),
  musicProgressBar: document.getElementById('music-progress-bar'),
  musicProgressFill: document.getElementById('music-progress-fill'),
  btnPrevTrack: document.getElementById('btn-prev-track'),
  btnNextTrack: document.getElementById('btn-next-track'),
  volSliderTrack: document.getElementById('vol-slider-track'),
  volSliderFill: document.getElementById('vol-slider-fill'),
  volSliderThumb: document.getElementById('vol-slider-thumb'),

  // Scenes & Navigation
  sceneTiles: document.querySelectorAll('.neu-scene-tile'),
  modePills: document.querySelectorAll('.neu-pill-btn'),
  dockBtns: document.querySelectorAll('.dock-btn'),
  btnMasterPower: document.getElementById('btn-master-power'),
  btnNotifications: document.getElementById('btn-notifications')
};

/* ══════════════════════════════════════════
 *  Tactile Toast Notifications
 * ══════════════════════════════════════════ */
function notify(message) {
  if (!Dom.toastContainer) return;
  const toast = document.createElement('div');
  toast.className = 'toast';
  toast.textContent = message;
  Dom.toastContainer.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(-12px) scale(0.95)';
    toast.style.transition = 'all 0.25s ease';
    setTimeout(() => toast.remove(), 250);
  }, 2800);
}

/* ══════════════════════════════════════════
 *  Active Device Counter
 * ══════════════════════════════════════════ */
function refreshActiveDeviceCount() {
  const allToggles = document.querySelectorAll('.toggle-input:checked');
  let count = allToggles.length;
  if (SmartState.isPlaying) count += 1;
  Dom.activeCount.textContent = `${count} Devices Active`;
}

/* ══════════════════════════════════════════
 *  Climate & Thermostat Logic
 * ══════════════════════════════════════════ */
function updateTemperatureUI() {
  const isF = SmartState.unit === 'F';
  const val = isF ? SmartState.temperatureF : SmartState.temperatureC;
  Dom.txtTemperature.textContent = val;

  const modeName = SmartState.hvacMode.charAt(0).toUpperCase() + SmartState.hvacMode.slice(1);
  Dom.txtClimateMode.textContent = `${modeName}ing to ${val}°${SmartState.unit}`;

  // Update slider position (F: 60 to 85, C: 15 to 30)
  const min = isF ? 60 : 15;
  const max = isF ? 85 : 30;
  const ratio = Math.max(0, Math.min(1, (val - min) / (max - min)));
  const pct = Math.round(ratio * 100);

  Dom.tempSliderFill.style.width = `${pct}%`;
  Dom.tempSliderThumb.style.left = `${pct}%`;
}

Dom.btnTempMinus.addEventListener('click', () => {
  if (SmartState.unit === 'F') {
    if (SmartState.temperatureF > 60) SmartState.temperatureF--;
  } else {
    if (SmartState.temperatureC > 15) SmartState.temperatureC--;
  }
  updateTemperatureUI();
});

Dom.btnTempPlus.addEventListener('click', () => {
  if (SmartState.unit === 'F') {
    if (SmartState.temperatureF < 85) SmartState.temperatureF++;
  } else {
    if (SmartState.temperatureC < 30) SmartState.temperatureC++;
  }
  updateTemperatureUI();
});

Dom.btnDegF.addEventListener('click', () => {
  if (SmartState.unit !== 'F') {
    SmartState.unit = 'F';
    Dom.btnDegF.classList.add('active');
    Dom.btnDegC.classList.remove('active');
    updateTemperatureUI();
    notify('Temperature scale set to Fahrenheit (°F)');
  }
});

Dom.btnDegC.addEventListener('click', () => {
  if (SmartState.unit !== 'C') {
    SmartState.unit = 'C';
    Dom.btnDegC.classList.add('active');
    Dom.btnDegF.classList.remove('active');
    updateTemperatureUI();
    notify('Temperature scale set to Celsius (°C)');
  }
});

// Interactive Temperature Slider Click/Drag
function handleTempSlider(e) {
  const rect = Dom.tempSliderTrack.getBoundingClientRect();
  const clickX = (e.clientX !== undefined ? e.clientX : (e.touches ? e.touches[0].clientX : 0)) - rect.left;
  const ratio = Math.max(0, Math.min(1, clickX / rect.width));

  const isF = SmartState.unit === 'F';
  const min = isF ? 60 : 15;
  const max = isF ? 85 : 30;
  const target = Math.round(min + ratio * (max - min));

  if (isF) SmartState.temperatureF = target;
  else SmartState.temperatureC = target;

  updateTemperatureUI();
}

Dom.tempSliderTrack.addEventListener('click', handleTempSlider);

// HVAC Mode Buttons
Dom.hvacChips.forEach(chip => {
  chip.addEventListener('click', () => {
    Dom.hvacChips.forEach(c => c.classList.remove('active'));
    chip.classList.add('active');
    SmartState.hvacMode = chip.getAttribute('data-hvac');
    updateTemperatureUI();
    notify(`HVAC mode switched to ${SmartState.hvacMode.toUpperCase()}`);
  });
});

/* ══════════════════════════════════════════
 *  Rotary Dimmer Dial (SVG Arc & Knob)
 * ══════════════════════════════════════════ */
function setDimmer(percentage, updatePresets = true) {
  const clamped = Math.max(0, Math.min(100, Math.round(percentage)));
  SmartState.dimmerLevel = clamped;
  Dom.valDimPct.textContent = `${clamped}%`;

  // Circumference of r=62 circle is 2 * PI * 62 = 389.5
  const totalArc = 389.5;
  const offset = totalArc - (clamped / 100) * totalArc;
  Dom.dialGlowArc.style.strokeDashoffset = offset;

  // Rotate knob notch (0% = -135deg, 100% = +135deg)
  const deg = -135 + (clamped / 100) * 270;
  Dom.knobNotch.style.transform = `translateX(-50%) rotate(${deg}deg)`;
  Dom.knobNotch.style.transformOrigin = 'center 46px';

  if (updatePresets) {
    Dom.dimPresets.forEach(btn => {
      const p = parseInt(btn.getAttribute('data-dim'), 10);
      btn.classList.toggle('active', p === clamped);
    });
  }
}

// Preset buttons
Dom.dimPresets.forEach(btn => {
  btn.addEventListener('click', () => {
    const p = parseInt(btn.getAttribute('data-dim'), 10);
    setDimmer(p, true);
    notify(`Master dimmer set to ${p}%`);
  });
});

// Rotary Dial Drag/Click Handling
function handleRotaryMove(e) {
  const rect = Dom.rotaryDial.getBoundingClientRect();
  const centerX = rect.left + rect.width / 2;
  const centerY = rect.top + rect.height / 2;

  const clientX = e.clientX !== undefined ? e.clientX : (e.touches ? e.touches[0].clientX : 0);
  const clientY = e.clientY !== undefined ? e.clientY : (e.touches ? e.touches[0].clientY : 0);

  const dx = clientX - centerX;
  const dy = clientY - centerY;

  // Angle from top (12 o'clock = 0 deg)
  let angleDeg = Math.atan2(dy, dx) * (180 / Math.PI) + 90;
  if (angleDeg < 0) angleDeg += 360;

  // Map 0 - 360 to percentage
  const pct = (angleDeg / 360) * 100;
  setDimmer(pct, true);
}

Dom.rotaryDial.addEventListener('mousedown', (e) => {
  SmartState.isDimmerDragging = true;
  handleRotaryMove(e);
});

window.addEventListener('mousemove', (e) => {
  if (SmartState.isDimmerDragging) handleRotaryMove(e);
});

window.addEventListener('mouseup', () => {
  if (SmartState.isDimmerDragging) {
    SmartState.isDimmerDragging = false;
    notify(`Lighting brightness set to ${SmartState.dimmerLevel}%`);
  }
});

// Touch support for rotary dial
Dom.rotaryDial.addEventListener('touchstart', (e) => {
  SmartState.isDimmerDragging = true;
  handleRotaryMove(e);
}, { passive: true });

window.addEventListener('touchmove', (e) => {
  if (SmartState.isDimmerDragging) handleRotaryMove(e);
}, { passive: true });

window.addEventListener('touchend', () => {
  SmartState.isDimmerDragging = false;
});

// Master Lights All Off Button
Dom.btnLightsAll.addEventListener('click', () => {
  Dom.lightToggles.forEach(toggle => {
    toggle.checked = false;
  });
  setDimmer(0, true);
  refreshActiveDeviceCount();
  notify('All luminaire zones powered down');
});

// Light Toggles event
Dom.lightToggles.forEach(toggle => {
  toggle.addEventListener('change', () => {
    refreshActiveDeviceCount();
  });
});

/* ══════════════════════════════════════════
 *  Smart Devices & Motorized Blinds
 * ══════════════════════════════════════════ */
Dom.toggleTv.addEventListener('change', (e) => {
  refreshActiveDeviceCount();
  notify(`Apple TV 4K ${e.target.checked ? 'powered on' : 'put to sleep'}`);
});

Dom.toggleLock.addEventListener('change', (e) => {
  SmartState.isLocked = e.target.checked;
  Dom.txtLockStatus.textContent = SmartState.isLocked ? 'Secured • Battery 94%' : 'Unlocked • Open';
  Dom.iconLock.classList.toggle('active', SmartState.isLocked);
  refreshActiveDeviceCount();
  notify(`Front door ${SmartState.isLocked ? 'deadbolt locked' : 'unlocked'}`);
});

Dom.btnBlindsUp.addEventListener('click', () => {
  SmartState.blindsPercent = Math.min(100, SmartState.blindsPercent + 20);
  Dom.txtBlindsStatus.textContent = `Open ${SmartState.blindsPercent}%`;
  notify(`Motorized blinds raised to ${SmartState.blindsPercent}%`);
});

Dom.btnBlindsDown.addEventListener('click', () => {
  SmartState.blindsPercent = Math.max(0, SmartState.blindsPercent - 20);
  Dom.txtBlindsStatus.textContent = SmartState.blindsPercent === 0 ? 'Closed 0%' : `Open ${SmartState.blindsPercent}%`;
  notify(`Motorized blinds lowered to ${SmartState.blindsPercent}%`);
});

/* ══════════════════════════════════════════
 *  Media & Music Player Logic
 * ══════════════════════════════════════════ */
function formatTime(totalSec) {
  const m = Math.floor(totalSec / 60);
  const s = totalSec % 60;
  return `${m}:${s < 10 ? '0' : ''}${s}`;
}

function updateTrackDisplay() {
  const cur = SmartState.tracks[SmartState.trackIndex];
  Dom.trackTitle.textContent = cur.title;
  Dom.trackArtist.textContent = cur.artist;
  Dom.txtTimeDur.textContent = formatTime(cur.duration);
  Dom.txtTimeCur.textContent = formatTime(SmartState.trackCurrentSec);

  const pct = (SmartState.trackCurrentSec / cur.duration) * 100;
  Dom.musicProgressFill.style.width = `${pct}%`;
}

function setPlayback(play) {
  SmartState.isPlaying = play;
  Dom.vinylDisc.classList.toggle('playing', play);

  if (play) {
    Dom.iconPlayState.innerHTML = '<rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/>';
    Dom.btnPlayPause.style.color = '#fff';
    Dom.btnPlayPause.style.boxShadow = 'inset 3px 3px 6px rgba(0,0,0,0.8), inset -2px -2px 5px rgba(255,255,255,0.06)';

    if (!SmartState.trackInterval) {
      SmartState.trackInterval = setInterval(() => {
        const cur = SmartState.tracks[SmartState.trackIndex];
        SmartState.trackCurrentSec++;
        if (SmartState.trackCurrentSec >= cur.duration) {
          SmartState.trackCurrentSec = 0;
          SmartState.trackIndex = (SmartState.trackIndex + 1) % SmartState.tracks.length;
        }
        updateTrackDisplay();
      }, 1000);
    }
    notify(`Playing "${SmartState.tracks[SmartState.trackIndex].title}"`);
  } else {
    Dom.iconPlayState.innerHTML = '<polygon points="5 3 19 12 5 21 5 3"/>';
    Dom.btnPlayPause.style.color = 'var(--cyan-primary)';
    Dom.btnPlayPause.style.boxShadow = 'var(--shadow-extruded)';

    clearInterval(SmartState.trackInterval);
    SmartState.trackInterval = null;
    notify('Audio playback paused');
  }

  refreshActiveDeviceCount();
}

Dom.btnPlayPause.addEventListener('click', () => {
  setPlayback(!SmartState.isPlaying);
});

Dom.btnNextTrack.addEventListener('click', () => {
  SmartState.trackIndex = (SmartState.trackIndex + 1) % SmartState.tracks.length;
  SmartState.trackCurrentSec = 0;
  updateTrackDisplay();
  if (SmartState.isPlaying) notify(`Track skipped: "${SmartState.tracks[SmartState.trackIndex].title}"`);
});

Dom.btnPrevTrack.addEventListener('click', () => {
  SmartState.trackIndex = (SmartState.trackIndex - 1 + SmartState.tracks.length) % SmartState.tracks.length;
  SmartState.trackCurrentSec = 0;
  updateTrackDisplay();
  if (SmartState.isPlaying) notify(`Previous track: "${SmartState.tracks[SmartState.trackIndex].title}"`);
});

// Interactive Music Progress Click
Dom.musicProgressBar.addEventListener('click', (e) => {
  const rect = Dom.musicProgressBar.getBoundingClientRect();
  const ratio = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
  const cur = SmartState.tracks[SmartState.trackIndex];
  SmartState.trackCurrentSec = Math.round(ratio * cur.duration);
  updateTrackDisplay();
});

// Volume Slider
Dom.volSliderTrack.addEventListener('click', (e) => {
  const rect = Dom.volSliderTrack.getBoundingClientRect();
  const ratio = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
  const pct = Math.round(ratio * 100);
  SmartState.volume = pct;
  Dom.volSliderFill.style.width = `${pct}%`;
  Dom.volSliderThumb.style.left = `${pct}%`;
  notify(`Volume level: ${pct}%`);
});

/* ══════════════════════════════════════════
 *  Ambience Scenes
 * ══════════════════════════════════════════ */
const Scenes = {
  cinema: {
    name: 'Cinema',
    dimmer: 20,
    tempF: 70,
    livingLight: true,
    kitchenLight: false,
    bedLight: false,
    patioLight: false
  },
  focus: {
    name: 'Focus',
    dimmer: 85,
    tempF: 72,
    livingLight: true,
    kitchenLight: true,
    bedLight: false,
    patioLight: false
  },
  relax: {
    name: 'Relax',
    dimmer: 40,
    tempF: 74,
    livingLight: true,
    kitchenLight: false,
    bedLight: true,
    patioLight: true
  },
  bedtime: {
    name: 'Bedtime',
    dimmer: 0,
    tempF: 68,
    livingLight: false,
    kitchenLight: false,
    bedLight: false,
    patioLight: false
  }
};

Dom.sceneTiles.forEach(tile => {
  tile.addEventListener('click', () => {
    Dom.sceneTiles.forEach(t => t.classList.remove('active'));
    tile.classList.add('active');

    const scKey = tile.getAttribute('data-scene-trigger');
    const sc = Scenes[scKey];
    if (sc) {
      setDimmer(sc.dimmer, true);
      SmartState.temperatureF = sc.tempF;
      updateTemperatureUI();

      document.getElementById('toggle-light-living').checked = sc.livingLight;
      document.getElementById('toggle-light-kitchen').checked = sc.kitchenLight;
      document.getElementById('toggle-light-bed').checked = sc.bedLight;
      document.getElementById('toggle-light-patio').checked = sc.patioLight;

      if (scKey === 'bedtime') {
        Dom.toggleLock.checked = true;
        SmartState.isLocked = true;
        Dom.txtLockStatus.textContent = 'Secured • Battery 94%';
      }

      refreshActiveDeviceCount();
      notify(`Scene activated: ${sc.name} Ambience`);
    }
  });
});

/* ══════════════════════════════════════════
 *  Mode Pills (Home / Away / Night)
 * ══════════════════════════════════════════ */
Dom.modePills.forEach(pill => {
  pill.addEventListener('click', () => {
    Dom.modePills.forEach(p => p.classList.remove('active'));
    pill.classList.add('active');
    const sc = pill.getAttribute('data-scene');
    notify(`House operating mode: ${sc.toUpperCase()}`);
  });
});

/* ══════════════════════════════════════════
 *  Dock Navigation
 * ══════════════════════════════════════════ */
Dom.dockBtns.forEach(btn => {
  btn.addEventListener('click', () => {
    Dom.dockBtns.forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    const room = btn.getAttribute('data-room');
    SmartState.activeRoom = room;
    notify(`Switched view to ${room.toUpperCase()} room`);
  });
});

Dom.btnMasterPower.addEventListener('click', () => {
  notify('Master power check: All zones nominal');
});

Dom.btnNotifications.addEventListener('click', () => {
  notify('System notifications: No warnings or intrusions');
});

/* ══════════════════════════════════════════
 *  Boot Initialization
 * ══════════════════════════════════════════ */
window.addEventListener('DOMContentLoaded', () => {
  updateTemperatureUI();
  setDimmer(75, true);
  updateTrackDisplay();
  refreshActiveDeviceCount();
});
