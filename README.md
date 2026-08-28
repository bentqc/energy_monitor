# Energy Monitor

ESPHome-based multi-breaker AC current monitor using CT clamps + ADS1115
ADCs on an ESP32-S3.

## Architecture

- **ESP32-S3-N16R8**, networked via **W5500 Ethernet** (SPI).
- **3x ADS1115** 16-bit ADCs on one shared I2C bus (addresses `0x48`,
  `0x49`, `0x4A` — set via each board's ADDR pin), giving 12 channels total
  (10 used, 2 spare).
- One **CT clamp per breaker**, each secondary across a 22Ω burden
  resistor, biased to the shared Vmid node (~half the ADC supply voltage).
- ESPHome's built-in [`ads1115`](https://esphome.io/components/sensor/ads1115.html)
  + [`ct_clamp`](https://esphome.io/components/sensor/ct_clamp.html) sensor
  platforms do all the RMS current math natively — **no custom component
  or `external_components:` is needed.**

There used to be a custom `ads1115_rms` component in this repo. It was
removed: its ESPHome codegen didn't match its C++ constructor (wouldn't
compile) and its sensor pointer was never initialized (would crash even if
it had compiled). ESPHome's built-in `ct_clamp` platform does the same job
correctly and is officially maintained.

**If your device YAML still has this block, delete it — it's no longer
needed and points at a component this repo no longer ships:**

```yaml
external_components:
  - source: github://bentqc/energy_monitor
    components:
      - ads1115_rms
```

## Using `energy_monitor.yaml`

1. Copy `secrets.yaml.example` to `secrets.yaml` and fill in your own
   values (never commit `secrets.yaml`).
2. Edit `energy_monitor.yaml`:
   - Fix the `i2c:` `sda`/`scl` pins and the `ethernet:` SPI pins
     (`clk_pin`, `mosi_pin`, `miso_pin`, `cs_pin`, `interrupt_pin`,
     `reset_pin`) to match your actual wiring — the values in the file are
     placeholders.
   - Rename each `"Breaker N Current"` sensor to match your panel.
3. Compile/flash with ESPHome as usual, e.g.:
   ```
   esphome run energy_monitor.yaml
   ```

### Why Vmid doesn't need to be configured in software

`ct_clamp`'s RMS formula is `sqrt(mean(v²) − mean(v)²)`, which subtracts
out the signal's own average automatically. As long as your bias network
keeps the signal within the ADC's input range, the exact Vmid voltage
doesn't need to be entered anywhere in the YAML.

### Calibrating each channel

Each `ct_clamp` sensor has a placeholder `calibrate_linear` filter:

```yaml
filters:
  - calibrate_linear:
      - 0.0 -> 0.0
      - 1.0 -> 1.0
```

**Important: until you replace the second line, the logged/displayed value
is NOT amps.** With the 1.0 -> 1.0 placeholder, `calibrate_linear` is an
identity function, so what you see is the raw RMS *voltage* across the
burden resistor, just mislabeled with an "A" unit in the log line. Don't
trust the number as real current until a channel has been calibrated.

To calibrate a channel:

1. Put a **known load** on that breaker (e.g. a 4A heater/lamp) and turn
   it on. A plug-in power meter (Kill-A-Watt or similar, ~$15-25) is the
   easiest source of a trustworthy reference current — it reads true RMS
   amps/watts directly off the appliance, no math needed.
2. Watch the raw (uncalibrated) reading for that channel in the ESPHome
   logs or the Home Assistant entity.
3. Replace the second line with `<raw_value> -> <known_amps>`, e.g. if the
   raw reading was `0.62` for a real 4.0A load: `0.62 -> 4.0`.
4. Repeat per channel — burden resistor tolerance and CT turns ratio vary
   slightly between channels, so each one should be calibrated
   individually for best accuracy.

### Validating a channel with a multimeter

A plain (non-clamp) multimeter can't safely measure line current directly —
that requires breaking the live circuit to insert an ammeter in series,
which isn't worth the shock/arc-flash risk on a breaker panel. It's still
useful for two checks:

- **DC bias sanity check**: with nothing clamped and the CT disconnected,
  measure DC volts from the ADS1115 input pin to ground. It should read a
  stable value at (or near) your Vmid bias point (e.g. ~1.65V on a 3.3V
  reference) with no big swings. A reading stuck at 0V or the rail
  suggests a bias-network wiring fault, not just an unclamped CT.
- **AC ripple cross-check**: set the multimeter to AC volts and measure
  across the burden resistor while the CT is clamped on a live wire. You
  should see a small AC voltage that rises/falls with load — if it stays
  flat while the ESPHome reading changes (or vice versa), suspect a bad
  connection between the CT, burden resistor, and ADS1115 input.

If you have (or can borrow) a **clamp meter**, that's the best validation
tool: clamp it on the exact same wire as your CT sensor and compare its
reading directly against the raw ESPHome value — this also gives you the
known-load numbers needed for step 3 above without plugging in any
appliance.

### Unused / not-yet-wired channels read a non-zero "current"

If a channel's ADS1115 input pin isn't wired to anything yet (floating),
it will pick up ambient 50/60Hz noise from the other AC wiring inside the
panel and settle on some small nonzero RMS value (commonly in the
0.1-0.5V-equivalent range) — that's expected electrical behavior for an
open analog input, not a bug. It goes away once a real CT clamp and bias
network are wired to that input. Until then, either ignore that channel's
reading or set `disabled_by_default: true` on it so it doesn't clutter
your Home Assistant dashboard.

## Safety

- **Never leave a CT secondary open** while its primary conductor is
  energized — short the two leads together before unplugging anything.
- CT secondaries must always have a burden resistor (or short) across
  them; this is what makes the induced current safe to handle.
