const state = { running: false, timer: null, serialPort: null, reader: null, packets: [], virtualPacket: 0, virtualPhase: 0 };
const $ = (id) => document.getElementById(id);

const sourceSelect = $('sourceSelect');
const startButton = $('startButton');
const connectButton = $('connectButton');
const stopButton = $('stopButton');
const intervalInput = $('intervalInput');

sourceSelect.addEventListener('change', () => {
  const serial = sourceSelect.value === 'serial';
  connectButton.hidden = !serial;
  startButton.hidden = serial;
  intervalInput.disabled = serial;
  setConnection(serial ? 'ESP32 ready to connect' : 'Virtual standby', false);
});
startButton.addEventListener('click', startVirtual);
stopButton.addEventListener('click', stopStream);
connectButton.addEventListener('click', connectSerial);
$('clearButton').addEventListener('click', () => { state.packets = []; renderTable(); });

function setConnection(text, connected) {
  $('connectionText').textContent = text;
  $('connectionPill').classList.toggle('connected', connected);
}

function startVirtual() {
  stopStream();
  state.running = true;
  state.virtualPacket = 0;
  state.virtualPhase = 0;
  setConnection('Virtual stream active', true);
  tickVirtual();
  state.timer = setInterval(tickVirtual, Math.max(250, Number(intervalInput.value) || 1000));
}

function tickVirtual() {
  state.virtualPacket += 1;
  state.virtualPhase += 0.55;
  const temperature = 32 + 4.5 * Math.sin(state.virtualPhase);
  const humidity = 58 - 12 * Math.sin(state.virtualPhase);
  const heatIndex = temperature + humidity * 0.1;
  const severity = severityName(temperature, heatIndex);
  const assessment = assess(temperature, humidity);
  applyPacket({
    packet_id: state.virtualPacket, node_id: 'VIRTUAL_UHI_NODE_JS', ambient_temp_c: temperature,
    relative_humidity_pct: humidity, heat_index_c: heatIndex, severity,
    green_led: severity === 'LOW', yellow_led: severity === 'MODERATE', red_led: severity === 'HIGH',
    buzzer: temperature >= 38, ...assessment, uptime_ms: state.virtualPacket * Number(intervalInput.value || 1000)
  });
}

function severityName(temperature, heatIndex) {
  if (temperature >= 36.5 || heatIndex >= 38) return 'HIGH';
  if (temperature >= 31 || heatIndex >= 33) return 'MODERATE';
  return 'LOW';
}

function assess(temperature, humidity) {
  const stress = clamp((temperature + 8 - 20) * 2 + (temperature - 20) * 2.5 + Math.max(0, humidity - 40) * .15 + 0.35 * 8 - .20 * 10, 0, 100);
  const raw = 2.4 + 3.4 + 1.7;
  const drop = clamp(6 * (1 - Math.exp(-.22 * raw)), 0, 6);
  return { thermal_stress_index: stress, expected_drop_c: drop, hvac_savings_pct: drop * 2.2, recommendation: 'PERMEABLE_PAVEMENT' };
}

async function connectSerial() {
  if (!('serial' in navigator)) { setConnection('Web Serial unavailable', false); return; }
  try {
    state.serialPort = await navigator.serial.requestPort();
    await state.serialPort.open({ baudRate: 115200 });
    setConnection('ESP32 connected', true);
    readSerial();
  } catch (error) { setConnection('Connection cancelled', false); console.error(error); }
}

async function readSerial() {
  const decoder = new TextDecoderStream();
  state.serialPort.readable.pipeTo(decoder.writable);
  state.reader = decoder.readable.getReader();
  let buffer = '';
  try {
    while (state.reader) {
      const { value, done } = await state.reader.read();
      if (done) break;
      buffer += value;
      const lines = buffer.split('\n');
      buffer = lines.pop();
      lines.forEach((line) => { try { if (line.trim()) applyPacket(JSON.parse(line)); } catch (_) {} });
    }
  } catch (error) { console.error(error); setConnection('Serial disconnected', false); }
}

async function stopStream() {
  state.running = false;
  if (state.timer) { clearInterval(state.timer); state.timer = null; }
  if (state.reader) { await state.reader.cancel().catch(() => {}); state.reader = null; }
  if (state.serialPort) { await state.serialPort.close().catch(() => {}); state.serialPort = null; }
  if (sourceSelect.value === 'virtual') setConnection('Virtual standby', false);
}

function applyPacket(packet) {
  if (packet.error) { $('severityValue').textContent = 'FAULT'; $('severityDetail').textContent = packet.error; return; }
  state.packets.unshift(packet);
  state.packets = state.packets.slice(0, 12);
  $('temperatureValue').textContent = number(packet.ambient_temp_c);
  $('humidityValue').textContent = `${number(packet.relative_humidity_pct)} %`;
  $('heatIndexValue').textContent = `${number(packet.heat_index_c)} C`;
  $('uptimeValue').textContent = packet.uptime_ms == null ? '--' : `${Math.round(packet.uptime_ms / 1000)} s`;
  $('severityValue').textContent = packet.severity || packet.hardware_alert || 'UNKNOWN';
  $('severityDetail').textContent = detail(packet.severity || packet.hardware_alert);
  $('stressValue').textContent = packet.thermal_stress_index == null ? '--' : number(packet.thermal_stress_index);
  $('dropValue').textContent = packet.expected_drop_c == null ? '--.- C' : `${number(packet.expected_drop_c)} C`;
  $('savingsValue').textContent = packet.hvac_savings_pct == null ? '--.- %' : `${number(packet.hvac_savings_pct)} %`;
  $('recommendationValue').textContent = pretty(packet.recommendation || 'Awaiting assessment');
  setActuator('greenLed', 'greenState', packet.green_led, 'ON');
  setActuator('yellowLed', 'yellowState', packet.yellow_led, 'ON');
  setActuator('redLed', 'redState', packet.red_led, 'ON');
  const buzzer = Boolean(packet.buzzer);
  $('buzzerState').classList.toggle('on', buzzer); $('buzzerText').textContent = buzzer ? 'ON' : 'OFF';
  updateSeverity(packet.severity || packet.hardware_alert);
  renderTable(); drawChart();
  $('packetCount').textContent = `${state.packets.length} packet${state.packets.length === 1 ? '' : 's'}`;
  $('lastUpdated').textContent = `Updated ${new Date().toLocaleTimeString()}`;
}

function setActuator(dot, text, on, label) { $(dot).classList.toggle('on', Boolean(on)); $(text).textContent = on ? label : 'OFF'; }
function updateSeverity(severity) { const panel = $('severityPanel'); panel.classList.remove('moderate', 'high'); if (severity === 'MODERATE') panel.classList.add('moderate'); if (severity === 'HIGH') panel.classList.add('high'); $('severityMeter').style.width = severity === 'HIGH' ? '100%' : severity === 'MODERATE' ? '62%' : '28%'; }
function detail(severity) { return severity === 'HIGH' ? 'Critical thermal stress detected. Immediate cooling action is recommended.' : severity === 'MODERATE' ? 'Elevated heat load. The edge engine has selected a mitigation strategy.' : 'Microclimate is within the low-stress range.'; }
function number(value) { return Number(value).toFixed(1); }
function pretty(value) { return String(value).replaceAll('_', ' '); }
function clamp(value, min, max) { return Math.min(max, Math.max(min, value)); }

function renderTable() { $('packetTable').innerHTML = state.packets.length ? state.packets.map((packet) => `<tr><td>#${packet.packet_id ?? '--'}</td><td>${number(packet.ambient_temp_c)} C</td><td>${number(packet.relative_humidity_pct)} %</td><td class="severity-cell">${packet.severity || '--'}</td><td>${pretty(packet.recommendation || '--')}</td><td>${new Date().toLocaleTimeString()}</td></tr>`).join('') : '<tr><td colspan="6" class="empty">No telemetry received.</td></tr>'; }

function drawChart() {
  const canvas = $('temperatureChart'); const ctx = canvas.getContext('2d'); const rect = canvas.getBoundingClientRect(); const scale = window.devicePixelRatio || 1;
  canvas.width = rect.width * scale; canvas.height = 260 * scale; ctx.scale(scale, scale); const width = rect.width; const height = 260; ctx.clearRect(0, 0, width, height);
  ctx.strokeStyle = '#d9ddd3'; ctx.lineWidth = 1; for (let y = 30; y < height; y += 48) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke(); }
  if (!state.packets.length) return; const values = state.packets.slice().reverse().map((p) => Number(p.ambient_temp_c)); const min = Math.min(...values) - 1; const max = Math.max(...values) + 1;
  ctx.strokeStyle = '#0d716b'; ctx.lineWidth = 3; ctx.beginPath(); values.forEach((value, index) => { const x = values.length === 1 ? width / 2 : (index / (values.length - 1)) * width; const y = height - 28 - ((value - min) / (max - min)) * (height - 58); index ? ctx.lineTo(x, y) : ctx.moveTo(x, y); }); ctx.stroke();
  ctx.fillStyle = '#d2e85a'; values.forEach((value, index) => { const x = values.length === 1 ? width / 2 : (index / (values.length - 1)) * width; const y = height - 28 - ((value - min) / (max - min)) * (height - 58); ctx.beginPath(); ctx.arc(x, y, 4, 0, Math.PI * 2); ctx.fill(); });
}
window.addEventListener('resize', drawChart);
