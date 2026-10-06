#pragma once

#include <ftxui/dom/elements.hpp>

#include "board/board.h"

namespace cb::ui {

// One work-item cell block: title, owner, category, priority and deadline.
// The item's description is deliberately never rendered here.
ftxui::Element render_card(const Item& item, bool selected);

} // namespace cb::ui
