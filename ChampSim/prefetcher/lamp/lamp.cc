#include "lamp.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

void lamp::prefetcher_initialize()
{
  history_ = {};
  mini_zones_ = {};
  mini_zone_cursor_ = 0;
  access_count_ = hits_window_ = misses_window_ = phase_window_accesses_ = 0;
  mini_zone_quota_ = 8;
  last_agent_revival_ = 0;
  phase_suspended_ = false;
  have_previous_access_ = false;
  previous_page_ = 0;
  previous_page_offset_ = 0;
  stream_detections_ = stride_detections_ = mini_zone_detections_ = 0;
  cross_page_predictions_ = issued_prefetches_ = 0;
}

void lamp::prefetcher_cycle_operate() {}

std::size_t lamp::entries_for_page(std::uint64_t page) const
{
  return static_cast<std::size_t>(std::count_if(history_.begin(), history_.end(),
      [page](const History& h) { return h.valid && h.page == page; }));
}

lamp::History* lamp::find_page_entry(std::uint64_t page, bool most_recent)
{
  History* result = nullptr;
  for (auto& h : history_) {
    if (!h.valid || h.page != page)
      continue;
    if (result == nullptr || (most_recent && h.last_used > result->last_used))
      result = &h;
  }
  return result;
}

lamp::History* lamp::find_matching_pattern(std::uint64_t page, std::uint8_t offset)
{
  History* result = nullptr;
  for (auto& h : history_) {
    if (!h.valid || h.page != page || h.delta == 0 || h.pattern == Pattern::mini_zone)
      continue;
    const int expected = static_cast<int>(h.last_offset) + static_cast<int>(h.delta);
    if (expected != static_cast<int>(offset))
      continue;
    if (result == nullptr || h.confidence > result->confidence ||
        (h.confidence == result->confidence && h.last_used > result->last_used))
      result = &h;
  }
  return result;
}

lamp::History* lamp::allocate_entry(std::uint64_t page, std::uint8_t page_last_offset,
                                    bool page_seen, std::uint8_t first_offset,
                                    std::uint8_t last_offset, std::int16_t delta,
                                    Pattern pattern)
{
  History* victim = nullptr;
  for (auto& h : history_) {
    if (!h.valid) {
      victim = &h;
      break;
    }
  }

  if (victim == nullptr) {
    // Preserve independent patterns on a page up to MAX_PATTERNS_PER_PAGE.
    // Prefer evicting the globally least-recently-used entry, except that an
    // already over-represented page is preferred when it owns the LRU entry.
    victim = &*std::min_element(history_.begin(), history_.end(),
        [this](const History& a, const History& b) {
          if (a.valid != b.valid)
            return !a.valid;
          return a.last_used < b.last_used;
        });
  }

  *victim = History{};
  victim->valid = true;
  victim->page = page;
  victim->last_used = access_count_;
  victim->page_last_offset = page_last_offset;
  victim->page_seen = page_seen;
  victim->first_offset = first_offset;
  victim->last_offset = last_offset;
  victim->delta = delta;
  victim->pattern = pattern;
  victim->depth = 0;
  return victim;
}

void lamp::update_page_last(std::uint64_t page, std::uint8_t offset)
{
  for (auto& h : history_) {
    if (h.valid && h.page == page) {
      h.page_last_offset = offset;
      h.page_seen = true;
      h.last_used = access_count_;
    }
  }
}

void lamp::issue_prefetch(std::uint64_t block_address, bool fill_this_level)
{
  if (block_address > (std::numeric_limits<std::uint64_t>::max() >> BLOCK_SHIFT))
    return;
  prefetch_line(champsim::address{block_address << BLOCK_SHIFT}, fill_this_level, 0);
  ++issued_prefetches_;
}

bool lamp::may_prefetch(const History& h) const
{
  if (phase_suspended_ && h.confidence < 6)
    return false;
  return !h.suspended && h.confidence > 0;
}

void lamp::prefetch_pattern(History& h, std::uint64_t current_block,
                            std::unordered_set<std::uint64_t>& issued)
{
  if (!may_prefetch(h) || h.delta == 0)
    return;

  const std::uint8_t depth = std::max<std::uint8_t>(1, h.depth);
  for (std::uint8_t i = 1; i <= depth; ++i) {
    const auto predicted_signed = static_cast<std::int64_t>(current_block) +
        static_cast<std::int64_t>(h.delta) * static_cast<std::int64_t>(i);
    if (predicted_signed < 0)
      break;
    const auto predicted = static_cast<std::uint64_t>(predicted_signed);
    // Intra-page patterns must not be extrapolated across a page boundary.
    if ((predicted >> (PAGE_SHIFT - BLOCK_SHIFT)) != h.page)
      break;
    if (issued.insert(predicted).second)
      issue_prefetch(predicted, true);
    h.last_prefetched = static_cast<std::uint8_t>(predicted % BLOCKS_PER_PAGE);
    h.has_last_prefetched = true;
  }
}

void lamp::remember_mini_zone(std::uint64_t page, std::uint8_t low, std::uint8_t high)
{
  auto& z = mini_zones_[mini_zone_cursor_++ % mini_zones_.size()];
  z = MiniZoneRecord{true, page, low, high, access_count_};
}

bool lamp::is_recent_mini_zone_hit(std::uint64_t page, std::uint8_t offset) const
{
  return std::any_of(mini_zones_.begin(), mini_zones_.end(),
      [=](const MiniZoneRecord& z) {
        return z.valid && z.page == page && offset >= z.low && offset <= z.high &&
               access_count_ - z.created_at <= PHASE_WINDOW * 8;
      });
}

void lamp::update_mini_zone_controller(bool reward)
{
  if (reward)
    mini_zone_quota_ = std::min<std::uint32_t>(64, mini_zone_quota_ + 1);

  if (access_count_ - last_agent_revival_ >= AGENT_REVIVAL_ACCESSES) {
    // Revival is intentionally gradual: it re-enables exploration without
    // resetting a quota that has already learned a useful policy.
    mini_zone_quota_ = std::max<std::uint32_t>(mini_zone_quota_, MINI_ZONE_QUOTA_THRESHOLD);
    last_agent_revival_ = access_count_;
  }
}

void lamp::prefetch_mini_zone(std::uint64_t page, std::uint8_t previous_offset,
                              std::uint8_t current_offset,
                              std::unordered_set<std::uint64_t>& issued)
{
  if (mini_zone_quota_ < MINI_ZONE_QUOTA_THRESHOLD)
    return;

  const int low = std::min<int>(previous_offset, current_offset);
  const int high = std::max<int>(previous_offset, current_offset);
  const int span = high - low;
  if (span <= 1 || span > static_cast<int>(MINI_ZONE_SIZE))
    return;

  // Reward/punishment controller from the paper: every zone trigger halves
  // the quota; demand hits to recently prefetched zones increase it.
  mini_zone_quota_ /= 2;
  // Only remember blocks actually requested as prefetches, not the two
  // demand endpoints that formed the candidate zone.
  if (high - low > 1)
    remember_mini_zone(page, static_cast<std::uint8_t>(low + 1),
                       static_cast<std::uint8_t>(high - 1));
  ++mini_zone_detections_;

  for (int offset = low; offset <= high; ++offset) {
    if (offset == previous_offset || offset == current_offset)
      continue;
    const auto block = page * BLOCKS_PER_PAGE + static_cast<std::uint64_t>(offset);
    if (issued.insert(block).second)
      issue_prefetch(block, true);
  }
}

void lamp::prefetch_correlated_pages(History& source,
                                     std::unordered_set<std::uint64_t>& issued)
{
  if (!source.has_next_page || source.transition_count < 2 || phase_suspended_)
    return;

  // Replay the learned page-transition chain. The first transition is the
  // observed target offset; subsequent links extrapolate the observed path.
  std::uint64_t page = source.next_page;
  std::uint8_t offset = source.next_page_offset;
  std::array<std::uint64_t, MAX_DEPTH> visited{};
  std::size_t visited_count = 0;

  for (std::size_t depth = 0; depth < MAX_DEPTH; ++depth) {
    if (std::find(visited.begin(), visited.begin() + static_cast<std::ptrdiff_t>(visited_count), page) !=
        visited.begin() + static_cast<std::ptrdiff_t>(visited_count))
      break;
    visited[visited_count++] = page;

    const auto block = page * BLOCKS_PER_PAGE + offset;
    if (issued.insert(block).second) {
      issue_prefetch(block, true);
      ++cross_page_predictions_;
    }

    History* next = find_page_entry(page);
    if (next == nullptr || !next->has_next_page || next->transition_count < 2)
      break;
    page = next->next_page;
    offset = next->next_page_offset;
  }
}

void lamp::update_phase_controller(bool cache_hit)
{
  if (cache_hit)
    ++hits_window_;
  else
    ++misses_window_;
  ++phase_window_accesses_;

  if (phase_window_accesses_ < PHASE_WINDOW)
    return;

  const bool hit_dominated = hits_window_ >= 8 * std::max<std::uint64_t>(1, misses_window_);
  if (hit_dominated)
    phase_suspended_ = true;
  else if (misses_window_ > 0)
    phase_suspended_ = false;

  hits_window_ = misses_window_ = phase_window_accesses_ = 0;
}

uint32_t lamp::prefetcher_cache_operate(champsim::address addr, champsim::address ip,
                                        uint8_t cache_hit, bool useful_prefetch,
                                        access_type type, uint32_t metadata_in)
{
  (void)ip;
  (void)type;
  ++access_count_;

  const std::uint64_t byte_addr = addr.to<std::uint64_t>();
  const std::uint64_t block = byte_addr >> BLOCK_SHIFT;
  const std::uint64_t page = byte_addr >> PAGE_SHIFT;
  const auto offset = static_cast<std::uint8_t>(block % BLOCKS_PER_PAGE);
  const bool hit = cache_hit != 0;

  // Mini-zone reward is based on a demand hit in a recently prefetched zone.
  update_mini_zone_controller(hit && is_recent_mini_zone_hit(page, offset));
  update_phase_controller(hit);

  std::unordered_set<std::uint64_t> issued;

  // First replay the path associated with the page being accessed. A page
  // transition is used only after the same edge has been observed twice.
  if (History* current_page_entry = find_page_entry(page); current_page_entry != nullptr)
    prefetch_correlated_pages(*current_page_entry, issued);

  // Learn the transition from the page exited by the preceding access.
  if (have_previous_access_ && previous_page_ != page) {
    History* exiting = find_page_entry(previous_page_);
    if (exiting != nullptr) {
      if (exiting->has_next_page && exiting->next_page == page &&
          exiting->next_page_offset == offset) {
        exiting->transition_count = std::min<std::uint8_t>(255, exiting->transition_count + 1);
      } else {
        exiting->transition_count = 1;
      }
      exiting->next_page = page;
      exiting->next_page_offset = offset;
      exiting->transition_delta = static_cast<std::int16_t>(static_cast<int>(offset) -
                                                             static_cast<int>(previous_page_offset_));
      exiting->has_next_page = true;
    }
  }

  History* matched = find_matching_pattern(page, offset);
  History* page_entry = find_page_entry(page);
  const bool page_was_seen = page_entry != nullptr && page_entry->page_seen;
  const std::uint8_t prior_offset = page_was_seen ? page_entry->page_last_offset : offset;
  const int delta_now = static_cast<int>(offset) - static_cast<int>(prior_offset);

  if (matched != nullptr) {
    // A demand access at the predicted offset validates the pattern.
    matched->last_used = access_count_;
    matched->last_offset = offset;
    matched->confidence = std::min<std::uint8_t>(7, matched->confidence + 1);

    if (matched->pattern == Pattern::candidate) {
      matched->pattern = std::abs(matched->delta) == 1 ? Pattern::stream : Pattern::stride;
      matched->confidence = std::max<std::uint8_t>(matched->confidence, 1);
      if (matched->pattern == Pattern::stream) {
        ++stream_detections_;
        matched->depth = 1;
      } else {
        ++stride_detections_;
        matched->depth = 2;
      }
    }

    if (matched->pattern == Pattern::disturbed_stride && matched->depth == 0)
      matched->depth = 1;

    // A correct prefetch increases lookahead. If a path reaches the maximum,
    // halve depth and suspend until demand approaches the predicted tail.
    if (useful_prefetch)
      matched->depth = std::min<std::uint8_t>(MAX_DEPTH,
          static_cast<std::uint8_t>(std::max<int>(1, matched->depth) + 1));

    if (matched->suspended && matched->has_last_prefetched &&
        std::abs(static_cast<int>(offset) - static_cast<int>(matched->last_prefetched)) <= 3) {
      matched->suspended = false;
      matched->depth = std::max<std::uint8_t>(3, matched->depth);
    }

    if (matched->depth >= MAX_DEPTH) {
      matched->depth = static_cast<std::uint8_t>(MAX_DEPTH / 2);
      matched->suspended = true;
    }

    // Prefetch only confirmed stream/stride paths. A candidate needs two
    // equal deltas before its first multi-line prediction.
    if (matched->pattern == Pattern::stream || matched->pattern == Pattern::stride ||
        matched->pattern == Pattern::disturbed_stride)
      prefetch_pattern(*matched, block, issued);
  } else if (page_was_seen && delta_now != 0) {
    const auto abs_delta = std::abs(delta_now);
    const bool has_confirmed_pattern = std::any_of(history_.begin(), history_.end(),
        [page](const History& h) {
          return h.valid && h.page == page &&
                 (h.pattern == Pattern::stream || h.pattern == Pattern::stride ||
                  h.pattern == Pattern::disturbed_stride);
        });

    // Each distinct local delta can own an independent entry, allowing mixed
    // strides in one page. Cap per-page entries to keep storage bounded.
    if (entries_for_page(page) >= MAX_PATTERNS_PER_PAGE) {
      History* least_confident = nullptr;
      for (auto& h : history_) {
        if (!h.valid || h.page != page)
          continue;
        if (least_confident == nullptr || h.confidence < least_confident->confidence ||
            (h.confidence == least_confident->confidence && h.last_used < least_confident->last_used))
          least_confident = &h;
      }
      if (least_confident != nullptr && least_confident->confidence == 0)
        *least_confident = History{};
    }

    if (entries_for_page(page) < MAX_PATTERNS_PER_PAGE) {
      const Pattern initial = (abs_delta == 1) ? Pattern::stream :
          (has_confirmed_pattern ? Pattern::disturbed_stride : Pattern::candidate);
      History* candidate = allocate_entry(page, offset, true,
          static_cast<std::uint8_t>(prior_offset), offset,
          static_cast<std::int16_t>(delta_now), initial);
      candidate->confidence = (initial == Pattern::stream) ? 1 : 0;
      candidate->depth = (initial == Pattern::stream) ? 1 : 0;

      if (initial == Pattern::stream) {
        ++stream_detections_;
        prefetch_pattern(*candidate, block, issued);
      } else if (!has_confirmed_pattern && abs_delta < static_cast<int>(MINI_ZONE_SIZE)) {
        // Mini-zone is considered only when no stream/stride is already known.
        prefetch_mini_zone(page, static_cast<std::uint8_t>(prior_offset), offset, issued);
      }
    }
  } else if (page_entry == nullptr) {
    // First observation of a page: allocate a history entry without a pattern.
    allocate_entry(page, offset, true, offset, offset, 0, Pattern::candidate);
  }

  // Keep the most recent demand offset coherent across every pattern entry
  // belonging to this page, without overwriting each pattern's own last_offset.
  update_page_last(page, offset);

  // A stable high-confidence pattern can resume even in a hit-dominated phase.
  if (phase_suspended_) {
    const bool strong = std::any_of(history_.begin(), history_.end(),
        [page](const History& h) { return h.valid && h.page == page && h.confidence >= 6; });
    if (strong && matched != nullptr)
      prefetch_pattern(*matched, block, issued);
  }

  previous_page_ = page;
  previous_page_offset_ = offset;
  have_previous_access_ = true;
  return metadata_in;
}

uint32_t lamp::prefetcher_cache_fill(champsim::address addr, long set, long way,
                                     uint8_t prefetch, champsim::address evicted_addr,
                                     uint32_t metadata_in)
{
  (void)addr;
  (void)set;
  (void)way;
  (void)prefetch;
  (void)evicted_addr;
  return metadata_in;
}

void lamp::prefetcher_final_stats()
{
  std::cout << "[LAMP] accesses=" << access_count_
            << " issued_prefetches=" << issued_prefetches_
            << " stream_detections=" << stream_detections_
            << " stride_detections=" << stride_detections_
            << " mini_zone_detections=" << mini_zone_detections_
            << " cross_page_predictions=" << cross_page_predictions_
            << " mini_zone_quota=" << mini_zone_quota_
            << " phase_suspended=" << (phase_suspended_ ? 1 : 0) << '\n';
}
