/* ==============================================================================
 * Project: Smart Cooling Strategy Recommendation System for Urban Heat Island
 * Component: GIS & Interactive Decision-Support Dashboard Logic
 * Language: Pure JavaScript / Web GIS / Open-Meteo Live API Integration
 * Academic: Sir MVIT CSE Capstone Phase-II
 * ============================================================================== */

// Micro-Zone Spatial and Environmental Database (Bengaluru Metropolitan Region)
const MICRO_ZONES = [
  {
    id: "MZ_001",
    name: "Cubbon Park Ecological Belt",
    type: "Green Canopy / Park",
    lat: 12.9763,
    lng: 77.5929,
    ambient_temp: 26.5,
    lst: 23.8,
    humidity: 68.0,
    ndvi: 0.74,
    ndbi: -0.42,
    albedo: 0.22,
    building_density: 6.5,
    tree_canopy: 81.0,
    traffic_density: 220,
    population_density: 1200,
    severity: "LOW",
    temp_offset: -2.8,
    lst_offset: -4.5
  },
  {
    id: "MZ_002",
    name: "Whitefield ITPL Technology Corridor",
    type: "Commercial IT Park",
    lat: 12.9856,
    lng: 77.7312,
    ambient_temp: 33.2,
    lst: 36.4,
    humidity: 52.0,
    ndvi: 0.28,
    ndbi: 0.38,
    albedo: 0.21,
    building_density: 68.0,
    tree_canopy: 18.0,
    traffic_density: 2800,
    population_density: 14000,
    severity: "MODERATE",
    temp_offset: +1.5,
    lst_offset: +4.2
  },
  {
    id: "MZ_003",
    name: "Indiranagar 100ft Road Corridor",
    type: "Commercial & High Density",
    lat: 12.9719,
    lng: 77.6412,
    ambient_temp: 34.0,
    lst: 37.8,
    humidity: 50.0,
    ndvi: 0.22,
    ndbi: 0.42,
    albedo: 0.18,
    building_density: 74.0,
    tree_canopy: 14.0,
    traffic_density: 3400,
    population_density: 16500,
    severity: "MODERATE",
    temp_offset: +2.2,
    lst_offset: +5.5
  },
  {
    id: "MZ_004",
    name: "Majestic Intermodal Transit Concourse",
    type: "High-Footfall Transport Hub",
    lat: 12.9772,
    lng: 77.5714,
    ambient_temp: 37.2,
    lst: 42.6,
    humidity: 44.0,
    ndvi: 0.08,
    ndbi: 0.54,
    albedo: 0.12,
    building_density: 84.0,
    tree_canopy: 5.0,
    traffic_density: 4800,
    population_density: 22000,
    severity: "HIGH",
    temp_offset: +4.5,
    lst_offset: +9.2
  },
  {
    id: "MZ_005",
    name: "Peenya Industrial Manufacturing Belt",
    type: "Heavy Industrial & Low Albedo",
    lat: 13.0285,
    lng: 77.5197,
    ambient_temp: 38.1,
    lst: 44.5,
    humidity: 41.0,
    ndvi: 0.06,
    ndbi: 0.58,
    albedo: 0.11,
    building_density: 86.0,
    tree_canopy: 4.0,
    traffic_density: 3900,
    population_density: 18000,
    severity: "HIGH",
    temp_offset: +5.2,
    lst_offset: +11.0
  },
  {
    id: "MZ_006",
    name: "Koramangala 4th Block Mixed Zone",
    type: "Commercial & Residential",
    lat: 12.9352,
    lng: 77.6245,
    ambient_temp: 32.8,
    lst: 35.2,
    humidity: 54.0,
    ndvi: 0.32,
    ndbi: 0.34,
    albedo: 0.23,
    building_density: 62.0,
    tree_canopy: 22.0,
    traffic_density: 2400,
    population_density: 13000,
    severity: "MODERATE",
    temp_offset: +1.0,
    lst_offset: +3.0
  },
  {
    id: "MZ_007",
    name: "Electronic City Phase-1 Expressway",
    type: "High Tech & Highway Concourse",
    lat: 12.8452,
    lng: 77.6602,
    ambient_temp: 35.6,
    lst: 39.8,
    humidity: 47.0,
    ndvi: 0.18,
    ndbi: 0.46,
    albedo: 0.15,
    building_density: 72.0,
    tree_canopy: 11.0,
    traffic_density: 3800,
    population_density: 12000,
    severity: "HIGH",
    temp_offset: +3.4,
    lst_offset: +7.6
  },
  {
    id: "MZ_008",
    name: "Jayanagar 4th Block Boulevard",
    type: "Avenue Canopy Residential",
    lat: 12.9299,
    lng: 77.5824,
    ambient_temp: 29.8,
    lst: 30.5,
    humidity: 61.0,
    ndvi: 0.52,
    ndbi: 0.12,
    albedo: 0.26,
    building_density: 48.0,
    tree_canopy: 46.0,
    traffic_density: 1200,
    population_density: 9500,
    severity: "LOW",
    temp_offset: -0.5,
    lst_offset: +0.2
  }
];

// 5 Core Cooling Interventions Taxonomy
const INTERVENTIONS_CATALOG = {
  COOL_ROOF_ALBEDO: {
    id: "COOL_ROOF_ALBEDO",
    title: "High-Albedo Cool Roof Coating & Reflective Membrane",
    category: "Surface Modification",
    description: "Applies titanium dioxide elastomeric or thermoplastic white roof coatings (Solar Reflectance Index SRI > 80) that reflect solar radiation before heat absorption.",
    baseDropMin: 1.5,
    baseDropMax: 3.2,
    costPerSqm: 350,
    weeks: 3,
    targets: "Flat concrete roofs, industrial metal sheds, commercial building roofs"
  },
  URBAN_FORESTRY_CANOPY: {
    id: "URBAN_FORESTRY_CANOPY",
    title: "Intensive Urban Tree Canopy & Street Afforestation",
    category: "Green Infrastructure",
    description: "Deploys multi-tier native shade trees (Neem, Honge, Peepal) along road medians and pedestrian avenues to maximize cooling via evapotranspiration and shade.",
    baseDropMin: 2.2,
    baseDropMax: 4.8,
    costPerSqm: 600,
    weeks: 8,
    targets: "Road medians, pedestrian walkways, public park buffers"
  },
  GREEN_ROOFS_VERTICAL_GARDEN: {
    id: "GREEN_ROOFS_VERTICAL_GARDEN",
    title: "Extensive Green Roofs & Bio-Solar Living Walls",
    category: "Green Infrastructure",
    description: "Integrates lightweight modular sedum and succulent vegetative layers on rooftops and vertical living facades to insulate buildings and reduce sensible heat flux.",
    baseDropMin: 1.2,
    baseDropMax: 2.6,
    costPerSqm: 1200,
    weeks: 6,
    targets: "Commercial facades, RCC flat roofs, public transit walls"
  },
  PERMEABLE_COOL_PAVEMENTS: {
    id: "PERMEABLE_COOL_PAVEMENTS",
    title: "Permeable Interlocking Cool Pavements & Porous Asphalt",
    category: "Pavement Engineering",
    description: "Replaces standard impervious asphalt with porous concrete or interlocking pavers that retain subsurface moisture for evaporative cooling.",
    baseDropMin: 1.0,
    baseDropMax: 2.4,
    costPerSqm: 850,
    weeks: 5,
    targets: "Parking lots, bus terminals, pedestrian plazas, internal roads"
  },
  EVAPORATIVE_MISTING_WATER_PLAZA: {
    id: "EVAPORATIVE_MISTING_WATER_PLAZA",
    title: "Micro-Misting Canopies & Urban Water Retention Plazas",
    category: "Active Evaporative Cooling",
    description: "Installs high-pressure micro-nozzle misting canopies and daylighted retention fountains to rapidly lower localized ambient air temperatures by sensible heat absorption.",
    baseDropMin: 1.5,
    baseDropMax: 3.0,
    costPerSqm: 950,
    weeks: 4,
    targets: "Intermodal transit terminals, high-footfall civic squares, open pedestrian markets"
  }
};

// Global App State
const state = {
  activeTab: "tab-gis",
  selectedZone: MICRO_ZONES[3], // Default to Majestic
  map: null,
  markers: [],
  running: false,
  timer: null,
  serialPort: null,
  reader: null,
  packets: [],
  virtualPacket: 0,
  virtualPhase: 0,
  liveWeather: {
    temperature: 28.5,
    humidity: 58.0,
    apparent_temp: 29.5,
    radiation: 420.0,
    wind_speed: 12.0,
    timestamp: new Date()
  }
};

const $ = (id) => document.getElementById(id);

// --- FETCH LIVE WEATHER FROM OPEN-METEO API ---
async function fetchLiveWeather() {
  const url = "https://api.open-meteo.com/v1/forecast?latitude=12.9716&longitude=77.5946&current=temperature_2m,relative_humidity_2m,apparent_temperature,direct_radiation,wind_speed_10m,surface_pressure&timezone=auto";
  try {
    const response = await fetch(url);
    if (!response.ok) throw new Error(`Weather API Error: ${response.status}`);
    const data = await response.json();

    if (data && data.current) {
      state.liveWeather = {
        temperature: Number(data.current.temperature_2m),
        humidity: Number(data.current.relative_humidity_2m),
        apparent_temp: Number(data.current.apparent_temperature),
        radiation: Number(data.current.direct_radiation || 350.0),
        wind_speed: Number(data.current.wind_speed_10m || 10.0),
        timestamp: new Date()
      };

      const weatherText = `${state.liveWeather.temperature.toFixed(1)}°C | ${state.liveWeather.humidity.toFixed(0)}% RH | Bengaluru`;
      $("liveWeatherText").textContent = weatherText;
      $("liveWeatherPill").title = `Direct Solar Radiation: ${state.liveWeather.radiation} W/m² | Wind: ${state.liveWeather.wind_speed} km/h`;
    }
  } catch (err) {
    console.warn("Using offline meteorological fallback:", err);
    $("liveWeatherText").textContent = `${state.liveWeather.temperature.toFixed(1)}°C | ${state.liveWeather.humidity.toFixed(0)}% RH | Bengaluru (Cached)`;
  }
}

// --- SYNC ALL MICRO-ZONES WITH LIVE WEATHER ---
function syncMicroZonesWithLiveWeather() {
  const baseTemp = state.liveWeather.temperature;
  const baseHum = state.liveWeather.humidity;
  const solarFactor = Math.min(1.5, Math.max(0.6, state.liveWeather.radiation / 400.0));

  MICRO_ZONES.forEach(zone => {
    zone.ambient_temp = Math.round((baseTemp + zone.temp_offset) * 10) / 10;
    zone.humidity = Math.max(10, Math.min(95, Math.round(baseHum - (zone.temp_offset * 1.5))));
    zone.lst = Math.round((zone.ambient_temp + (zone.lst_offset * solarFactor)) * 10) / 10;

    // Recalculate Severity
    if (zone.lst >= 40.0 || zone.ambient_temp >= 36.5) {
      zone.severity = "HIGH";
    } else if (zone.lst >= 32.0 || zone.ambient_temp >= 30.5) {
      zone.severity = "MODERATE";
    } else {
      zone.severity = "LOW";
    }
  });

  // Re-render GIS map markers
  if (state.markers && state.markers.length) {
    state.markers.forEach((marker, idx) => {
      const zone = MICRO_ZONES[idx];
      const color = zone.severity === "HIGH" ? "#f43f5e" : zone.severity === "MODERATE" ? "#f59e0b" : "#10b981";
      marker.setStyle({ fillColor: color });
      marker.setTooltipContent(`<b>${zone.name}</b><br>Live Air: ${zone.ambient_temp}°C | Live LST: ${zone.lst}°C<br>Status: ${zone.severity}`);
    });
  }

  selectMicroZone(state.selectedZone);
  updateScenarioSimulation();
}

// --- TAB SWITCHING ---
function initTabs() {
  document.querySelectorAll(".tab-btn").forEach(btn => {
    btn.addEventListener("click", () => {
      document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));
      document.querySelectorAll(".tab-content").forEach(c => c.classList.remove("active"));
      btn.classList.add("active");
      const targetId = btn.getAttribute("data-tab");
      const content = $(targetId);
      if (content) content.classList.add("active");
      state.activeTab = targetId;

      if (targetId === "tab-gis" && state.map) {
        setTimeout(() => state.map.invalidateSize(), 150);
      }
      if (targetId === "tab-telemetry") {
        setTimeout(() => drawChart(), 150);
      }
    });
  });
}

// --- GIS LEAFLET MAP INITIALIZATION ---
function initGisMap() {
  const mapElement = $("gisMap");
  if (!mapElement) return;

  state.map = L.map("gisMap", {
    center: [12.9716, 77.5946],
    zoom: 12,
    zoomControl: true
  });

  // Dark CartoDB Tile Layer for rich aesthetics
  L.tileLayer("https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png", {
    attribution: '&copy; <a href="https://carto.com/">CartoDB</a> OpenStreetMap contributors',
    maxZoom: 19
  }).addTo(state.map);

  MICRO_ZONES.forEach(zone => {
    const color = zone.severity === "HIGH" ? "#f43f5e" : zone.severity === "MODERATE" ? "#f59e0b" : "#10b981";

    // Circle marker with pulse effect
    const circle = L.circleMarker([zone.lat, zone.lng], {
      radius: zone.severity === "HIGH" ? 14 : zone.severity === "MODERATE" ? 12 : 10,
      fillColor: color,
      color: "#ffffff",
      weight: 2,
      opacity: 0.9,
      fillOpacity: 0.75
    }).addTo(state.map);

    circle.bindTooltip(`<b>${zone.name}</b><br>Live Air: ${zone.ambient_temp}°C | Live LST: ${zone.lst}°C<br>Status: ${zone.severity}`, {
      direction: 'top',
      className: 'map-tooltip'
    });

    circle.on("click", () => selectMicroZone(zone));
    state.markers.push(circle);
  });

  selectMicroZone(state.selectedZone);

  // Sync Live Button listener
  const syncBtn = $("syncLiveWeatherBtn");
  if (syncBtn) {
    syncBtn.addEventListener("click", async () => {
      syncBtn.textContent = "⏳ Syncing...";
      await fetchLiveWeather();
      syncMicroZonesWithLiveWeather();
      syncBtn.textContent = "✅ Synced Live";
      setTimeout(() => { syncBtn.textContent = "🔄 Sync Live Weather"; }, 2000);
    });
  }
}

// --- MICRO-ZONE SELECTION & PRESCRIPTION CALCULATION ---
function selectMicroZone(zone) {
  state.selectedZone = zone;

  $("zoneTitle").textContent = zone.name;
  $("zoneLst").textContent = `${zone.lst.toFixed(1)} °C`;
  $("zoneAirTemp").textContent = `${zone.ambient_temp.toFixed(1)} °C`;
  $("zoneNdvi").textContent = zone.ndvi.toFixed(2);
  $("zoneNdbi").textContent = zone.ndbi.toFixed(2);
  $("zoneBuilt").textContent = `${zone.building_density.toFixed(0)} %`;
  $("zoneCanopy").textContent = `${zone.tree_canopy.toFixed(0)} %`;

  const badge = $("zoneSeverityBadge");
  badge.textContent = zone.severity;
  badge.className = `badge badge-${zone.severity.toLowerCase()}`;

  // Evaluate prescription using multi-criteria suitability
  const prescription = evaluatePrescription(zone);

  $("zoneRecTitle").textContent = prescription.strategy.title;
  $("zoneRecDesc").textContent = prescription.strategy.description;
  $("zoneExpDrop").textContent = `-${prescription.expectedDrop.toFixed(1)} °C`;
  $("zoneHvacSavings").textContent = `${prescription.hvacSavings.toFixed(1)} %`;
  $("zoneCost").textContent = `₹ ${prescription.strategy.costPerSqm} / m²`;
  $("zoneWeeks").textContent = `${prescription.strategy.weeks} Weeks`;
  $("zoneRationale").innerHTML = `<strong>Context Rationale:</strong> ${prescription.rationale}`;
}

function evaluatePrescription(zone) {
  let bestStrategyKey = "COOL_ROOF_ALBEDO";
  let rationale = "";

  if (zone.ndbi >= 0.45 && zone.albedo <= 0.18) {
    bestStrategyKey = "COOL_ROOF_ALBEDO";
    rationale = `High built-up index (NDBI ${zone.ndbi}) and low surface albedo (${zone.albedo}) indicate severe solar heat absorption. Applying high-reflectance cool coatings directly reflects solar radiation before absorption.`;
  } else if (zone.tree_canopy <= 15 || zone.ndvi <= 0.20) {
    bestStrategyKey = "URBAN_FORESTRY_CANOPY";
    rationale = `Extremely low vegetative canopy (${zone.tree_canopy}%) exacerbates micro-climate heat. Intensive native tree planting provides continuous shade and latent heat evapotranspiration.`;
  } else if (zone.traffic_density >= 3500) {
    bestStrategyKey = "EVAPORATIVE_MISTING_WATER_PLAZA";
    rationale = `High anthropogenic footfall (${zone.traffic_density} veh/hr) and dry ambient heat make high-pressure evaporative misting canopies optimal for rapid parcel cooling.`;
  } else if (zone.building_density >= 70) {
    bestStrategyKey = "GREEN_ROOFS_VERTICAL_GARDEN";
    rationale = `Dense building envelope (${zone.building_density}%) limits ground space. Installing modular green living roofs and vertical gardens insulates structures and reduces sensible heat flux.`;
  } else {
    bestStrategyKey = "PERMEABLE_COOL_PAVEMENTS";
    rationale = `Impervious parking and road surfaces store residual heat. Permeable interlocking cool pavers allow subsurface moisture retention and evaporative cooling.`;
  }

  const strategy = INTERVENTIONS_CATALOG[bestStrategyKey];
  const rawDrop = (strategy.baseDropMin + strategy.baseDropMax) * 0.5;
  // Non-linear diminishing return formula: ΔT_effective = 6.0 * (1 - exp(-0.22 * raw))
  const expectedDrop = 6.0 * (1.0 - Math.exp(-0.22 * rawDrop));
  const hvacSavings = expectedDrop * 2.2;

  return { strategy, expectedDrop, hvacSavings, rationale };
}

// --- WHAT-IF SCENARIO SIMULATOR ---
function initScenarioSimulator() {
  const sliders = ["sliderCanopy", "sliderAlbedo", "sliderGreenRoof", "sliderPermPave", "sliderMisting"];
  sliders.forEach(id => {
    const el = $(id);
    if (el) el.addEventListener("input", updateScenarioSimulation);
  });

  const zoneSelect = $("scenarioZoneSelect");
  if (zoneSelect) {
    zoneSelect.addEventListener("change", updateScenarioSimulation);
  }

  updateScenarioSimulation();
}

function updateScenarioSimulation() {
  const zoneId = $("scenarioZoneSelect").value;
  const baseline = MICRO_ZONES.find(z => z.id === zoneId) || MICRO_ZONES[3];

  const canopy = Number($("sliderCanopy").value);
  const albedo = Number($("sliderAlbedo").value);
  const greenRoof = Number($("sliderGreenRoof").value);
  const permPave = Number($("sliderPermPave").value);
  const misting = Number($("sliderMisting").value);

  $("valCanopy").textContent = `+${canopy} %`;
  $("valAlbedo").textContent = `+${albedo.toFixed(2)} Albedo`;
  $("valGreenRoof").textContent = `+${greenRoof} %`;
  $("valPermPave").textContent = `+${permPave} %`;
  $("valMisting").textContent = `+${misting} %`;

  // Raw component reductions
  const canopyDrop = canopy * 0.075;
  const albedoDrop = albedo * 6.5;
  const greenRoofDrop = greenRoof * 0.035;
  const permPaveDrop = permPave * 0.025;
  const mistingDrop = misting * 0.025;

  const rawPotential = canopyDrop + albedoDrop + greenRoofDrop + permPaveDrop + mistingDrop;
  // Non-linear effective drop
  const effectiveDrop = 6.0 * (1.0 - Math.exp(-0.22 * rawPotential));
  const hvacSavings = Math.min(25.0, effectiveDrop * 2.2);

  const area = 50000; // 50,000 m² (5 hectares standard zone)
  const canopyCost = (canopy / 100) * area * 600;
  const albedoCost = (albedo > 0.05 ? 1 : 0) * (area * (baseline.building_density / 100) * 0.6) * 350;
  const greenRoofCost = (greenRoof / 100) * (area * (baseline.building_density / 100)) * 1200;
  const permPaveCost = (permPave / 100) * (area * 0.35) * 850;
  const mistingCost = (misting / 100) * (area * 0.10) * 950;
  const totalCostLakhs = (canopyCost + albedoCost + greenRoofCost + permPaveCost + mistingCost) / 100000;

  const annualKwhSaved = area * (baseline.building_density / 100) * 45 * (hvacSavings / 100);
  const co2Tons = (annualKwhSaved * 0.82) / 1000;

  $("simTempDrop").textContent = `-${effectiveDrop.toFixed(1)} °C`;
  $("simHvacSavings").textContent = `${hvacSavings.toFixed(1)} %`;
  $("simBudget").textContent = `₹ ${totalCostLakhs.toFixed(1)} Lakhs`;
  $("simCarbon").textContent = `${co2Tons.toFixed(0)} Tons/Yr`;

  renderScenarioTable(baseline, effectiveDrop, totalCostLakhs, hvacSavings);
}

function renderScenarioTable(baseline, customDrop, customCost, customSavings) {
  const tableBody = $("scenarioTableBody");
  if (!tableBody) return;

  const scenarios = [
    {
      name: "Baseline (No Action)",
      temp: baseline.ambient_temp,
      drop: 0.0,
      rfClass: baseline.severity,
      budget: "₹ 0.0 Lakhs",
      hvac: "0.0 %"
    },
    {
      name: "Scenario A: High-Albedo Cool Roofs Only",
      temp: baseline.ambient_temp - 2.1,
      drop: 2.1,
      rfClass: baseline.severity === "HIGH" ? "MODERATE" : "LOW",
      budget: "₹ 10.5 Lakhs",
      hvac: "4.6 %"
    },
    {
      name: "Scenario B: Intensive Miyawaki Tree Canopy",
      temp: baseline.ambient_temp - 2.8,
      drop: 2.8,
      rfClass: "MODERATE",
      budget: "₹ 15.0 Lakhs",
      hvac: "6.2 %"
    },
    {
      name: "Scenario C: Custom Multi-Tier (Current Sliders)",
      temp: baseline.ambient_temp - customDrop,
      drop: customDrop,
      rfClass: customDrop >= 3.0 ? "LOW" : "MODERATE",
      budget: `₹ ${customCost.toFixed(1)} Lakhs`,
      hvac: `${customSavings.toFixed(1)} %`
    }
  ];

  tableBody.innerHTML = scenarios.map(s => `
    <tr>
      <td><strong>${s.name}</strong></td>
      <td>${s.temp.toFixed(1)} °C</td>
      <td><span style="color:var(--accent-emerald)">-${s.drop.toFixed(1)} °C</span></td>
      <td><span class="badge badge-${s.rfClass.toLowerCase()}">${s.rfClass}</span></td>
      <td>${s.budget}</td>
      <td><strong>${s.hvac}</strong></td>
    </tr>
  `).join("");
}

// --- LIVE TELEMETRY & DATA SOURCES ---
function initTelemetry() {
  const sourceSelect = $("sourceSelect");
  const startButton = $("startButton");
  const connectButton = $("connectButton");
  const stopButton = $("stopButton");
  const intervalInput = $("intervalInput");

  sourceSelect.addEventListener("change", () => {
    const val = sourceSelect.value;
    const isSerial = val === "serial";
    connectButton.hidden = !isSerial;
    startButton.hidden = isSerial;
    intervalInput.disabled = isSerial;

    if (val === "live_api") {
      setConnection("Live Bengaluru Open-Meteo Feed Ready", false);
    } else if (isSerial) {
      setConnection("ESP32 ready to connect", false);
    } else {
      setConnection("Virtual C Standby", false);
    }
  });

  startButton.addEventListener("click", () => {
    if (sourceSelect.value === "live_api") {
      startLiveApiStream();
    } else {
      startVirtualStream();
    }
  });

  stopButton.addEventListener("click", stopStream);
  connectButton.addEventListener("click", connectSerial);
  $("clearButton").addEventListener("click", () => { state.packets = []; renderPacketTable(); });
}

function setConnection(text, connected) {
  $("connectionText").textContent = text;
  $("connectionPill").classList.toggle("connected", connected);
}

// --- LIVE OPEN-METEO DATA STREAM ---
function startLiveApiStream() {
  stopStream();
  state.running = true;
  state.virtualPacket = 0;
  setConnection("Live Open-Meteo Bengaluru Stream Active", true);
  tickLiveApiStream();
  const interval = Math.max(1000, Number($("intervalInput").value) || 3000);
  state.timer = setInterval(tickLiveApiStream, interval);
}

async function tickLiveApiStream() {
  state.virtualPacket += 1;
  await fetchLiveWeather();
  syncMicroZonesWithLiveWeather();

  const selectedZone = state.selectedZone || MICRO_ZONES[3];
  // Micro-variation jitter for natural sensor simulation
  const noise = (Math.random() - 0.5) * 0.4;
  const temp = selectedZone.ambient_temp + noise;
  const hum = selectedZone.humidity + (Math.random() - 0.5) * 0.8;
  const lst = selectedZone.lst + (Math.random() - 0.5) * 0.6;
  const heatIndex = temp + (hum * 0.1);

  const rfClass = temp >= 36.5 || heatIndex >= 38.0 ? "HIGH" : temp >= 31.0 || heatIndex >= 33.0 ? "MODERATE" : "LOW";
  const confidence = 0.88 + 0.10 * Math.random();

  const rec = rfClass === "HIGH" ? "COOL_ROOF_ALBEDO" : rfClass === "MODERATE" ? "URBAN_FORESTRY_CANOPY" : "NONE";
  const drop = rfClass === "HIGH" ? 3.2 : rfClass === "MODERATE" ? 2.4 : 0.0;

  applyPacket({
    packet_id: state.virtualPacket,
    node_id: "LIVE_BENGALURU_OPENMETEO",
    ambient_temp_c: temp,
    relative_humidity_pct: hum,
    lst_c: lst,
    heat_index_c: heatIndex,
    severity: rfClass,
    rf_class: rfClass,
    rf_confidence: confidence,
    thermal_stress_index: 45.0 + (lst - 30.0) * 2.0,
    expected_drop_c: drop,
    hvac_savings_pct: drop * 2.2,
    recommendation: rec,
    green_led: rfClass === "LOW",
    yellow_led: rfClass === "MODERATE",
    red_led: rfClass === "HIGH",
    buzzer: temp >= 38.0,
    uptime_ms: state.virtualPacket * 1000
  });
}

// --- VIRTUAL C SENSOR STREAM ---
function startVirtualStream() {
  stopStream();
  state.running = true;
  state.virtualPacket = 0;
  state.virtualPhase = 0;
  setConnection("Virtual C Stream Active (1.0 Hz)", true);
  tickVirtualStream();
  state.timer = setInterval(tickVirtualStream, Math.max(250, Number($("intervalInput").value) || 1000));
}

function tickVirtualStream() {
  state.virtualPacket += 1;
  state.virtualPhase += 0.45;

  const temp = 32.0 + 4.5 * Math.sin(state.virtualPhase);
  const hum = 58.0 - 12.0 * Math.sin(state.virtualPhase);
  const lst = temp + 7.5 + 1.5 * Math.max(0, Math.sin(state.virtualPhase));
  const heatIndex = temp + (hum * 0.1);

  const rfClass = temp >= 36.5 || heatIndex >= 38.0 ? "HIGH" : temp >= 31.0 || heatIndex >= 33.0 ? "MODERATE" : "LOW";
  const confidence = 0.88 + 0.10 * Math.random();

  const rec = rfClass === "HIGH" ? "COOL_ROOF_ALBEDO" : rfClass === "MODERATE" ? "URBAN_FORESTRY_CANOPY" : "NONE";
  const drop = rfClass === "HIGH" ? 3.2 : rfClass === "MODERATE" ? 2.4 : 0.0;

  applyPacket({
    packet_id: state.virtualPacket,
    node_id: "VIRTUAL_UHI_NODE_C",
    ambient_temp_c: temp,
    relative_humidity_pct: hum,
    lst_c: lst,
    heat_index_c: heatIndex,
    severity: rfClass,
    rf_class: rfClass,
    rf_confidence: confidence,
    thermal_stress_index: 45.0 + 35.0 * Math.sin(state.virtualPhase),
    expected_drop_c: drop,
    hvac_savings_pct: drop * 2.2,
    recommendation: rec,
    green_led: rfClass === "LOW",
    yellow_led: rfClass === "MODERATE",
    red_led: rfClass === "HIGH",
    buzzer: temp >= 38.0,
    uptime_ms: state.virtualPacket * 1000
  });
}

async function connectSerial() {
  if (!("serial" in navigator)) {
    alert("Web Serial API is not supported in this browser. Please use Chrome or Edge.");
    return;
  }
  try {
    state.serialPort = await navigator.serial.requestPort();
    await state.serialPort.open({ baudRate: 115200 });
    setConnection("ESP32 DevKit V1 Connected", true);
    readSerial();
  } catch (err) {
    console.error(err);
    setConnection("Connection Cancelled", false);
  }
}

async function readSerial() {
  const decoder = new TextDecoderStream();
  state.serialPort.readable.pipeTo(decoder.writable);
  state.reader = decoder.readable.getReader();
  let buffer = "";

  try {
    while (state.reader) {
      const { value, done } = await state.reader.read();
      if (done) break;
      buffer += value;
      const lines = buffer.split("\n");
      buffer = lines.pop();
      lines.forEach(line => {
        try {
          if (line.trim()) applyPacket(JSON.parse(line));
        } catch (_) {}
      });
    }
  } catch (err) {
    console.error(err);
    setConnection("Serial Disconnected", false);
  }
}

async function stopStream() {
  state.running = false;
  if (state.timer) { clearInterval(state.timer); state.timer = null; }
  if (state.reader) { await state.reader.cancel().catch(() => {}); state.reader = null; }
  if (state.serialPort) { await state.serialPort.close().catch(() => {}); state.serialPort = null; }
  setConnection("Standby", false);
}

function applyPacket(packet) {
  state.packets.unshift(packet);
  state.packets = state.packets.slice(0, 15);

  $("temperatureValue").textContent = packet.ambient_temp_c.toFixed(1);
  $("humidityValue").textContent = `${packet.relative_humidity_pct.toFixed(1)} %`;
  $("lstValue").textContent = packet.lst_c ? `${packet.lst_c.toFixed(1)} °C` : "--.- °C";
  $("heatIndexValue").textContent = packet.heat_index_c ? `${packet.heat_index_c.toFixed(1)} °C` : "--.- °C";
  $("uptimeValue").textContent = packet.uptime_ms ? `${Math.round(packet.uptime_ms / 1000)} s` : "-- s";

  const sev = packet.severity || "LOW";
  $("severityValue").textContent = sev;
  const sevPanel = $("severityPanel");
  sevPanel.className = `card alert-card ${sev.toLowerCase()}`;
  $("severityMeter").style.width = sev === "HIGH" ? "100%" : sev === "MODERATE" ? "60%" : "25%";

  $("rfClassValue").textContent = packet.rf_class || sev;
  $("rfConfValue").textContent = packet.rf_confidence ? `${Math.round(packet.rf_confidence * 100)}%` : "--%";

  // Update LEDs
  setLed("greenLed", "greenState", packet.green_led);
  setLed("yellowLed", "yellowState", packet.yellow_led);
  setLed("redLed", "redState", packet.red_led);

  const buzzer = Boolean(packet.buzzer);
  $("buzzerState").classList.toggle("on", buzzer);
  $("buzzerText").textContent = buzzer ? "ACTIVE ALARM" : "OFF";

  $("packetCount").textContent = `${state.packets.length} telemetry packet${state.packets.length === 1 ? '' : 's'} received`;
  $("lastUpdated").textContent = `Updated ${new Date().toLocaleTimeString()}`;

  renderPacketTable();
  drawChart();
}

function setLed(bulbId, textId, stateVal) {
  $(bulbId).classList.toggle("on", Boolean(stateVal));
  $(textId).textContent = stateVal ? "ON" : "OFF";
}

function renderPacketTable() {
  const tbody = $("packetTable");
  if (!state.packets.length) {
    tbody.innerHTML = '<tr><td colspan="8" class="empty">Awaiting sensor telemetry packets...</td></tr>';
    return;
  }

  tbody.innerHTML = state.packets.map(p => `
    <tr>
      <td>#${p.packet_id || '--'}</td>
      <td>${p.ambient_temp_c ? p.ambient_temp_c.toFixed(1) + ' °C' : '--'}</td>
      <td>${p.relative_humidity_pct ? p.relative_humidity_pct.toFixed(1) + ' %' : '--'}</td>
      <td>${p.lst_c ? p.lst_c.toFixed(1) + ' °C' : '--'}</td>
      <td><span class="badge badge-${(p.rf_class || p.severity || 'low').toLowerCase()}">${p.rf_class || p.severity || '--'}</span></td>
      <td>${p.recommendation || '--'}</td>
      <td><span style="color:var(--accent-emerald)">-${p.expected_drop_c ? p.expected_drop_c.toFixed(1) : '0.0'} °C</span></td>
      <td>${new Date().toLocaleTimeString()}</td>
    </tr>
  `).join("");
}

// Chart rendering
function drawChart() {
  const canvas = $("temperatureChart");
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  const rect = canvas.getBoundingClientRect();
  const scale = window.devicePixelRatio || 1;

  canvas.width = rect.width * scale;
  canvas.height = 240 * scale;
  ctx.scale(scale, scale);

  const width = rect.width;
  const height = 240;
  ctx.clearRect(0, 0, width, height);

  // Grid lines
  ctx.strokeStyle = "rgba(255, 255, 255, 0.06)";
  ctx.lineWidth = 1;
  for (let y = 30; y < height; y += 45) {
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }

  if (!state.packets.length) return;

  const values = state.packets.slice().reverse().map(p => p.ambient_temp_c);
  const min = Math.min(...values) - 1.5;
  const max = Math.max(...values) + 1.5;

  // Gradient area fill
  const gradient = ctx.createLinearGradient(0, 0, 0, height);
  gradient.addColorStop(0, "rgba(6, 182, 212, 0.35)");
  gradient.addColorStop(1, "rgba(6, 182, 212, 0.0)");

  ctx.beginPath();
  values.forEach((v, i) => {
    const x = values.length === 1 ? width / 2 : (i / (values.length - 1)) * width;
    const y = height - 25 - ((v - min) / (max - min)) * (height - 50);
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  });
  ctx.lineTo(width, height);
  ctx.lineTo(0, height);
  ctx.closePath();
  ctx.fillStyle = gradient;
  ctx.fill();

  // Line stroke
  ctx.strokeStyle = "#06b6d4";
  ctx.lineWidth = 3;
  ctx.beginPath();
  values.forEach((v, i) => {
    const x = values.length === 1 ? width / 2 : (i / (values.length - 1)) * width;
    const y = height - 25 - ((v - min) / (max - min)) * (height - 50);
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  });
  ctx.stroke();

  // Data point dots
  ctx.fillStyle = "#ffffff";
  values.forEach((v, i) => {
    const x = values.length === 1 ? width / 2 : (i / (values.length - 1)) * width;
    const y = height - 25 - ((v - min) / (max - min)) * (height - 50);
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, Math.PI * 2);
    ctx.fill();
  });
}

// --- 5-COOLING INTERVENTIONS CATALOG RENDER ---
function renderCatalog() {
  const container = $("catalogCards");
  if (!container) return;

  container.innerHTML = Object.values(INTERVENTIONS_CATALOG).map(c => `
    <div class="catalog-card">
      <span class="card-tag">${c.category}</span>
      <h3>${c.title}</h3>
      <p>${c.description}</p>
      <div class="catalog-meta-grid">
        <div><span>Expected Cooling ΔT</span><strong>-${c.baseDropMin} to -${c.baseDropMax} °C</strong></div>
        <div><span>Unit CapEx</span><strong>₹ ${c.costPerSqm} / m²</strong></div>
        <div><span>Execution Duration</span><strong>${c.weeks} Weeks</strong></div>
        <div><span>Target Surfaces</span><strong style="font-size:0.75rem">${c.targets}</strong></div>
      </div>
    </div>
  `).join("");
}

// --- MUNICIPAL ACTION PLAN EXPORT ---
function initExport() {
  const btn = $("exportReportBtn");
  if (!btn) return;

  btn.addEventListener("click", () => {
    const plan = {
      system_name: "Smart Cooling Strategy Recommendation System for Urban Heat Island Reduction",
      export_type: "Municipal Heat Mitigation & Intervention Action Plan",
      export_timestamp: new Date().toISOString(),
      live_meteorological_baseline: state.liveWeather,
      micro_zone_assessments: MICRO_ZONES.map(zone => {
        const evalRes = evaluatePrescription(zone);
        return {
          zone_id: zone.id,
          zone_name: zone.name,
          type: zone.type,
          coordinates: { lat: zone.lat, lng: zone.lng },
          thermal_parameters: {
            lst_c: zone.lst,
            ambient_air_temp_c: zone.ambient_temp,
            humidity_pct: zone.humidity,
            ndvi: zone.ndvi,
            ndbi: zone.ndbi,
            building_density_pct: zone.building_density,
            canopy_pct: zone.tree_canopy
          },
          rf_severity_tier: zone.severity,
          prescribed_intervention: {
            strategy_code: evalRes.strategy.id,
            strategy_title: evalRes.strategy.title,
            category: evalRes.strategy.category,
            predicted_temp_drop_c: evalRes.expectedDrop,
            hvac_energy_savings_pct: evalRes.hvacSavings,
            cost_per_sqm_inr: evalRes.strategy.costPerSqm,
            timeline_weeks: evalRes.strategy.weeks,
            context_rationale: evalRes.rationale
          }
        };
      })
    };

    const blob = new Blob([JSON.stringify(plan, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `Municipal_Cooling_Action_Plan_${new Date().toISOString().slice(0,10)}.json`;
    a.click();
    URL.revokeObjectURL(url);
  });
}

// --- BOOTSTRAP ---
document.addEventListener("DOMContentLoaded", async () => {
  initTabs();
  initGisMap();
  initScenarioSimulator();
  initTelemetry();
  renderCatalog();
  initExport();
  window.addEventListener("resize", drawChart);

  // Auto-fetch Live Bengaluru Weather on load
  await fetchLiveWeather();
  syncMicroZonesWithLiveWeather();
});
