#pragma once

#include <ftxui/dom/elements.hpp>

#include "board/board.h"

namespace cb::ui {

// The whole-screen view of one work item: every field, including the status that the board
// conveys only through column position and the description the cards omit. Null when the
// selected column is empty.
ftxui::Element render_focus(const Item* item);

} // namespace cb::ui
