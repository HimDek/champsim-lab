# Hybrid sBerti for ChampSim

This package starts from the supplied `sberti_ampm.h/.cc` implementation and adds compile-time switches for Berti, AMPM, global-delta learning, PC-path signatures, and a Bloom-filter duplicate filter.

## Files
- `hybrid_berti.h` — prefetcher declarations and state
- `hybrid_berti.cc` — implementation
- `hybrid_berti_config.h` — feature switches

## Configuration
Edit `hybrid_berti_config.h`, then rebuild ChampSim.

```c
#define ENABLE_SBERTI_BERTI       1
#define ENABLE_AMPM               1
#define ENABLE_SMART_STRIDE       0
#define ENABLE_GBERTI_GLOBAL      0
#define ENABLE_BERTIGO_PC_PATH    0
#define ENABLE_BLOOM_FILTER       0
#define HYBRID_RECENT_DEDUP       1
```

Recommended experiment order: baseline (all optional switches off), then enable `ENABLE_GBERTI_GLOBAL`, then `ENABLE_BERTIGO_PC_PATH`, then `ENABLE_BLOOM_FILTER`, and finally combinations.

## Build integration
Use module name `hybrid_berti` in your ChampSim configuration and compile `hybrid_berti.cc` as the prefetcher source. Keep the file names and class/module name consistent with your ChampSim version.

## Important notes
- `ENABLE_GBERTI_GLOBAL` enables a compact global-delta learner implemented here; it is an integration experiment, not a line-for-line port of the supplied gBerti implementation.
- `ENABLE_BERTIGO_PC_PATH` changes the global-delta context key to include a rolling four-PC signature. It does not add a separate full BertiGo predictor.
- `ENABLE_BLOOM_FILTER` enables a 16,384-bit Bloom filter on the shared issue path. Bloom false positives can suppress a useful prefetch; evaluate it separately.
- `ENABLE_SMART_STRIDE` is exposed as a configuration flag, but the supplied `sberti_ampm.cc` does not contain the Smart Stride implementation/call path. It therefore remains disabled and is not activated by this package.
- The source is based on the supplied sBerti+AMPM variant. Compile against the same ChampSim API/version as the original source.
