import json
import argparse
import pandas as pd
import numpy as np


def counter(stats, request_type, counter):
    """Extract a ChampSim counter, handling array-valued counters."""
    value = stats.get(request_type, {}).get(counter, [0])

    if isinstance(value, list):
        return sum(value)

    return value or 0


def calculate_miss_rate(stats, request_type):
    hits = counter(stats, request_type, "hit")
    misses = counter(stats, request_type, "miss")
    merges = counter(stats, request_type, "miss_merge")

    total = hits + misses
    total_merged = total + merges

    return {
        "Hits": hits,
        "Misses": misses,
        "Miss Merges": merges,
        "Miss Rate (%)": (
            100 * misses / total if total else np.nan
        ),
        "Miss Rate incl. Merges (%)": (
            100 * (misses + merges) / total_merged
            if total_merged else np.nan
        ),
    }


def cache_miss_matrix(sim):
    cache_names = [
        "cpu0_L1D",
        "cpu0_L2C",
        "LLC",
    ]

    request_types = [
        "LOAD",
        "PREFETCH",
        "RFO",
        "WRITE",
        "TRANSLATION",
    ]

    rows = []

    for cache in cache_names:
        stats = sim.get(cache, {})
        cache_rows = []

        for request_type in request_types:
            result = calculate_miss_rate(stats, request_type)

            row = {
                "Cache": cache,
                "Request Type": request_type,
                **result,
            }

            rows.append(row)
            cache_rows.append(result)

        # Aggregate counters before calculating total rates.
        hits = sum(r["Hits"] for r in cache_rows)
        misses = sum(r["Misses"] for r in cache_rows)
        merges = sum(r["Miss Merges"] for r in cache_rows)

        total = hits + misses
        total_merged = total + merges

        rows.append({
            "Cache": cache,
            "Request Type": "TOTAL",
            "Hits": hits,
            "Misses": misses,
            "Miss Merges": merges,
            "Miss Rate (%)": (
                100 * misses / total if total else np.nan
            ),
            "Miss Rate incl. Merges (%)": (
                100 * (misses + merges) / total_merged
                if total_merged else np.nan
            ),
        })

    return pd.DataFrame(rows)


def prefetcher_matrix(sim):
    cache_names = [
        "cpu0_L1D",
        "cpu0_L2C",
        "LLC",
    ]

    rows = []

    for cache in cache_names:
        stats = sim.get(cache, {})

        issued = stats.get("prefetch issued", 0)
        requested = stats.get("prefetch requested", 0)
        useful = stats.get("useful prefetch", 0)
        useless = stats.get("useless prefetch", 0)

        rows.append({
            "Cache": cache,
            "Issued": issued,
            "Requested": requested,
            "Useful": useful,
            "Useless": useless,
            "Useful / Issued (%)": (
                100 * useful / issued if issued else np.nan
            ),
            "Useless / Issued (%)": (
                100 * useless / issued if issued else np.nan
            ),
            "Useful / Requested (%)": (
                100 * useful / requested if requested else np.nan
            ),
            "Issued / Requested (%)": (
                100 * issued / requested if requested else np.nan
            ),
        })

    # Aggregate prefetcher counters across cache levels.
    issued = sum(r["Issued"] for r in rows)
    requested = sum(r["Requested"] for r in rows)
    useful = sum(r["Useful"] for r in rows)
    useless = sum(r["Useless"] for r in rows)

    rows.append({
        "Cache": "TOTAL",
        "Issued": issued,
        "Requested": requested,
        "Useful": useful,
        "Useless": useless,
        "Useful / Issued (%)": (
            100 * useful / issued if issued else np.nan
        ),
        "Useless / Issued (%)": (
            100 * useless / issued if issued else np.nan
        ),
        "Useful / Requested (%)": (
            100 * useful / requested if requested else np.nan
        ),
        "Issued / Requested (%)": (
            100 * issued / requested if requested else np.nan
        ),
    })

    return pd.DataFrame(rows)


def main():
    parser = argparse.ArgumentParser(
        description="Analyze ChampSim cache miss rates and prefetcher performance."
    )
    parser.add_argument(
        "json_path",
        help="Path to the ChampSim JSON results file"
    )
    args = parser.parse_args()

    with open(args.json_path, "r") as f:
        data = json.load(f)

    if isinstance(data, dict):
        data = [data]

    for index, entry in enumerate(data, start=1):
        sim = entry.get("sim", entry)

        print(f"\n{'=' * 80}")
        print(f"SIMULATION {index}")
        print(f"{'=' * 80}")

        print("\nCACHE MISS-RATE MATRIX")
        miss_matrix = cache_miss_matrix(sim)
        print(miss_matrix.to_string(index=False, float_format=lambda x: f"{x:.3f}"))

        print("\nPREFETCHER PERFORMANCE MATRIX")
        prefetch_matrix = prefetcher_matrix(sim)
        print(prefetch_matrix.to_string(index=False, float_format=lambda x: f"{x:.3f}"))

        # miss_file = f"miss_rates_{index}.csv"
        # prefetch_file = f"prefetch_performance_{index}.csv"

        # miss_matrix.to_csv(miss_file, index=False)
        # prefetch_matrix.to_csv(prefetch_file, index=False)

        # print(f"\nSaved {miss_file}")
        # print(f"Saved {prefetch_file}")


if __name__ == "__main__":
    main()
