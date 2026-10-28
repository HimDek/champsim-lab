import lzma
import struct
from collections import Counter, defaultdict

TRACE = "traces/llama-gemv.champsimtrace.xz"
RECORD_SIZE = 64
LINE_SIZE = 64

strides = Counter()
temporal_deltas = Counter()

last_line = None
last_seen_record = {}

accesses = 0
records = 0

with lzma.open(TRACE, "rb") as f:
    while True:
        record = f.read(RECORD_SIZE)

        if not record:
            break

        if len(record) != RECORD_SIZE:
            print(f"Incomplete record: {len(record)} bytes")
            break

        records += 1

        dst_mem, = struct.unpack_from("<Q", record, 32)
        src_mem, = struct.unpack_from("<Q", record, 40)

        for addr in (src_mem, dst_mem):
            if addr == 0:
                continue

            line = addr // LINE_SIZE

            # Spatial stride between consecutive accesses
            if last_line is not None:
                strides[line - last_line] += 1

            last_line = line

            # Temporal delta between accesses to the same cache line
            if line in last_seen_record:
                delta = records - last_seen_record[line]
                temporal_deltas[delta] += 1

            last_seen_record[line] = records
            accesses += 1

print(f"Records: {records:,}")
print(f"Memory accesses: {accesses:,}")

print("\nMost common spatial strides:")
for stride, count in strides.most_common(20):
    print(f"{stride:+8} lines ({stride * LINE_SIZE:+10} bytes): {count:,}")

print("\nMost common temporal deltas:")
for delta, count in temporal_deltas.most_common(20):
    print(f"{delta:8} records: {count:,}")
