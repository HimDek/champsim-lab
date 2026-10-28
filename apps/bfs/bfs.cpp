#include <cstdint>
#include <cstdio>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
extern "C" __attribute__((noinline)) void __champsim_roi_begin() { asm volatile("" ::: "memory"); }
extern "C" __attribute__((noinline)) void __champsim_roi_end() { asm volatile("" ::: "memory"); }
int main()
{
    const uint32_t N = 200000;
    const uint32_t AVG_DEGREE = 12;
    const uint32_t NUM_EDGES = N * AVG_DEGREE / 2;
    const int NUM_BFS = 16;
    printf("[BFS] building graph: %u vertices, ~%u undirected edges\n", N,
           NUM_EDGES);
    std::mt19937 rng(12345);
    std::uniform_int_distribution<uint32_t> pick(0, N - 1);
    std::vector<uint32_t> degree(N, 0);
    std::vector<std::pair<uint32_t, uint32_t>> edges;
    edges.reserve(NUM_EDGES);
    for (uint32_t e = 0; e < NUM_EDGES; ++e)
    {
        uint32_t a = pick(rng), b = pick(rng);
        if (a == b)
            continue;
        edges.emplace_back(a, b);
        degree[a]++;

        degree[b]++;
    }
    std::vector<uint32_t> row_ptr(N + 1, 0);
    for (uint32_t v = 0; v < N; ++v)
        row_ptr[v + 1] = row_ptr[v] + degree[v];
    const uint32_t total = row_ptr[N];
    std::vector<uint32_t> col_idx(total);
    std::vector<uint32_t> cursor(row_ptr.begin(), row_ptr.end() - 1);
    for (auto &ed : edges)
    {
        col_idx[cursor[ed.first]++] = ed.second;
        col_idx[cursor[ed.second]++] = ed.first;
    }
    std::vector<int32_t> dist(N);
    uint64_t checksum = 0;
    __champsim_roi_begin();
    for (int run = 0; run < NUM_BFS; ++run)
    {
        std::fill(dist.begin(), dist.end(), -1);
        uint32_t src = (uint32_t)((uint64_t)run * 6364136223846793005ULL % N);
        dist[src] = 0;
        std::queue<uint32_t> frontier;
        frontier.push(src);
        while (!frontier.empty())
        {
            uint32_t u = frontier.front();
            frontier.pop();
            int32_t nd = dist[u] + 1;
            for (uint32_t k = row_ptr[u]; k < row_ptr[u + 1]; ++k)
            {
                uint32_t w = col_idx[k];
                if (dist[w] < 0)
                {
                    dist[w] = nd;
                    frontier.push(w);
                }
            }
        }
        for (uint32_t v = 0; v < N; ++v)
            if (dist[v] > 0)
                checksum += (uint64_t)dist[v];
    }
    __champsim_roi_end();
    printf("[BFS] done. checksum = %llu\n", (unsigned long long)checksum);
    return 0;
}