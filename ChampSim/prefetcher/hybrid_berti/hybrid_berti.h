//=======================================================================================//
// File             : hybrid_berti.h
// Description      : sBerti (AMPM + Berti) Prefetcher
//=======================================================================================//

#ifndef __HYBRID_BERTI_H__
#define __HYBRID_BERTI_H__

#include "champsim.h"
#include "modules.h"
#include "hybrid_berti_config.h"
#include <array>
#include <cstdint>
#include <vector>
#include <cmath>

// =====================================================================
// BERTI DEFINITIONS
// =====================================================================
#define L1D_PAGE_BLOCKS_BITS (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)
#define L1D_PAGE_BLOCKS (1 << L1D_PAGE_BLOCKS_BITS)
#define L1D_PAGE_OFFSET_MASK (L1D_PAGE_BLOCKS - 1)
#define L1D_MAX_NUM_BURST_PREFETCHES 3
#define L1D_BERTI_CTR_MED_HIGH_CONFIDENCE 2

// TIME AND OVERFLOWS
#define L1D_TIME_BITS 16
#define L1D_TIME_OVERFLOW ((uint64_t)1 << L1D_TIME_BITS)
#define L1D_TIME_MASK (L1D_TIME_OVERFLOW - 1)

// CURRENT PAGES TABLE
#define L1D_CURRENT_PAGES_TABLE_INDEX_BITS 6
#define L1D_CURRENT_PAGES_TABLE_ENTRIES ((1 << L1D_CURRENT_PAGES_TABLE_INDEX_BITS) - 1)
#define L1D_CURRENT_PAGES_TABLE_NUM_BERTI 10
#define L1D_CURRENT_PAGES_TABLE_NUM_BERTI_PER_ACCESS 7

// PREVIOUS REQUESTS TABLE
#define L1D_PREV_REQUESTS_TABLE_INDEX_BITS 10
#define L1D_PREV_REQUESTS_TABLE_ENTRIES (1 << L1D_PREV_REQUESTS_TABLE_INDEX_BITS)
#define L1D_PREV_REQUESTS_TABLE_MASK (L1D_PREV_REQUESTS_TABLE_ENTRIES - 1)
#define L1D_PREV_REQUESTS_TABLE_NULL_POINTER L1D_CURRENT_PAGES_TABLE_ENTRIES

// PREVIOUS PREFETCHES TABLE
#define L1D_PREV_PREFETCHES_TABLE_INDEX_BITS 9
#define L1D_PREV_PREFETCHES_TABLE_ENTRIES (1 << L1D_PREV_PREFETCHES_TABLE_INDEX_BITS)
#define L1D_PREV_PREFETCHES_TABLE_MASK (L1D_PREV_PREFETCHES_TABLE_ENTRIES - 1)
#define L1D_PREV_PREFETCHES_TABLE_NULL_POINTER L1D_CURRENT_PAGES_TABLE_ENTRIES

// RECORD PAGES TABLE
#define L1D_RECORD_PAGES_TABLE_ENTRIES (((1 << 10) + (1 << 7)) - 1)
#define L1D_TRUNCATED_PAGE_ADDR_BITS 32
#define L1D_TRUNCATED_PAGE_ADDR_MASK (((uint64_t)1 << L1D_TRUNCATED_PAGE_ADDR_BITS) - 1)

// IP TABLE
#define L1D_IP_TABLE_INDEX_BITS 10
#define L1D_IP_TABLE_ENTRIES (1 << L1D_IP_TABLE_INDEX_BITS)
#define L1D_IP_TABLE_INDEX_MASK (L1D_IP_TABLE_ENTRIES - 1)
#define L1D_IP_TABLE_NULL_POINTER L1D_RECORD_PAGES_TABLE_ENTRIES

// =====================================================================
// BERTI STRUCTURES
// =====================================================================
typedef struct __l1d_current_page_entry {
  uint64_t page_addr;
  uint64_t ip;
  uint64_t u_vector;
  uint64_t first_offset;
  int berti[L1D_CURRENT_PAGES_TABLE_NUM_BERTI];
  unsigned berti_ctr[L1D_CURRENT_PAGES_TABLE_NUM_BERTI];
  uint64_t last_burst;
  uint64_t lru;
} l1d_current_page_entry;

typedef struct __l1d_prev_request_entry {
  uint64_t page_addr_pointer;
  uint64_t offset;
  uint64_t time;
} l1d_prev_request_entry;

typedef struct __l1d_prev_prefetch_entry {
  uint64_t page_addr_pointer;
  uint64_t offset;
  uint64_t time_lat;
  bool completed;
} l1d_prev_prefetch_entry;

typedef struct __l1d_record_page_entry {
  uint64_t page_addr;
  uint64_t u_vector;
  uint64_t first_offset;
  int berti;
  uint64_t lru;
} l1d_record_page_entry;

// =====================================================================
// AMPM STRUCTURES
// =====================================================================
struct AMPMEntry {
    uint64_t page_tag = 0;
    uint64_t access_map = 0;  // One bit per cache line in a page
    uint64_t lru_cycle = 0;
    bool valid = false;
};

// =====================================================================
// SBERTI CLASS
// =====================================================================
class hybrid_berti : public champsim::modules::prefetcher {
private:
  // --- BERTI DATA ---
  l1d_current_page_entry l1d_current_pages_table[L1D_CURRENT_PAGES_TABLE_ENTRIES];
  l1d_prev_request_entry l1d_prev_requests_table[L1D_PREV_REQUESTS_TABLE_ENTRIES];
  uint64_t l1d_prev_requests_table_head;
  l1d_prev_prefetch_entry l1d_prev_prefetches_table[L1D_PREV_PREFETCHES_TABLE_ENTRIES];
  uint64_t l1d_prev_prefetches_table_head;
  l1d_record_page_entry l1d_record_pages_table[L1D_RECORD_PAGES_TABLE_ENTRIES];
  uint64_t l1d_ip_table[L1D_IP_TABLE_ENTRIES];

  // --- AMPM DATA ---
  static const int AMPM_SETS = 32;
  static const int AMPM_WAYS = 4;
  static const int AMPM_MAX_STRIDE = 4;
  int AMPM_BLOCKS = L1D_PAGE_BLOCKS;
  AMPMEntry ampm_table[AMPM_SETS][AMPM_WAYS];

  // --- SHARED DEDUP STRUCTURE ---
  static const int RECENT_WINDOW_SIZE = 256; 
  uint64_t recent_prefetches[RECENT_WINDOW_SIZE];
  int recent_head = 0;

  // --- BERTI FUNCTIONS ---
  uint64_t l1d_get_latency(uint64_t cycle, uint64_t cycle_prev);
  int l1d_calculate_stride(uint64_t prev_offset, uint64_t current_offset);
  void l1d_init_current_pages_table();
  uint64_t l1d_get_current_pages_entry(uint64_t page_addr);
  void l1d_update_lru_current_pages_table(uint64_t index);
  uint64_t l1d_get_lru_current_pages_entry();
  int l1d_get_berti_current_pages_table(uint64_t index, uint64_t& ctr);
  void l1d_add_current_pages_table(uint64_t index, uint64_t page_addr, uint64_t ip, uint64_t offset);
  uint64_t l1d_update_demand_current_pages_table(uint64_t index, uint64_t offset);
  void l1d_add_berti_current_pages_table(uint64_t index, int berti);
  bool l1d_requested_offset_current_pages_table(uint64_t index, uint64_t offset);
  void l1d_remove_current_table_entry(uint64_t index);
  void l1d_init_prev_requests_table();
  uint64_t l1d_find_prev_request_entry(uint64_t pointer, uint64_t offset);
  void l1d_add_prev_requests_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  void l1d_reset_pointer_prev_requests(uint64_t pointer);
  uint64_t l1d_get_latency_prev_requests_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  void l1d_get_berti_prev_requests_table(uint64_t pointer, uint64_t offset, uint64_t cycle, int* berti);
  void l1d_init_prev_prefetches_table();
  uint64_t l1d_find_prev_prefetch_entry(uint64_t pointer, uint64_t offset);
  void l1d_add_prev_prefetches_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  void l1d_reset_pointer_prev_prefetches(uint64_t pointer);
  void l1d_reset_entry_prev_prefetches_table(uint64_t pointer, uint64_t offset);
  uint64_t l1d_get_and_set_latency_prev_prefetches_table(uint64_t pointer, uint64_t offset, uint64_t cycle);
  uint64_t l1d_get_latency_prev_prefetches_table(uint64_t pointer, uint64_t offset);
  void l1d_init_record_pages_table();
  uint64_t l1d_get_lru_record_pages_entry();
  void l1d_update_lru_record_pages_table(uint64_t index);
  void l1d_add_record_pages_table(uint64_t index, uint64_t page_addr, uint64_t vector, uint64_t first_offset, int berti);
  uint64_t l1d_get_entry_record_pages_table(uint64_t page_addr, uint64_t first_offset);
  uint64_t l1d_get_entry_record_pages_table(uint64_t page_addr);
  void l1d_copy_entries_record_pages_table(uint64_t index_from, uint64_t index_to);
  void l1d_init_ip_table();
  void l1d_record_current_page(uint64_t index_current);
  // --- AMPM FUNCTIONS ---
  void ampm_initialize();
  void ampm_operate(uint64_t addr, uint64_t page_addr, uint64_t offset, uint64_t ip,
                    uint64_t current_cycle, uint32_t metadata_in);

  // --- SHARED HELPER FUNCTIONS ---
  void addRecentPrefetch(uint64_t addr);
  bool hasRecentlyPrefetched(uint64_t addr);
  // Wrapper to issue prefetch, update Bloom filter, and check dedup
  bool issue_prefetch(champsim::address addr, uint64_t ip, uint32_t metadata_in);

  // Optional global-delta predictor. Entries are indexed by PC or PC-path signature.
  struct GlobalDeltaEntry {
    uint64_t key = 0;
    int64_t delta = 0;       // in cache lines
    uint8_t confidence = 0;
    bool valid = false;
  };
  static const unsigned GLOBAL_DELTA_ENTRIES = 256;
  GlobalDeltaEntry global_delta_table[GLOBAL_DELTA_ENTRIES]{};
  uint64_t global_last_line = 0;
  uint64_t global_last_pc = 0;
  bool global_have_last = false;
  std::array<uint64_t, 4> pc_path{};
  uint64_t pc_path_signature = 0;
  uint64_t bloom_bits[256]{}; // 16,384-bit optional issue filter

  uint64_t make_pc_path_signature(uint64_t ip);
  void global_delta_operate(uint64_t line_addr, uint64_t ip, uint32_t metadata_in);
  bool bloom_seen_or_mark(uint64_t line_addr);

public:
  using champsim::modules::prefetcher::prefetcher;
  void prefetcher_initialize();
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
  void prefetcher_cycle_operate();
};

#endif /* __HYBRID_BERTI_H__ */