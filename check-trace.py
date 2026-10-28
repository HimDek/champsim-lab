import lzma
import struct
from collections import Counter

TRACE = "traces/llama-gemv.champsimtrace.xz"
RECORD_SIZE = 64
LINE_SIZE = 64

strides = Counter()
prev_line = None
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

        # Standard ChampSim record layout
        dst_mem, = struct.unpack_from("<Q", record, 32)
        src_mem, = struct.unpack_from("<Q", record, 40)

        # Count nonzero memory addresses
        for addr in (src_mem, dst_mem):
            if addr == 0:
                continue

            line = addr // LINE_SIZE

            if prev_line is not None:
                strides[line - prev_line] += 1

            prev_line = line
            accesses += 1

print(f"Records: {records:,}")
print(f"Memory accesses: {accesses:,}")
print("\nMost common strides:")

for stride, count in strides.most_common(20):
    print(f"{stride:+8} lines ({stride * LINE_SIZE:+10} bytes): {count:,}")