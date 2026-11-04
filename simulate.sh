ChampSim/bin/champsim --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv.txt
ChampSim/bin/champsim-fa --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-fa.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-fa.txt

ChampSim/bin/champsim-a --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-a.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-a.txt
ChampSim/bin/champsim-b --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-b.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-b.txt

ChampSim/bin/champsim-d --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-d.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-d.txt
ChampSim/bin/champsim-e --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-e.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-e.txt
ChampSim/bin/champsim-va-ampm-lite --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-va-ampm-lite.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-va-ampm-lite.txt
ChampSim/bin/champsim-spp-dev --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-spp-dev.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-spp-dev.txt

ChampSim/bin/champsim-ip-stride --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-ip-stride.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-ip-stride.txt
ChampSim/bin/champsim-next-line --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-next-line.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-next-line.txt
ChampSim/bin/champsim-ip-stride-b --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-ip-stride-b.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-ip-stride-b.txt
ChampSim/bin/champsim-next-line-b --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-next-line-b.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-next-line-b.txt

ChampSim/bin/champsim-c --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-c.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-c.txt
ChampSim/bin/champsim-f --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-f.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-f.txt
ChampSim/bin/champsim-g --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-g.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-g.txt
ChampSim/bin/champsim-h --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-h.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-h.txt
ChampSim/bin/champsim-i --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-i.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-i.txt
ChampSim/bin/champsim-j --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-j.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-j.txt
ChampSim/bin/champsim-k --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-k.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-k.txt
ChampSim/bin/champsim-l --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-l.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-l.txt
ChampSim/bin/champsim-m --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-m.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-m.txt
ChampSim/bin/champsim-n --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-n.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-n.txt

ChampSim/bin/champsim-lamp-a --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-lamp-a.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-lamp-a.txt
ChampSim/bin/champsim-lamp-b --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-lamp-b.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-lamp-b.txt
ChampSim/bin/champsim-lamp-c --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-lamp-c.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-lamp-c.txt
ChampSim/bin/champsim-lamp-d --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-lamp-d.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-lamp-d.txt

ChampSim/bin/champsim-berti --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-berti.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-berti.txt
ChampSim/bin/champsim-gberti --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-gberti.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-gberti.txt
ChampSim/bin/champsim-berti-go --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-berti-go.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-berti-go.txt
ChampSim/bin/champsim-sberti --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-sberti.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-sberti.txt
ChampSim/bin/champsim-ampm-berti --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-ampm-berti.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-ampm-berti.txt
ChampSim/bin/champsim-sberti-tuned --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-sberti-tuned.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-sberti-tuned.txt

ChampSim/bin/champsim-hybrid-berti --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-hybrid-berti.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-hybrid-berti.txt

ChampSim/bin/champsim-sppam --warmup-instructions 40000000 --simulation-instructions 200000000 --json results/llama-gemv-sppam.json traces/llama-gemv.champsimtrace.xz | tee results/llama-gemv-sppam.txt