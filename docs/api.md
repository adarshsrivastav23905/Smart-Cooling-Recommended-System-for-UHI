# Project REST API

The dashboard's Python server also exposes a small local REST API. It uses the
processed zone dataset and intervention catalog already included in the
repository; no additional package is required.

## Start the API

From the repository root:

```powershell
python dashboard\server.py
```

The server listens on `http://127.0.0.1:8000`. API responses are JSON. The
server is bound to localhost and is intended for local demonstrations, not
public deployment.

## Endpoints

### Health

`GET /api/health`

Returns server readiness and planner mode.

```json
{
  "ready": true,
  "mode": "local",
  "model": "Local rules"
}
```

### List micro-zones

`GET /api/v1/zones`

Returns mean numeric profiles grouped by `zone_id` from
`dataset/processed_microzone_data.csv`, plus the record count and severity-label
counts for each zone.

### Get a micro-zone

`GET /api/v1/zones/{zone_id}`

Example: `GET /api/v1/zones/MZ_001`. Returns one aggregated zone profile or
`404` for an unknown identifier.

### List strategies

`GET /api/v1/strategies`

Returns the five strategies from `recommendation_engine/intervention_catalog.json`.

### Get a recommendation

`POST /api/v1/recommendations`

Provide a known `zone_id`:

```json
{
  "zone_id": "MZ_004"
}
```

Alternatively, supply a feature profile directly:

```json
{
  "profile": {
    "ndbi": 0.54,
    "albedo": 0.12,
    "tree_canopy_cover": 5,
    "ndvi": 0.08,
    "traffic_density": 4800,
    "building_density": 84
  }
}
```

The response includes a recommendation ID, catalog entry, and
`"mode": "local_rules"`. This is a deterministic rules-based recommendation,
not a model inference or guaranteed site outcome.

### Estimate a scenario

`POST /api/v1/scenarios`

Required: `zone_id`. Optional controls and ranges:

| Field | Default | Allowed range |
|---|---:|---:|
| `canopy_pct` | 0 | 0–50 |
| `albedo_delta` | 0 | 0–0.5 |
| `green_roof_pct` | 0 | 0–60 |
| `permeable_pavement_pct` | 0 | 0–80 |
| `misting_pct` | 0 | 0–100 |
| `area_sqm` | 50,000 | 1–10,000,000 |

Example:

```json
{
  "zone_id": "MZ_004",
  "canopy_pct": 25,
  "albedo_delta": 0.3,
  "green_roof_pct": 15,
  "permeable_pavement_pct": 20,
  "misting_pct": 10,
  "area_sqm": 50000
}
```

The response reports estimated temperature drop, budget, energy savings, and
carbon impact. These are dashboard-formula estimates, not field measurements
or a live execution of the native C engine. Invalid values return `400`;
unknown zones return `404`.

### Existing planner endpoint

`POST /api/ai-recommendation` remains available for the dashboard's cooling
planner. Without `OPENAI_API_KEY`, it returns a local rules-based response.
With a key, the request is proxied to the configured OpenAI model.

## Errors and limits

Errors use a JSON object with an `error` string. Request bodies must be JSON
objects and are limited to 16 KB. The planner question is limited to 1,200
characters. API calls are local and unauthenticated; do not expose this server
to an untrusted network without adding authentication, rate limits, and
deployment hardening.

## Quick smoke tests

```powershell
Invoke-RestMethod http://127.0.0.1:8000/api/v1/zones
Invoke-RestMethod http://127.0.0.1:8000/api/v1/zones/MZ_001
Invoke-RestMethod http://127.0.0.1:8000/api/v1/strategies
```

For POST requests, pass a JSON body with `Invoke-RestMethod -Method Post
-ContentType 'application/json' -Body $json`.
