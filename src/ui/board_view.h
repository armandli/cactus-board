#pragma once

#include <vector>

#include <ftxui/dom/elements.hpp>

#include "board/column.h"

namespace cb::ui {

// Which card the user is on. `row` indexes into the sorted column, not into Board::items().
struct Selection {
  Status column = Status::Ready;
  int row = 0;
};

// Pulls `row` back into range; board mutations behind the UI's back can orphan it.
void clamp(Selection& sel, const std::vector<Column>& cols);

// Null when the selected column is empty.
const Item* selected_item(const Selection& sel, const std::vector<Column>& cols);

ftxui::Element render_column(const Column& col, bool is_current, int selected_row);
ftxui::Element render_board(const std::vector<Column>& cols, const Selection& sel);
// The only place an item's description is shown.
ftxui::Element render_detail(const Item* item);

} // namespace cb::ui
