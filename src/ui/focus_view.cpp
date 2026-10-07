#include "ui/focus_view.h"

#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace cb::ui {

using namespace ftxui;

namespace {

// A fixed label width keeps the values in one column at full screen width.
Element field(std::string_view label, std::string value) {
  return hbox({
    text(std::string(label)) | dim | size(WIDTH, EQUAL, 10),
    text(std::move(value)),
  });
}

} // namespace

Element render_focus(const Item* item) {
  if (!item) {
    return window(text("Focus") | bold, text("no card selected") | dim | center) | flex;
  }

  // `center` is a no-op on a window title, so the header is left-aligned in the border.
  auto header = text(std::format("#{} {}", item->id, item->title)) | bold;
  return window(header,
                vbox({
                  // Status leads: it is the one field the board states only by position.
                  field("status", std::string(display_name(item->status))),
                  field("owner", item->owner.empty() ? "unassigned" : item->owner),
                  field("category", item->category.empty() ? "none" : item->category),
                  field("priority", std::format("{}", item->priority)),
                  field("deadline",
                        item->deadline ? format_date(*item->deadline) : "none"),
                  separator(),
                  paragraph(item->description.empty() ? "(no description)"
                                                      : item->description),
                  filler(),
                })) |
         flex;
}

} // namespace cb::ui
