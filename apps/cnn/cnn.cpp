#include <cstdint>
#include <cstdio>
#include <vector>
#include <random>
#include <algorithm>
extern "C" __attribute__((noinline)) void __champsim_roi_begin() { asm volatile("" ::: "memory"); }
extern "C" __attribute__((noinline)) void __champsim_roi_end() { asm volatile("" ::: "memory"); }
int main()
{
    const int C_in = 3;
    const int C_out = 32;
    const int H = 112;
    const int W = 112;
    const int K = 3;
    const int PAD = 1;
    const int ITERS = 3;
    printf("[CNN] conv forward: in %dx%dx%d, %d filters %dx%d, %d iteration(s)\n",
           C_in, H, W, C_out, K, K, ITERS);
    std::vector<float> input((size_t)C_in * H * W);
    std::vector<float> weight((size_t)C_out * C_in * K * K);
    std::vector<float> output((size_t)C_out * H * W, 0.0f);
    std::mt19937 rng(2468);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    for (auto &x : input)
        x = d(rng);
    for (auto &x : weight)
        x = d(rng);
    auto in_at = [&](int c, int y, int x) -> float
    {
        if (y < 0 || y >= H || x < 0 || x >= W)
            return 0.0f;
        return input[((size_t)c * H + y) * W + x];
    };

    __champsim_roi_begin();
    for (int it = 0; it < ITERS; ++it)
    {
        for (int oc = 0; oc < C_out; ++oc)
        {
            for (int oy = 0; oy < H; ++oy)
            {
                for (int ox = 0; ox < W; ++ox)
                {
                    float acc = 0.0f;
                    for (int ic = 0; ic < C_in; ++ic)
                    {
                        for (int ky = 0; ky < K; ++ky)
                        {
                            for (int kx = 0; kx < K; ++kx)
                            {
                                float w = weight[(((size_t)oc * C_in + ic) * K + ky) * K + kx];
                                acc += w * in_at(ic, oy + ky - PAD, ox + kx - PAD);
                            }
                        }
                    }
                    acc = acc > 0.0f ? acc : 0.0f;
                    output[((size_t)oc * H + oy) * W + ox] = acc;
                }
            }
        }
    }
    __champsim_roi_end();
    double checksum = 0.0;
    for (const float v : output)
        checksum += v;
    printf("[CNN] done. checksum = %.6f\n", checksum);
    return 0;
}