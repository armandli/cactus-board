#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cb {

enum class Status { Ready, Progressing, Complete };

inline constexpr std::array<Status, 3> kAllStatuses{Status::Ready, Status::Progressing,
                                                   Status::Complete};

// Wire name used in tool calls: "ready", "progressing", "complete".
std::string_view to_string(Status s);
// Display name for column headers: "Ready", "Progressing", "Complete".
std::string_view display_name(Status s);
// Accepts the wire names plus legacy spellings ("todo", "in progress", "done").
std::optional<Status> parse_status(std::string_view s);

using Date = std::chrono::year_month_day;

// Parses a strict "YYYY-MM-DD" date, rejecting impossible dates like 2026-13-45.
std::optional<Date> parse_date(std::string_view s);
std::string format_date(Date d);

struct Item {
  int id = 0;
  std::string title; // single word, e.g. "auth"
  std::string owner;
  std::string description; // detail text, deliberately not shown on the board
  std::string category;    // groups items by project
  int priority = 3;        // lower value means higher priority
  std::optional<Date> deadline;
  Status status = Status::Ready;
};

class Board {
public:
  Board() = default;
  // Adopts items with their existing ids; subsequent add() continues past the highest one.
  explicit Board(std::vector<Item> items);

  int add(std::string title, Status status = Status::Ready);
  // Ignores any caller-set id and assigns a fresh one.
  int add(Item item);
  bool move(int id, Status status);
  bool remove(int id);

  const Item* find(int id) const;
  // Case-insensitive exact title match, falling back to a unique substring match.
  const Item* find_by_title(std::string_view title) const;

  std::vector<const Item*> column(Status status) const;
  const std::vector<Item>& items() const { return items_; }

private:
  Item* find_mut(int id);

  std::vector<Item> items_;
  int next_id_ = 1;
};

} // namespace cb
