#include "ui/card.h"

#include <format>
#include <utility>

namespace cb::ui {

using namespace ftxui;

Element render_card(const Item& item, bool selected) {
  // `dim` on top of `inverted` renders as mush, so drop it while selected.
  auto subtle = [selected](Element e) {
    return selected ? std::move(e) : std::move(e) | dim;
  };

  Elements rows;
  rows.push_back(hbox({
    text(item.title) | bold,
    filler(),
    subtle(text(std::format("P{}", item.priority))),
  }));

  std::string byline = item.owner.empty() ? "unassigned" : item.owner;
  if (!item.category.empty()) byline += " · " + item.category;
  rows.push_back(subtle(text(byline)));

  if (item.deadline) rows.push_back(subtle(text("due " + format_date(*item.deadline))));

  auto body = vbox(std::move(rows));
  if (selected) return focus(body | inverted | borderDouble);
  return body | border;
}

} // namespace cb::ui
