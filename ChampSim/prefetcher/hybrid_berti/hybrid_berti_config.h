#ifndef HYBRID_BERTI_CONFIG_H
#define HYBRID_BERTI_CONFIG_H

// Base predictors
#define ENABLE_SBERTI_BERTI       1
#define ENABLE_AMPM               0
#define ENABLE_SMART_STRIDE       0

// Optional predictors / optimizations
#define ENABLE_GBERTI_GLOBAL      0
#define ENABLE_BERTIGO_PC_PATH    0
#define ENABLE_BLOOM_FILTER       0

// Shared issue path
#define HYBRID_RECENT_DEDUP       1

#endif // HYBRID_BERTI_CONFIG_H
