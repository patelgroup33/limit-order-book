# Order Book Visualizer

A self-contained (no dependencies, no build step) web replay of the matching
engine. It renders a live L2 order-book ladder, a cumulative market-depth chart,
a time-&-sales tape, and a price sparkline, with play/pause/scrub/speed controls.

The data is produced by the C++ simulator (`lob_sim`), which drives the real
matching engine with synthetic order flow and writes `simulation.json`.

## Run it locally

The page loads `simulation.json` via `fetch`, so it must be served over HTTP
(opening `index.html` as a `file://` URL won't work):

```bash
# from the repo root: build the simulator and (re)generate the data
cmake --build build --target lob_sim
./build/sim/lob_sim 1200 web/simulation.json

# serve this folder and open http://localhost:8137
cd web && python3 -m http.server 8137
```

## Regenerating the data

```bash
./build/sim/lob_sim <steps> web/simulation.json
```

The run is deterministic (seeded), so the same arguments always produce the same
replay. Tune the market in `sim/order_flow_simulator.hpp` (`Config`: start price,
depth, seed, warmup, drift).

## Hosting (GitHub Pages)

`.github/workflows/pages.yml` publishes this `web/` folder to GitHub Pages on
every push to `main`. Enable it once under **Settings → Pages → Source: GitHub
Actions**. The demo then lives at:

```
https://patelgroup33.github.io/limit-order-book/
```

## Schema

`simulation.json` (positional arrays keep it compact):

```json
{
  "meta": { "instrument": "SIM", "price_scale": 100, "depth": 12, "frames": 1200, "start_mid": 10000 },
  "frames": [
    { "t": 0, "last": 10000,
      "bids": [[price, size, orders], ...],   // best (highest) first
      "asks": [[price, size, orders], ...],   // best (lowest) first
      "trades": [[price, size, "B"|"S"], ...] }
  ]
}
```

Prices are integer ticks; divide by `price_scale` for display.
