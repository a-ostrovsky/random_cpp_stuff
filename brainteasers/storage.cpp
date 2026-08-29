#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <vector>

// TASK
// * Storage is divided into ordered levels, each with a fixed capacity.
// * An item is stored in the first level with enough available capacity.
// * Retrieval scans levels from highest to lowest and only considers levels
//   that are > 50% occupied
// * From the selected level, retrieval returns the heaviest non-expired item.
// * Lazy deletion of expired items on retrieval

namespace Storage {

using ItemId = std::uint32_t;

ItemId GetNextId() {
  static ItemId nextId = 0;
  return ++nextId;
}

struct Item {
  ItemId m_id{}; // assuming it is unique
  int m_weight{};
  int m_expirationTimestamp{};
};

struct ItemByWeightSorter {
  bool operator()(const Item &a, const Item &b) const {
    if (a.m_weight == b.m_weight) {
      return a.m_id < b.m_id;
    }
    return a.m_weight < b.m_weight;
  }
};

struct ItemByTimeSorter {
  bool operator()(const Item &a, const Item &b) const {
    return a.m_expirationTimestamp > b.m_expirationTimestamp;
  }
};

class Level {
  std::unordered_set<ItemId> m_activeItems{};
  std::priority_queue<Item, std::vector<Item>, ItemByWeightSorter>
      m_itemsByWeight{};
  std::priority_queue<Item, std::vector<Item>, ItemByTimeSorter>
      m_itemsByTime{};

  int m_initialCapacity{};
  int m_remainingCapacity{};

public:
  explicit Level(int capacity)
      : m_initialCapacity(capacity), m_remainingCapacity(capacity) {}

  bool TryInsert(Item item) {
    if (m_remainingCapacity < item.m_weight) {
      return false;
    }

    [[maybe_unused]] const auto isInserted =
        m_activeItems.insert(item.m_id).second;
    assert(isInserted);

    m_itemsByWeight.push(item);
    m_itemsByTime.push(item);
    m_remainingCapacity -= item.m_weight;
    return true;
  }

  std::optional<Item> Pop(int currentTime) {
    RemoveExpired(currentTime);

    // Occupancy less 50%
    if (m_remainingCapacity * 2 >= m_initialCapacity) {
      return {};
    }

    // Get the heaviest, non-removed element
    while (!m_itemsByWeight.empty()) {
      auto heaviest = m_itemsByWeight.top();
      m_itemsByWeight.pop();
      if (!m_activeItems.contains(heaviest.m_id)) {
        continue;
      }
      m_activeItems.erase(heaviest.m_id);
      m_remainingCapacity += heaviest.m_weight;
      return heaviest;
    }

    return {};
  }

private:
  void RemoveExpired(int currentTime) {
    while (!m_itemsByTime.empty() &&
           m_itemsByTime.top().m_expirationTimestamp <= currentTime) {
      const auto expired = m_itemsByTime.top();
      m_itemsByTime.pop();
      if (m_activeItems.erase(expired.m_id)) {
        m_remainingCapacity += expired.m_weight;
      }
    }
  }
};

class ConstrainedStorage {
  std::vector<Level> m_levels{};

public:
  explicit ConstrainedStorage(const std::initializer_list<int> &capacities)
      : m_levels(ToLevels(capacities)) {}

  bool Store(Item item) {
    assert(item.m_weight >= 0);
    for (auto &level : m_levels) {
      if (level.TryInsert(item)) {
        return true;
      }
    }
    return false;
  }

  std::optional<Item> Retrieve(int currentTime) {
    for (auto &data : m_levels | std::views::reverse) {
      if (auto item = data.Pop(currentTime)) {
        return item;
      }
    }

    return {};
  }

private:
  static std::vector<Level>
  ToLevels(const std::initializer_list<int> &capacities) {
    return capacities |
           std::views::transform([](int capacity) { return Level(capacity); }) |
           std::ranges::to<std::vector>();
  }
};

namespace Tests {
void Assert(bool pred) {
  if (!pred) {
    throw std::runtime_error("Assertion failed");
  }
}

void OnePerLevel() {
  ConstrainedStorage storage1({10, 20, 30});
  Assert(storage1.Store({GetNextId(), 10, 2}));
  Assert(storage1.Store({GetNextId(), 20, 2}));
  Assert(storage1.Store({GetNextId(), 30, 2}));
  Assert(!storage1.Store({GetNextId(), 30, 2}));

  Assert(storage1.Retrieve(1).value().m_weight == 30);
  Assert(storage1.Retrieve(1).value().m_weight == 20);
  Assert(!storage1.Retrieve(10).has_value());
}

void MultiplePerLevel() {
  ConstrainedStorage storage2({10, 0});
  Assert(storage2.Store({GetNextId(), 5, 2}));
  Assert(storage2.Store({GetNextId(), 5, 2}));
  Assert(!storage2.Store({GetNextId(), 5, 2}));
  Assert(storage2.Retrieve(1).value().m_weight == 5);
  Assert(!storage2.Retrieve(1).has_value()); // 50% full
}

void ExpiredItemsFreeCapacityWithoutRetrievingActiveItems() {
  ConstrainedStorage storage({10});

  Assert(storage.Store({GetNextId(), 6, 10}));
  Assert(storage.Store({GetNextId(), 4, 2}));
  Assert(storage.Retrieve(2).value().m_weight == 6);

  // 6 units were freed by expiration, 4 were retrieved
  Assert(storage.Store({GetNextId(), 6, 10}));
}

void RunAll() {
  OnePerLevel();
  MultiplePerLevel();
}
} // namespace Tests

} // namespace Storage

int main() {
  Storage::Tests::RunAll();
  return 0;
}
