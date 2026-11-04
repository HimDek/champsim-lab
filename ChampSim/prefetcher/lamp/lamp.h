#ifndef LAMP_H
#define LAMP_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include "cache.h"
#include "modules.h"

// LAMP: Learning-based Adaptive Multi-objective Prefetcher.
// ChampSim class-based prefetcher API. Intended for L2C only.
struct lamp : public champsim::modules::prefetcher {
  using prefetcher::prefetcher;

  static constexpr std::size_t HISTORY_ENTRIES = 256;
  static constexpr std::size_t MAX_DEPTH = 7;
  static constexpr std::size_t MAX_PATTERNS_PER_PAGE = 4;
  static constexpr std::size_t MINI_ZONE_SIZE = 8; // cache blocks
  static constexpr std::size_t MINI_ZONE_HISTORY = 32;
  static constexpr std::uint32_t MINI_ZONE_QUOTA_THRESHOLD = 4;
  static constexpr std::uint64_t AGENT_REVIVAL_ACCESSES = 4096;
  static constexpr std::uint64_t PHASE_WINDOW = 1024;
  static constexpr std::uint64_t PAGE_SHIFT = 12;
  static constexpr std::uint64_t BLOCK_SHIFT = 6;
  static constexpr std::uint64_t BLOCKS_PER_PAGE = 1ULL << (PAGE_SHIFT - BLOCK_SHIFT);

  void prefetcher_initialize();
  void prefetcher_cycle_operate();
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip,
                                    uint8_t cache_hit, bool useful_prefetch,
                                    access_type type, uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way,
                                 uint8_t prefetch, champsim::address evicted_addr,
                                 uint32_t metadata_in);
  void prefetcher_final_stats();

private:
  enum class Pattern : std::uint8_t {
    candidate,
    stream,
    stride,
    disturbed_stride,
    mini_zone,
    correlated_page
  };

  struct History {
    bool valid = false;
    std::uint64_t page = 0;
    std::uint64_t last_used = 0;

    // Most recent demand offset on this page (replicated across entries for
    // the page so multiple independent patterns can coexist).
    std::uint8_t page_last_offset = 0;
    bool page_seen = false;

    // Pattern-local state. last_offset is advanced only when this pattern
    // matches; delta is the candidate/predicted within-page stride.
    std::uint8_t first_offset = 0;
    std::uint8_t last_offset = 0;
    std::int16_t delta = 0;
    std::uint8_t confidence = 0;
    std::uint8_t depth = 0;
    std::uint8_t last_prefetched = 0;
    bool has_last_prefetched = false;
    bool suspended = false;
    Pattern pattern = Pattern::candidate;

    // Last observed page transition, for cross-page path replay.
    std::uint64_t next_page = 0;
    std::uint8_t next_page_offset = 0;
    bool has_next_page = false;
    std::uint8_t transition_count = 0;
    std::int16_t transition_delta = 0;
  };

  struct MiniZoneRecord {
    bool valid = false;
    std::uint64_t page = 0;
    std::uint8_t low = 0;
    std::uint8_t high = 0;
    std::uint64_t created_at = 0;
  };

  std::array<History, HISTORY_ENTRIES> history_{};
  std::array<MiniZoneRecord, MINI_ZONE_HISTORY> mini_zones_{};
  std::size_t mini_zone_cursor_ = 0;

  std::uint64_t access_count_ = 0;
  std::uint64_t hits_window_ = 0;
  std::uint64_t misses_window_ = 0;
  std::uint64_t phase_window_accesses_ = 0;
  std::uint32_t mini_zone_quota_ = 8;
  std::uint64_t last_agent_revival_ = 0;
  bool phase_suspended_ = false;

  bool have_previous_access_ = false;
  std::uint64_t previous_page_ = 0;
  std::uint8_t previous_page_offset_ = 0;

  std::uint64_t stream_detections_ = 0;
  std::uint64_t stride_detections_ = 0;
  std::uint64_t mini_zone_detections_ = 0;
  std::uint64_t cross_page_predictions_ = 0;
  std::uint64_t issued_prefetches_ = 0;

  History* find_page_entry(std::uint64_t page, bool most_recent = true);
  History* find_matching_pattern(std::uint64_t page, std::uint8_t offset);
  History* allocate_entry(std::uint64_t page, std::uint8_t page_last_offset,
                          bool page_seen, std::uint8_t first_offset,
                          std::uint8_t last_offset, std::int16_t delta,
                          Pattern pattern);
  std::size_t entries_for_page(std::uint64_t page) const;
  void update_page_last(std::uint64_t page, std::uint8_t offset);

  void issue_prefetch(std::uint64_t block_address, bool fill_this_level = true);
  void prefetch_pattern(History& h, std::uint64_t current_block,
                        std::unordered_set<std::uint64_t>& issued);
  void prefetch_mini_zone(std::uint64_t page, std::uint8_t previous_offset,
                          std::uint8_t current_offset,
                          std::unordered_set<std::uint64_t>& issued);
  void prefetch_correlated_pages(History& source,
                                 std::unordered_set<std::uint64_t>& issued);
  void remember_mini_zone(std::uint64_t page, std::uint8_t low, std::uint8_t high);
  bool is_recent_mini_zone_hit(std::uint64_t page, std::uint8_t offset) const;
  void update_mini_zone_controller(bool reward);
  void update_phase_controller(bool cache_hit);
  bool may_prefetch(const History& h) const;
};

#endif // LAMP_H
