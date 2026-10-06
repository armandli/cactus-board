#include "board/column.h"

#include <algorithm>

namespace cb {

bool item_precedes(const Item& a, const Item& b) {
  if (a.priority != b.priority) return a.priority < b.priority;
  if (a.deadline && b.deadline) {
    if (*a.deadline != *b.deadline) {
      using std::chrono::sys_days;
      return sys_days(*a.deadline) < sys_days(*b.deadline);
    }
  } else if (a.deadline.has_value() != b.deadline.has_value()) {
    return a.deadline.has_value();
  }
  return a.title < b.title;
}

int status_index(Status status) {
  auto it = std::ranges::find(kAllStatuses, status);
  return static_cast<int>(it - kAllStatuses.begin());
}

Column column_for(const Board& board, Status status) {
  Column col{status, display_name(status), board.column(status)};
  std::ranges::sort(col.items,
                    [](const Item* a, const Item* b) { return item_precedes(*a, *b); });
  return col;
}

std::vector<Column> columns(const Board& board) {
  std::vector<Column> out;
  out.reserve(kAllStatuses.size());
  for (auto s : kAllStatuses) out.push_back(column_for(board, s));
  return out;
}

} // namespace cb
