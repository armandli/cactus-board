#pragma once

#include <string_view>
#include <vector>

#include "board/board.h"

namespace cb {

// A status column ready for display: its header plus its items in display order.
struct Column {
  Status status;
  std::string_view name; // display_name(status)
  std::vector<const Item*> items;
};

// Display order: lower priority first; then the nearer deadline, with items that have no
// deadline sorted last; then title, so the order never depends on insertion sequence.
bool item_precedes(const Item& a, const Item& b);

// Position of `status` within kAllStatuses, i.e. its column index left to right.
int status_index(Status status);

Column column_for(const Board& board, Status status);
// Items alias the board's storage, so the result is only valid until the board is mutated.
std::vector<Column> columns(const Board& board);

} // namespace cb
