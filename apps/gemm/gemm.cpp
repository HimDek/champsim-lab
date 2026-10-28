#include <cstdint>
#include <cstdio>
#include <vector>
#include <random>
extern "C" __attribute__((noinline)) void __champsim_roi_begin() { asm volatile("" ::: "memory"); }
extern "C" __attribute__((noinline)) void __champsim_roi_end() { asm volatile("" ::: "memory"); }
int main()
{
    const int N = 320;
    const int ITERS = 2;

    printf("[GEMM] C = A*B, N=%d, single precision, %d iteration(s)\n", N, ITERS);
    std::vector<float> A((size_t)N * N), B((size_t)N * N), C((size_t)N * N, 0.0f);
    std::mt19937 rng(6789);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto &x : A)
        x = dist(rng);
    for (auto &x : B)
        x = dist(rng);
    __champsim_roi_begin();
    for (int it = 0; it < ITERS; ++it)
    {
        for (int i = 0; i < N; ++i)
        {
            for (int k = 0; k < N; ++k)
            {
                const float a = A[(size_t)i * N + k];
                const float *brow = &B[(size_t)k * N];
                float *crow = &C[(size_t)i * N];
                for (int j = 0; j < N; ++j)
                {
                    crow[j] += a * brow[j];
                }
            }
        }
    }
    __champsim_roi_end();
    double checksum = 0.0;
    for (const float v : C)
        checksum += v;
    printf("[GEMM] done. checksum = %.6f\n", checksum);
    return 0;
}