#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cb {

enum class Status { Todo, InProgress, Done };

inline constexpr std::array<Status, 3> kAllStatuses{Status::Todo, Status::InProgress, Status::Done};

// Wire name used in tool calls: "todo", "in_progress", "done".
std::string_view to_string(Status s);
// Display name for column headers: "To Do", "In Progress", "Done".
std::string_view display_name(Status s);
std::optional<Status> parse_status(std::string_view s);

struct Item {
  int id;
  std::string title;
  Status status;
};

class Board {
public:
  int add(std::string title, Status status = Status::Todo);
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
