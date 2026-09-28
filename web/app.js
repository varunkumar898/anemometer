/**
 * ══════════════════════════════════════════════════════════════════
 *  AeroGauge — Human-Centered Controller Logic & Web Serial Engine
 * ══════════════════════════════════════════════════════════════════
 */

// Application State
const State = {
  port: null,
  reader: null,
  isConnected: false,
  isDemo: false,
  demoInterval: null,
  isStreaming: true,
  
  // Selected display unit ('ms', 'knots', 'kmh', 'mph')
  unit: 'ms',
  
  // Real-time telemetry
  currentWindMS: 0.0,
  peakGustMS: 0.0,
  peakTimeStr: '--:--',
  rawADC: 0,
  filteredADC: 0.0,
  
  // Settings & Thresholds
  windLimit: 20.0,
  buzzerLimit: 20.0,
  buzzerPeriodMS: 500,
  maxSensorRating: 45.0,
  brightness: 8,
  
  // Safety Interlocks
  limitEnabled: false,
  buzzerEnabled: false,
  cutoff1Enabled: false,
  cutoff2Enabled: false,
  
  // Status flags
  alarmActive: false,
  isWarmingUp: false,
  isFault: false,
  
  // 16-Bit Calibration
  zeroCounts: 6400,
  fsCounts: 32000,
  shuntOhms: 100.0,
  adcMax: 32767,
  
  // Waveform 60-point rolling history
  historyMS: new Array(60).fill(0),
  commandHistory: [],
  historyPointer: -1,
};

// Cached Element References
const UI = {
  // Navigation & Toasts
  toastContainer: document.getElementById('toast-container'),
  tabButtons: document.querySelectorAll('.tab-btn, .mobile-nav-btn'),
  tabPanes: document.querySelectorAll('.tab-pane'),
  
  // Connection Elements
  btnConnect: document.getElementById('btn-connect'),
  connectText: document.getElementById('connect-text'),
  statusDot: document.getElementById('status-dot'),
  statusLabel: document.getElementById('status-label'),
  selectBaud: document.getElementById('select-baud'),
  btnToggleDemo: document.getElementById('btn-toggle-demo'),
  demoBtnLabel: document.getElementById('demo-btn-label'),
  
  // System Alert Banner
  systemBanner: document.getElementById('system-banner'),
  bannerTitle: document.getElementById('banner-title'),
  bannerDesc: document.getElementById('banner-desc'),
  bannerIcon: document.getElementById('banner-icon'),
  
  // Hero Instrument Display
  valPrimaryWind: document.getElementById('val-primary-wind'),
  valPrimaryUnit: document.getElementById('val-primary-unit'),
  txtBeaufort: document.getElementById('txt-beaufort'),
  unitPills: document.querySelectorAll('.unit-pill'),
  
  // Dial Elements
  dialFillArc: document.getElementById('dial-fill-arc'),
  dialNeedlePoly: document.getElementById('dial-needle-poly'),
  dialLimitMarker: document.getElementById('dial-limit-marker'),
  dialTxtMid: document.getElementById('dial-txt-mid'),
  dialTxtMax: document.getElementById('dial-txt-max'),
  
  // MAX7219 Hardware Display Replica
  replicaD0: document.getElementById('replica-d0'),
  replicaD1: document.getElementById('replica-d1'),
  replicaD2: document.getElementById('replica-d2'),
  
  // Metric Cards
  metricPeakGust: document.getElementById('metric-peak-gust'),
  metricPeakSubtext: document.getElementById('metric-peak-subtext'),
  metricAvgWind: document.getElementById('metric-avg-wind'),
  metricLoopCurrent: document.getElementById('metric-loop-current'),
  metricAdcCounts: document.getElementById('metric-adc-counts'),
  metricHealthBadge: document.getElementById('metric-health-badge'),
  
  // Waveform Canvas
  chartCanvas: document.getElementById('canvas-wind-trend'),
  chartLimitVal: document.getElementById('chart-limit-val'),
  
  // Safety Interlocks
  toggleMasterAlarm: document.getElementById('toggle-master-alarm'),
  toggleBuzzer: document.getElementById('toggle-buzzer'),
  toggleCutoff1: document.getElementById('toggle-cutoff1'),
  toggleCutoff2: document.getElementById('toggle-cutoff2'),
  chipAlarmState: document.getElementById('chip-alarm-state'),
  chipBuzzerState: document.getElementById('chip-buzzer-state'),
  chipCutoff1State: document.getElementById('chip-cutoff1-state'),
  chipCutoff2State: document.getElementById('chip-cutoff2-state'),
  lblAlarmThreshold: document.getElementById('lbl-alarm-threshold'),
  
  // Buzzer Limit & Pulse Rate Controls
  inputBuzzerLimit: document.getElementById('input-buzzer-limit'),
  btnApplyBuzzLimit: document.getElementById('btn-apply-buzzlimit'),
  buzzerRateButtons: document.querySelectorAll('#buzzer-rate-selector .rate-btn'),
  lblBuzzerSummary: document.getElementById('lbl-buzzer-summary'),

  // Onboard Hardware Indicators
  lampHwGreen: document.getElementById('lamp-hw-green'),
  lampTxtGreen: document.getElementById('lamp-txt-green'),
  lampHwRed: document.getElementById('lamp-hw-red'),
  lampTxtRed: document.getElementById('lamp-txt-red'),
  lampHwWarmup: document.getElementById('lamp-hw-warmup'),
  lampTxtWarmup: document.getElementById('lamp-txt-warmup'),
  
  // Calibration & Form Controls
  inputAlarmLimit: document.getElementById('input-alarm-limit'),
  btnApplyLimit: document.getElementById('btn-apply-limit'),
  inputSensorMax: document.getElementById('input-sensor-max'),
  btnApplyMax: document.getElementById('btn-apply-max'),
  sliderDisplayIntensity: document.getElementById('slider-display-intensity'),
  sliderBrightVal: document.getElementById('slider-bright-val'),
  
  inputZeroVal: document.getElementById('input-zero-val'),
  btnApplyZero: document.getElementById('btn-apply-zero'),
  inputFsVal: document.getElementById('input-fs-val'),
  btnApplyFs: document.getElementById('btn-apply-fs'),
  inputShuntVal: document.getElementById('input-shunt-val'),
  btnApplyShunt: document.getElementById('btn-apply-shunt'),
  inputAdcMaxVal: document.getElementById('input-adcmax-val'),
  btnApplyAdcMax: document.getElementById('btn-apply-adcmax'),
  
  btnSaveFlash: document.getElementById('btn-save-flash'),
  btnRestoreDefaults: document.getElementById('btn-restore-defaults'),
  
  // Terminal Elements
  terminalScreen: document.getElementById('terminal-screen'),
  termInputText: document.getElementById('term-input-text'),
  btnTermSend: document.getElementById('btn-term-send'),
  btnTermClear: document.getElementById('btn-term-clear'),
  btnTermStreamToggle: document.getElementById('btn-term-stream-toggle'),
  termStreamPill: document.getElementById('term-stream-pill'),
};

const canvasCtx = UI.chartCanvas ? UI.chartCanvas.getContext('2d') : null;

/* ══════════════════════════════════════════
 *  Toast Notifications
 * ══════════════════════════════════════════ */
function showToast(message, type = 'info') {
  if (!UI.toastContainer) return;
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  toast.textContent = message;
  UI.toastContainer.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(-10px)';
    toast.style.transition = 'all 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 3200);
}

/* ══════════════════════════════════════════
 *  Navigation & Tab Switching
 * ══════════════════════════════════════════ */
function switchTab(tabId) {
  UI.tabPanes.forEach(pane => {
    pane.classList.toggle('active', pane.id === tabId);
  });

  UI.tabButtons.forEach(btn => {
    const isTarget = btn.getAttribute('data-tab') === tabId;
    btn.classList.toggle('active', isTarget);
  });
}

UI.tabButtons.forEach(btn => {
  btn.addEventListener('click', () => {
    const tabId = btn.getAttribute('data-tab');
    if (tabId) switchTab(tabId);
  });
});

/* ══════════════════════════════════════════
 *  Unit Conversions
 * ══════════════════════════════════════════ */
function convertWind(speedMS, targetUnit) {
  switch (targetUnit) {
    case 'knots': return speedMS * 1.94384;
    case 'kmh':   return speedMS * 3.6;
    case 'mph':   return speedMS * 2.23694;
    default:      return speedMS;
  }
}

function getUnitLabel(unit) {
  switch (unit) {
    case 'knots': return 'knots';
    case 'kmh':   return 'km/h';
    case 'mph':   return 'mph';
    default:      return 'm/s';
  }
}

UI.unitPills.forEach(pill => {
  pill.addEventListener('click', () => {
    UI.unitPills.forEach(p => p.classList.remove('active'));
    pill.classList.add('active');
    State.unit = pill.getAttribute('data-unit') || 'ms';
    UI.valPrimaryUnit.textContent = getUnitLabel(State.unit);
    updateTelemetry(State.currentWindMS, State.rawADC);
  });
});

/* ══════════════════════════════════════════
 *  Web Serial API Communication
 * ══════════════════════════════════════════ */
async function connectSerial() {
  if (!('serial' in navigator)) {
    showToast('Web Serial is supported on Chrome, Edge, and Opera.', 'error');
    enableDemo(true);
    return;
  }

  try {
    const baudRate = (UI.selectBaud && UI.selectBaud.value) ? parseInt(UI.selectBaud.value, 10) : 115200;
    State.port = await navigator.serial.requestPort();
    await State.port.open({ baudRate });

    State.isConnected = true;
    updateConnectionStatus('online', 'USB Online');
    UI.connectText.textContent = 'Disconnect';
    UI.btnConnect.classList.add('connected');
    showToast('Connected to anemometer via USB', 'success');

    if (State.isDemo) enableDemo(false);

    readLoop();

    // Sync state
    setTimeout(() => {
      sendCommand('status');
      sendCommand('stream 1');
    }, 350);

  } catch (err) {
    if (err.name !== 'NotFoundError') {
      showToast(`Connection failed: ${err.message}`, 'error');
    }
  }
}

async function disconnectSerial() {
  if (State.reader) {
    await State.reader.cancel();
  }
  if (State.port) {
    try {
      await State.port.close();
    } catch (e) {
      console.warn(e);
    }
  }
  State.port = null;
  State.reader = null;
  State.isConnected = false;
  UI.connectText.textContent = 'Connect USB';
  UI.btnConnect.classList.remove('connected');
  updateConnectionStatus('offline', 'Offline');
  showToast('Serial port disconnected', 'info');
}

async function readLoop() {
  const textDecoder = new TextDecoderStream();
  State.port.readable.pipeTo(textDecoder.writable);
  const reader = textDecoder.readable.getReader();
  State.reader = reader;

  let buffer = '';

  try {
    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      if (value) {
        buffer += value;
        const lines = buffer.split(/\r?\n/);
        buffer = lines.pop();

        for (const line of lines) {
          const trimmed = line.trim();
          if (trimmed.length > 0) {
            handleFirmwareMessage(trimmed);
          }
        }
      }
    }
  } catch (err) {
    if (State.isConnected) {
      logTerminal(`Read error: ${err.message}`, 'err');
    }
  } finally {
    reader.releaseLock();
  }
}

async function sendCommand(cmd) {
  const clean = cmd.trim();
  if (!clean) return;

  logTerminal(`> ${clean}`, 'tx');

  if (State.isConnected && State.port && State.port.writable) {
    const encoder = new TextEncoder();
    const writer = State.port.writable.getWriter();
    await writer.write(encoder.encode(clean + '\r\n'));
    writer.releaseLock();
  } else if (State.isDemo) {
    processDemoCommand(clean);
  } else {
    showToast('Device offline. Connect via USB or turn on Demo.', 'error');
  }
}

function updateConnectionStatus(state, label) {
  UI.statusDot.className = `status-indicator-dot ${state}`;
  UI.statusLabel.textContent = label;
}

/* ══════════════════════════════════════════
 *  Telemetry & Firmware Parser
 * ══════════════════════════════════════════ */
function handleFirmwareMessage(line) {
  logTerminal(line);

  // 1. JSON Telemetry: {"wind":"12.45","raw":"16200"}
  if (line.startsWith('{') && line.endsWith('}')) {
    try {
      const obj = JSON.parse(line);
      if (obj.wind !== undefined) {
        updateTelemetry(parseFloat(obj.wind), parseInt(obj.raw, 10));
      }
    } catch (e) {
      // not JSON
    }
    return;
  }

  // 2. Structured text parsing
  if (line.includes('Current Wind Speed')) {
    const m = line.match(/:\s*([0-9.]+)/);
    if (m) State.currentWindMS = parseFloat(m[1]);
  }
  if (line.includes('Raw ADC Counts')) {
    const m = line.match(/:\s*([0-9]+)/);
    if (m) State.rawADC = parseInt(m[1], 10);
  }
  if (line.includes('Wind Limit Alarm')) {
    State.alarmActive = line.includes('EXCEEDED');
  }
  if (line.includes('Buzzer Output')) {
    const limMatch = line.match(/Limit:\s*([0-9.]+)/);
    if (limMatch) {
      State.buzzerLimit = parseFloat(limMatch[1]);
      if (UI.inputBuzzerLimit) UI.inputBuzzerLimit.value = State.buzzerLimit.toFixed(1);
    }
    const rateMatch = line.match(/Rate:\s*([0-9]+)/);
    if (rateMatch) {
      State.buzzerPeriodMS = parseInt(rateMatch[1], 10);
      updateBuzzerRateUI(State.buzzerPeriodMS);
    }
  }
  if (line.includes('Buzzer wind limit set to')) {
    const m = line.match(/set to\s*([0-9.]+)/);
    if (m) {
      State.buzzerLimit = parseFloat(m[1]);
      if (UI.inputBuzzerLimit) UI.inputBuzzerLimit.value = State.buzzerLimit.toFixed(1);
      showToast(`Buzzer limit: ${State.buzzerLimit.toFixed(1)} m/s`, 'success');
    }
  }
  if (line.includes('Buzzer pulse period set to')) {
    const m = line.match(/set to\s*([0-9]+)/);
    if (m) {
      State.buzzerPeriodMS = parseInt(m[1], 10);
      updateBuzzerRateUI(State.buzzerPeriodMS);
      showToast(`Buzzer pulse: ${State.buzzerPeriodMS}ms`, 'success');
    }
  }
  if (line.includes('continuous solid tone')) {
    State.buzzerPeriodMS = 0;
    updateBuzzerRateUI(0);
    showToast('Buzzer mode: Continuous Tone', 'success');
  }
  if (line.includes('Sensor Status')) {
    State.isFault = line.includes('FAULT');
  }
  if (line.includes('Display Brightness')) {
    const m = line.match(/:\s*([0-9]+)/);
    if (m) {
      State.brightness = parseInt(m[1], 10);
      UI.sliderDisplayIntensity.value = State.brightness;
      UI.sliderBrightVal.textContent = `Level ${State.brightness} / 15`;
    }
  }
  if (line.includes('Zero / FS Counts')) {
    const m = line.match(/:\s*([0-9]+)\s*\/\s*([0-9]+)/);
    if (m) {
      State.zeroCounts = parseInt(m[1], 10);
      State.fsCounts = parseInt(m[2], 10);
      UI.inputZeroVal.value = State.zeroCounts;
      UI.inputFsVal.value = State.fsCounts;
    }
  }
}

function updateTelemetry(windMS, rawCount) {
  State.currentWindMS = windMS;
  State.rawADC = rawCount;

  // Session Peak Gust tracking
  if (windMS > State.peakGustMS) {
    State.peakGustMS = windMS;
    const now = new Date();
    State.peakTimeStr = now.toTimeString().substring(0, 5);
  }

  // EMA Filter calculation
  if (State.filteredADC < 0.0) {
    State.filteredADC = rawCount;
  } else {
    State.filteredADC = State.filteredADC * 0.8 + rawCount * 0.2;
  }

  // Rolling History Buffer
  State.historyMS.push(windMS);
  if (State.historyMS.length > 60) State.historyMS.shift();

  // 1. Primary Wind Readout
  const convertedValue = convertWind(windMS, State.unit);
  UI.valPrimaryWind.textContent = convertedValue.toFixed(2);
  UI.valPrimaryUnit.textContent = getUnitLabel(State.unit);
  UI.txtBeaufort.textContent = formatBeaufort(windMS);

  // 2. Dial Gauge Needle & Swept Arc
  renderDialGauge(windMS, State.maxSensorRating, State.windLimit);

  // 3. Physical MAX7219 Hardware Display Replica
  renderPhysicalDisplay(windMS);

  // 4. Metrics Cards
  const convertedPeak = convertWind(State.peakGustMS, State.unit);
  UI.metricPeakGust.textContent = convertedPeak.toFixed(2);
  UI.metricPeakSubtext.textContent = `Peak logged: ${State.peakTimeStr}`;

  const avgMS = State.historyMS.reduce((a, b) => a + b, 0) / State.historyMS.length;
  const convertedAvg = convertWind(avgMS, State.unit);
  UI.metricAvgWind.textContent = convertedAvg.toFixed(2);

  const loopVolt = rawCount * 0.0000625;
  const loopCurrentMA = loopVolt / (State.shuntOhms / 1000.0);
  UI.metricLoopCurrent.textContent = loopCurrentMA.toFixed(2);
  UI.metricAdcCounts.textContent = `Raw ADC: ${rawCount} counts`;

  const isFault = (State.zeroCounts > 500 && rawCount < 100) || (rawCount > State.adcMax + 1000);
  State.isFault = isFault;
  UI.metricHealthBadge.className = `health-badge ${isFault ? 'fault' : 'healthy'}`;
  UI.metricHealthBadge.textContent = isFault ? 'Fault / Wire Break' : 'Loop Healthy';

  // 5. System Status Banner & Relays
  const exceed = (windMS >= State.windLimit) && !State.isWarmingUp;
  State.alarmActive = State.limitEnabled && exceed;

  if (exceed) {
    UI.systemBanner.className = 'system-banner alarm';
    UI.bannerTitle.textContent = 'High Wind Velocity Alert';
    UI.bannerDesc.textContent = `Speed of ${windMS.toFixed(2)} m/s exceeds threshold (${State.windLimit.toFixed(1)} m/s). Safety interlocks engaged.`;
  } else {
    UI.systemBanner.className = 'system-banner safe';
    UI.bannerTitle.textContent = 'Normal Operating Conditions';
    UI.bannerDesc.textContent = 'Wind velocity within safe limits. Safety relays disengaged.';
  }

  // Interlock Chips
  UI.chipAlarmState.textContent = State.alarmActive ? 'Triggered' : (State.limitEnabled ? 'Armed' : 'Disarmed');
  UI.chipAlarmState.className = `status-chip-state ${State.alarmActive ? 'active' : (State.limitEnabled ? 'armed' : '')}`;

  // Buzzer condition: independent threshold buzzerLimit or fallback to windLimit
  const bLimit = State.buzzerLimit > 0.001 ? State.buzzerLimit : State.windLimit;
  const buzzerExceed = (windMS >= bLimit) && !State.isWarmingUp;
  const buzzActive = State.buzzerEnabled && buzzerExceed;

  if (UI.chipBuzzerState) {
    if (buzzActive) {
      const rateLabel = State.buzzerPeriodMS === 0 ? 'Continuous' : `${State.buzzerPeriodMS}ms Pulse`;
      UI.chipBuzzerState.textContent = `Active (${rateLabel})`;
      UI.chipBuzzerState.className = 'status-chip-state active';
    } else {
      UI.chipBuzzerState.textContent = State.buzzerEnabled ? 'Armed' : 'Silent';
      UI.chipBuzzerState.className = `status-chip-state ${State.buzzerEnabled ? 'armed' : ''}`;
    }
  }

  if (UI.lblBuzzerSummary) {
    const rateText = State.buzzerPeriodMS === 0 ? 'Continuous siren' : `${State.buzzerPeriodMS}ms pulse`;
    UI.lblBuzzerSummary.textContent = `Triggers at \u2265 ${bLimit.toFixed(1)} m/s \u2022 ${rateText}`;
  }

  const c1 = State.cutoff1Enabled && State.alarmActive;
  UI.chipCutoff1State.textContent = c1 ? 'Cutoff Active' : (State.cutoff1Enabled ? 'Armed' : 'Disengaged');
  UI.chipCutoff1State.className = `status-chip-state ${c1 ? 'active' : ''}`;

  const c2 = State.cutoff2Enabled && State.alarmActive;
  UI.chipCutoff2State.textContent = c2 ? 'Cutoff Active' : (State.cutoff2Enabled ? 'Armed' : 'Disengaged');
  UI.chipCutoff2State.className = `status-chip-state ${c2 ? 'active' : ''}`;

  // Hardware Lamps
  UI.lampHwGreen.classList.toggle('on', !exceed);
  UI.lampTxtGreen.textContent = !exceed ? 'Active (Normal)' : 'Standby';

  UI.lampHwRed.classList.toggle('on', exceed);
  UI.lampTxtRed.textContent = exceed ? 'Triggered (High)' : 'Off';

  // 6. Draw Trend Waveform
  renderTrendCanvas();
}

function renderDialGauge(valueMS, maxMS, limitMS) {
  const clamped = Math.min(Math.max(valueMS, 0), maxMS);
  const ratio = maxMS > 0 ? clamped / maxMS : 0;

  // Angle: -90deg to +90deg (180deg arc)
  const angle = -90 + (ratio * 180);
  UI.dialNeedlePoly.style.transform = `rotate(${angle}deg)`;

  // Arc length for r=110 semicircle is 345.5
  const arcLen = 345.5;
  const offset = arcLen - (ratio * arcLen);
  UI.dialFillArc.style.strokeDashoffset = offset;
  UI.dialFillArc.classList.toggle('alarm', valueMS >= limitMS);

  // Limit tick position
  const limitRatio = maxMS > 0 ? Math.min(Math.max(limitMS, 0), maxMS) / maxMS : 0.5;
  const rad = Math.PI - (limitRatio * Math.PI);
  const x1 = 150 + 110 * Math.cos(rad);
  const y1 = 175 - 110 * Math.sin(rad);
  const x2 = 150 + 96 * Math.cos(rad);
  const y2 = 175 - 96 * Math.sin(rad);
  UI.dialLimitMarker.setAttribute('x1', x1);
  UI.dialLimitMarker.setAttribute('y1', y1);
  UI.dialLimitMarker.setAttribute('x2', x2);
  UI.dialLimitMarker.setAttribute('y2', y2);

  UI.dialTxtMax.textContent = maxMS.toFixed(1);
  UI.dialTxtMid.textContent = (maxMS / 2).toFixed(1);
  UI.chartLimitVal.textContent = `${limitMS.toFixed(1)} m/s`;
  UI.lblAlarmThreshold.textContent = `${limitMS.toFixed(1)} m/s`;
}

function renderPhysicalDisplay(valMS) {
  if (State.alarmActive) {
    const flash = Math.floor(Date.now() / 500) % 2 === 0;
    if (flash) {
      UI.replicaD0.textContent = 'V';
      UI.replicaD1.textContent = 'i';
      UI.replicaD2.textContent = 'd';
    } else {
      UI.replicaD0.innerHTML = '&nbsp;';
      UI.replicaD1.innerHTML = '&nbsp;';
      UI.replicaD2.innerHTML = '&nbsp;';
    }
    return;
  }

  const clamped = Math.min(Math.max(valMS, 0), 99.9);
  const integerVal = Math.round(clamped * 10);
  const tens = Math.floor(integerVal / 100) % 10;
  const ones = Math.floor(integerVal / 10) % 10;
  const tenths = integerVal % 10;

  UI.replicaD0.innerHTML = (tens === 0 && integerVal < 100) ? '&nbsp;' : tens;
  UI.replicaD1.textContent = ones + '.';
  UI.replicaD2.textContent = tenths;
}

function formatBeaufort(ms) {
  if (ms < 0.3) return 'F0 Calm (0 m/s)';
  if (ms < 1.6) return 'F1 Light Air (1 m/s)';
  if (ms < 3.4) return 'F2 Light Breeze (2–3 m/s)';
  if (ms < 5.5) return 'F3 Gentle Breeze (4–5 m/s)';
  if (ms < 8.0) return 'F4 Moderate Breeze (6–7 m/s)';
  if (ms < 10.8) return 'F5 Fresh Breeze (8–10 m/s)';
  if (ms < 13.9) return 'F6 Strong Breeze (11–13 m/s)';
  if (ms < 17.2) return 'F7 Near Gale (14–17 m/s)';
  if (ms < 20.8) return 'F8 Gale (18–20 m/s)';
  if (ms < 24.5) return 'F9 Strong Gale (21–24 m/s)';
  if (ms < 28.5) return 'F10 Storm (25–28 m/s)';
  return 'F11 Violent Storm (29+ m/s)';
}

/* ══════════════════════════════════════════
 *  Waveform Canvas Rendering
 * ══════════════════════════════════════════ */
function renderTrendCanvas() {
  if (!canvasCtx || !UI.chartCanvas) return;
  const w = UI.chartCanvas.width;
  const h = UI.chartCanvas.height;

  canvasCtx.clearRect(0, 0, w, h);

  // Subtle Horizontal Guides
  canvasCtx.strokeStyle = 'rgba(255, 255, 255, 0.04)';
  canvasCtx.lineWidth = 1;
  for (let y = 15; y < h; y += 30) {
    canvasCtx.beginPath();
    canvasCtx.moveTo(0, y);
    canvasCtx.lineTo(w, y);
    canvasCtx.stroke();
  }

  const maxVal = Math.max(State.maxSensorRating, 45.0);
  const stepX = w / (State.historyMS.length - 1);

  // Alarm Limit Reference Line
  const limitY = h - (State.windLimit / maxVal) * (h - 20) - 10;
  canvasCtx.beginPath();
  canvasCtx.setLineDash([4, 4]);
  canvasCtx.strokeStyle = '#f43f5e';
  canvasCtx.lineWidth = 1.2;
  canvasCtx.moveTo(0, limitY);
  canvasCtx.lineTo(w, limitY);
  canvasCtx.stroke();
  canvasCtx.setLineDash([]);

  // Shaded Area
  const grad = canvasCtx.createLinearGradient(0, 0, 0, h);
  grad.addColorStop(0, 'rgba(56, 189, 248, 0.35)');
  grad.addColorStop(1, 'rgba(56, 189, 248, 0.0)');

  canvasCtx.beginPath();
  canvasCtx.moveTo(0, h);
  for (let i = 0; i < State.historyMS.length; i++) {
    const val = State.historyMS[i];
    const x = i * stepX;
    const y = h - (val / maxVal) * (h - 20) - 10;
    if (i === 0) canvasCtx.lineTo(x, y);
    else canvasCtx.lineTo(x, y);
  }
  canvasCtx.lineTo(w, h);
  canvasCtx.closePath();
  canvasCtx.fillStyle = grad;
  canvasCtx.fill();

  // Waveform Stroke
  canvasCtx.beginPath();
  for (let i = 0; i < State.historyMS.length; i++) {
    const val = State.historyMS[i];
    const x = i * stepX;
    const y = h - (val / maxVal) * (h - 20) - 10;
    if (i === 0) canvasCtx.moveTo(x, y);
    else canvasCtx.lineTo(x, y);
  }
  canvasCtx.strokeStyle = '#38bdf8';
  canvasCtx.lineWidth = 2;
  canvasCtx.stroke();
}

/* ══════════════════════════════════════════
 *  Demo Simulation Engine
 * ══════════════════════════════════════════ */
function enableDemo(enable) {
  State.isDemo = enable;
  UI.btnToggleDemo.classList.toggle('active', enable);

  if (enable) {
    updateConnectionStatus('demo', 'Demo Simulation Active');
    UI.demoBtnLabel.textContent = 'Demo (On)';
    showToast('Demo simulation started with realistic gust dynamics', 'info');

    let count = 12000;
    let target = 15000;

    State.demoInterval = setInterval(() => {
      if (Math.random() < 0.1) {
        target = 6400 + Math.random() * (State.fsCounts - 6400);
      }
      count += (target - count) * 0.12 + (Math.random() - 0.5) * 120;
      const raw = Math.round(Math.max(0, Math.min(32767, count)));

      const span = State.fsCounts - State.zeroCounts;
      let w = (raw - State.zeroCounts) * (State.maxSensorRating / span);
      if (w < 0.2) w = 0.0;
      if (w > State.maxSensorRating) w = State.maxSensorRating;

      updateTelemetry(w, raw);
    }, 200);

  } else {
    clearInterval(State.demoInterval);
    State.demoInterval = null;
    UI.demoBtnLabel.textContent = 'Demo';
    if (!State.isConnected) updateConnectionStatus('offline', 'Offline');
    showToast('Demo simulation stopped', 'info');
  }
}

function processDemoCommand(cmd) {
  const parts = cmd.split(/\s+/);
  const op = parts[0].toLowerCase();
  const val = parts[1];

  switch (op) {
    case 'data':
      handleFirmwareMessage(`{"wind":"${State.currentWindMS.toFixed(2)}","raw":"${State.rawADC}"}`);
      break;
    case 'status':
      logTerminal('--- Anemometer System Status ---');
      logTerminal(`  Current Wind Speed : ${State.currentWindMS.toFixed(2)} m/s`);
      logTerminal(`  Raw ADC Counts     : ${State.rawADC}`);
      logTerminal(`  Filtered ADC       : ${State.filteredADC.toFixed(1)}`);
      logTerminal(`  Sensor Status      : ${State.isFault ? 'FAULT' : 'HEALTHY'}`);
      logTerminal(`  Wind Limit Alarm   : ${State.currentWindMS >= State.windLimit ? 'EXCEEDED' : 'NORMAL'}`);
      logTerminal(`  Buzzer Wind Limit  : ${State.buzzerLimit.toFixed(2)} m/s`);
      logTerminal(`  Buzzer Pulse Rate  : ${State.buzzerPeriodMS === 0 ? 'CONTINUOUS (0 ms)' : State.buzzerPeriodMS + ' ms'}`);
      logTerminal(`  Display Brightness : ${State.brightness}`);
      logTerminal(`  Zero / FS Counts   : ${State.zeroCounts} / ${State.fsCounts}`);
      break;
    case 'setlimit':
      if (val) {
        State.windLimit = parseFloat(val);
        UI.inputAlarmLimit.value = State.windLimit;
        showToast(`Alarm limit set to ${State.windLimit.toFixed(1)} m/s`, 'success');
      }
      break;
    case 'setbuzzlimit':
      if (val) {
        State.buzzerLimit = parseFloat(val);
        if (UI.inputBuzzerLimit) UI.inputBuzzerLimit.value = State.buzzerLimit.toFixed(1);
        if (UI.lblBuzzerSummary) {
          const rateText = State.buzzerPeriodMS === 0 ? 'Continuous siren' : `${State.buzzerPeriodMS}ms pulse`;
          UI.lblBuzzerSummary.textContent = `Triggers at \u2265 ${State.buzzerLimit.toFixed(1)} m/s \u2022 ${rateText}`;
        }
        showToast(`Buzzer limit set to ${State.buzzerLimit.toFixed(1)} m/s`, 'success');
        logTerminal(`OK: Buzzer wind limit set to ${State.buzzerLimit.toFixed(2)} m/s`);
      }
      break;
    case 'setbuzzrate':
      if (val !== undefined) {
        State.buzzerPeriodMS = parseInt(val, 10);
        updateBuzzerRateUI(State.buzzerPeriodMS);
        const rateText = State.buzzerPeriodMS === 0 ? 'Continuous siren' : `${State.buzzerPeriodMS}ms pulse`;
        if (UI.lblBuzzerSummary) {
          UI.lblBuzzerSummary.textContent = `Triggers at \u2265 ${State.buzzerLimit.toFixed(1)} m/s \u2022 ${rateText}`;
        }
        showToast(`Buzzer cadence set to ${rateText}`, 'success');
        logTerminal(State.buzzerPeriodMS === 0 ? 'OK: Buzzer rate set to continuous solid tone' : `OK: Buzzer pulse period set to ${State.buzzerPeriodMS} ms`);
      }
      break;
    case 'setmax':
      if (val) {
        State.maxSensorRating = parseFloat(val);
        UI.inputSensorMax.value = State.maxSensorRating;
        showToast(`Max sensor rating set to ${State.maxSensorRating.toFixed(1)} m/s`, 'success');
      }
      break;
    case 'setbright':
      if (val) {
        State.brightness = parseInt(val, 10);
        UI.sliderDisplayIntensity.value = State.brightness;
        UI.sliderBrightVal.textContent = `Level ${State.brightness} / 15`;
      }
      break;
    case 'sim':
      if (val) {
        const c = parseInt(val, 10);
        const span = State.fsCounts - State.zeroCounts;
        let w = (c - State.zeroCounts) * (State.maxSensorRating / span);
        if (w < 0.2) w = 0.0;
        if (w > State.maxSensorRating) w = State.maxSensorRating;
        updateTelemetry(w, c);
        showToast(`Injected simulated ADC counts: ${c}`, 'info');
      }
      break;
    case 'save':
      showToast('Configuration committed to non-volatile memory', 'success');
      break;
    case 'reset':
      State.windLimit = 20.0;
      State.buzzerLimit = 20.0;
      State.buzzerPeriodMS = 500;
      State.maxSensorRating = 45.0;
      State.brightness = 8;
      UI.inputAlarmLimit.value = 20.0;
      if (UI.inputBuzzerLimit) UI.inputBuzzerLimit.value = '20.0';
      updateBuzzerRateUI(500);
      UI.inputSensorMax.value = 45.0;
      UI.sliderDisplayIntensity.value = 8;
      UI.sliderBrightVal.textContent = 'Level 8 / 15';
      showToast('Factory calibration defaults restored', 'info');
      break;
    default:
      logTerminal(`OK: Command '${op}' processed.`);
  }
}

/* ══════════════════════════════════════════
 *  Terminal Logging
 * ══════════════════════════════════════════ */
function logTerminal(text, cls = '') {
  if (!UI.terminalScreen) return;
  const row = document.createElement('div');
  row.className = `term-row ${cls}`;
  row.textContent = text;
  UI.terminalScreen.appendChild(row);
  UI.terminalScreen.scrollTop = UI.terminalScreen.scrollHeight;
}

/* ══════════════════════════════════════════
 *  Interactive Controls & Steppers
 * ══════════════════════════════════════════ */
// Rate Segmented UI Updater
function updateBuzzerRateUI(periodMS) {
  if (!UI.buzzerRateButtons) return;
  UI.buzzerRateButtons.forEach(btn => {
    const rate = parseInt(btn.getAttribute('data-rate'), 10);
    btn.classList.toggle('active', rate === periodMS);
  });
}

// Stepper buttons for standard and compact steppers
document.querySelectorAll('.btn-stepper, .btn-stepper-sm').forEach(btn => {
  btn.addEventListener('click', () => {
    const targetId = btn.getAttribute('data-target');
    const step = parseFloat(btn.getAttribute('data-step')) || 1;
    const input = document.getElementById(targetId);
    if (input) {
      let current = parseFloat(input.value) || 0;
      current = Math.max(0, current + step);
      input.value = current.toFixed(1);
    }
  });
});

// Stepper Apply Buttons
UI.btnApplyLimit.addEventListener('click', () => {
  const val = parseFloat(UI.inputAlarmLimit.value);
  if (!isNaN(val)) sendCommand(`setlimit ${val}`);
});

if (UI.btnApplyBuzzLimit) {
  UI.btnApplyBuzzLimit.addEventListener('click', () => {
    const val = parseFloat(UI.inputBuzzerLimit.value);
    if (!isNaN(val)) sendCommand(`setbuzzlimit ${val}`);
  });
}

if (UI.buzzerRateButtons) {
  UI.buzzerRateButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      const rate = parseInt(btn.getAttribute('data-rate'), 10);
      if (!isNaN(rate)) {
        updateBuzzerRateUI(rate);
        sendCommand(`setbuzzrate ${rate}`);
      }
    });
  });
}

UI.btnApplyMax.addEventListener('click', () => {
  const val = parseFloat(UI.inputSensorMax.value);
  if (!isNaN(val)) sendCommand(`setmax ${val}`);
});

UI.sliderDisplayIntensity.addEventListener('input', (e) => {
  const b = parseInt(e.target.value, 10);
  UI.sliderBrightVal.textContent = `Level ${b} / 15`;
  sendCommand(`setbright ${b}`);
});

// Switches
UI.toggleMasterAlarm.addEventListener('change', (e) => {
  State.limitEnabled = e.target.checked;
  sendCommand(`alarm ${e.target.checked ? 1 : 0}`);
});

UI.toggleBuzzer.addEventListener('change', (e) => {
  State.buzzerEnabled = e.target.checked;
  sendCommand(`buzzer ${e.target.checked ? 1 : 0}`);
});

UI.toggleCutoff1.addEventListener('change', (e) => {
  State.cutoff1Enabled = e.target.checked;
  sendCommand(`cutoff1 ${e.target.checked ? 1 : 0}`);
});

UI.toggleCutoff2.addEventListener('change', (e) => {
  State.cutoff2Enabled = e.target.checked;
  sendCommand(`cutoff2 ${e.target.checked ? 1 : 0}`);
});

// Flash Save & Restore
UI.btnSaveFlash.addEventListener('click', () => {
  sendCommand('save');
});

UI.btnRestoreDefaults.addEventListener('click', () => {
  if (confirm('Restore factory default 16-bit calibration?')) {
    sendCommand('reset');
  }
});

// Advanced Calibration Appliers
UI.btnApplyZero.addEventListener('click', () => {
  const v = parseInt(UI.inputZeroVal.value, 10);
  if (!isNaN(v)) sendCommand(`setzero ${v}`);
});

UI.btnApplyFs.addEventListener('click', () => {
  const v = parseInt(UI.inputFsVal.value, 10);
  if (!isNaN(v)) sendCommand(`setfs ${v}`);
});

UI.btnApplyShunt.addEventListener('click', () => {
  const v = parseFloat(UI.inputShuntVal.value);
  if (!isNaN(v)) sendCommand(`setshunt ${v}`);
});

UI.btnApplyAdcMax.addEventListener('click', () => {
  const v = parseInt(UI.inputAdcMaxVal.value, 10);
  if (!isNaN(v)) sendCommand(`setadcmax ${v}`);
});

// Quick Presets
document.querySelectorAll('.btn-preset').forEach(btn => {
  btn.addEventListener('click', () => {
    const sim = btn.getAttribute('data-sim');
    if (sim) sendCommand(`sim ${sim}`);
  });
});

// Quick Command Chips
document.querySelectorAll('.btn-cmd-chip').forEach(btn => {
  btn.addEventListener('click', () => {
    const cmd = btn.getAttribute('data-cmd');
    if (cmd) sendCommand(cmd);
  });
});

// Terminal Commands
UI.btnTermSend.addEventListener('click', () => {
  const text = UI.termInputText.value.trim();
  if (text) {
    State.commandHistory.push(text);
    State.historyPointer = State.commandHistory.length;
    sendCommand(text);
    UI.termInputText.value = '';
  }
});

UI.termInputText.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') {
    UI.btnTermSend.click();
  } else if (e.key === 'ArrowUp') {
    if (State.historyPointer > 0) {
      State.historyPointer--;
      UI.termInputText.value = State.commandHistory[State.historyPointer] || '';
    }
  } else if (e.key === 'ArrowDown') {
    if (State.historyPointer < State.commandHistory.length - 1) {
      State.historyPointer++;
      UI.termInputText.value = State.commandHistory[State.historyPointer] || '';
    } else {
      State.historyPointer = State.commandHistory.length;
      UI.termInputText.value = '';
    }
  }
});

UI.btnTermClear.addEventListener('click', () => {
  UI.terminalScreen.innerHTML = '';
});

UI.btnTermStreamToggle.addEventListener('click', () => {
  State.isStreaming = !State.isStreaming;
  sendCommand(`stream ${State.isStreaming ? 1 : 0}`);
  UI.termStreamPill.textContent = State.isStreaming ? 'Streaming' : 'Paused';
  UI.termStreamPill.className = `term-pill ${State.isStreaming ? 'green-pill' : ''}`;
  UI.btnTermStreamToggle.textContent = State.isStreaming ? 'Pause Stream' : 'Resume Stream';
});

// Connect Button
UI.btnConnect.addEventListener('click', () => {
  if (State.isConnected) {
    disconnectSerial();
  } else {
    connectSerial();
  }
});

// Demo Button
UI.btnToggleDemo.addEventListener('click', () => {
  enableDemo(!State.isDemo);
});

// Resize canvas listener for crisp HiDPI displays
window.addEventListener('resize', () => {
  renderTrendCanvas();
});

// Start Demo on boot for immediate human engagement
window.addEventListener('DOMContentLoaded', () => {
  enableDemo(true);
});
