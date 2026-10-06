#include "ui/board_view.h"

#include <algorithm>
#include <format>
#include <utility>

#include "ui/card.h"

namespace cb::ui {

using namespace ftxui;

namespace {

const Column* find_column(const std::vector<Column>& cols, Status status) {
  auto it = std::ranges::find(cols, status, &Column::status);
  return it == cols.end() ? nullptr : &*it;
}

Element field(std::string_view label, std::string value) {
  return hbox({text(std::string(label)) | dim, text(std::move(value))});
}

} // namespace

void clamp(Selection& sel, const std::vector<Column>& cols) {
  const Column* col = find_column(cols, sel.column);
  if (!col) {
    sel.column = Status::Ready;
    sel.row = 0;
    return;
  }
  int size = static_cast<int>(col->items.size());
  sel.row = size == 0 ? 0 : std::clamp(sel.row, 0, size - 1);
}

const Item* selected_item(const Selection& sel, const std::vector<Column>& cols) {
  const Column* col = find_column(cols, sel.column);
  if (!col || col->items.empty()) return nullptr;
  if (sel.row < 0 || sel.row >= static_cast<int>(col->items.size())) return nullptr;
  return col->items[sel.row];
}

Element render_column(const Column& col, bool is_current, int selected_row) {
  Elements cards;
  for (int i = 0; i < static_cast<int>(col.items.size()); ++i) {
    cards.push_back(render_card(*col.items[i], is_current && i == selected_row));
  }
  if (cards.empty()) cards.push_back(text("empty") | dim | center);
  cards.push_back(filler());

  auto header = text(std::format("{} ({})", col.name, col.items.size())) | bold | center;
  if (is_current) header = header | bgcolor(Color::Blue);

  return window(header, vscroll_indicator(yframe(vbox(std::move(cards)))) | flex) | flex;
}

Element render_board(const std::vector<Column>& cols, const Selection& sel) {
  Elements rendered;
  for (const auto& col : cols) {
    rendered.push_back(render_column(col, col.status == sel.column, sel.row));
  }
  return hbox(std::move(rendered));
}

Element render_detail(const Item* item) {
  if (!item) {
    return window(text("Details") | bold, text("no card selected") | dim);
  }
  return window(text(item->title) | bold,
                vbox({
                  field("owner: ", item->owner.empty() ? "unassigned" : item->owner),
                  field("category: ", item->category.empty() ? "none" : item->category),
                  field("priority: ", std::format("{}", item->priority)),
                  field("deadline: ",
                        item->deadline ? format_date(*item->deadline) : "none"),
                  separator(),
                  paragraph(item->description.empty() ? "(no description)"
                                                      : item->description),
                }));
}

} // namespace cb::ui
