#include "ui/app.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "board/column.h"
#include "nl/tools.h"
#include "ui/board_view.h"
#include "ui/focus_view.h"

namespace cb::ui {

using namespace ftxui;

namespace {

enum class View { Board, Focus };

constexpr std::string_view kNavHint =
    "hjkl/arrows move · H/L move card · f focus · i command · q quit";
constexpr std::string_view kFocusHint =
    "j/k prev/next card · H/L move card · f or Esc back · i command · q quit";
constexpr std::string_view kCommandHint = "Enter runs the command · Esc cancels";

} // namespace

int run_tui(NeedleClient& needle, Board& board) {
  auto app = App::Fullscreen();

  Selection sel;
  View view = View::Board;
  bool command_mode = false;
  std::string command;
  std::string status_line;

  InputOption input_opt;
  input_opt.multiline = false;
  input_opt.on_enter = [&] {
    if (!command.empty()) {
      status_line = apply_command(needle, board, command);
      command.clear();
    }
    command_mode = false;
  };
  auto input = Input(&command, "move docs to progressing", input_opt);

  auto root = Renderer(input, [&] {
    auto cols = columns(board);
    clamp(sel, cols);

    Elements rows{text("cactus-board") | bold | center};
    rows.push_back(view == View::Focus ? render_focus(selected_item(sel, cols)) | flex
                                      : render_board(cols, sel) | flex);
    rows.push_back(hbox({text("> "), input->Render()}) | border);

    auto hint = command_mode ? kCommandHint : view == View::Focus ? kFocusHint : kNavHint;
    rows.push_back(text(std::string(hint)) | dim);
    if (!status_line.empty()) rows.push_back(text(status_line) | dim);
    return vbox(std::move(rows));
  });

  // Moves the selected card `delta` columns over, keeping the highlight on that card.
  auto move_card = [&](int delta) {
    auto cols = columns(board);
    clamp(sel, cols);
    const Item* item = selected_item(sel, cols);
    if (!item) return;

    int target = status_index(sel.column) + delta;
    if (target < 0 || target >= static_cast<int>(kAllStatuses.size())) return;

    int id = item->id;
    Status dest = kAllStatuses[static_cast<std::size_t>(target)];
    board.move(id, dest);
    status_line = std::format("moved '{}' to {}", item->title, display_name(dest));

    sel.column = dest;
    auto moved = column_for(board, dest);
    auto it = std::ranges::find(moved.items, id, &Item::id);
    sel.row = it == moved.items.end() ? 0 : static_cast<int>(it - moved.items.begin());
  };

  auto select_column = [&](int delta) {
    int target = status_index(sel.column) + delta;
    if (target < 0 || target >= static_cast<int>(kAllStatuses.size())) return;
    sel.column = kAllStatuses[static_cast<std::size_t>(target)];
  };

  root |= CatchEvent([&](const Event& e) {
    if (command_mode) {
      if (e == Event::Escape) {
        command_mode = false;
        return true;
      }
      return false; // the Input owns the keyboard
    }

    if (e == Event::Character('q')) {
      app.Exit();
      return true;
    }
    if (view == View::Focus) {
      if (e == Event::Escape || e == Event::Character('f')) {
        view = View::Board;
        return true;
      }
    } else {
      if (e == Event::Escape) {
        app.Exit();
        return true;
      }
      if (e == Event::Character('f')) {
        view = View::Focus;
        return true;
      }
    }

    if (e == Event::ArrowUp || e == Event::Character('k')) {
      --sel.row;
      return true;
    }
    if (e == Event::ArrowDown || e == Event::Character('j')) {
      ++sel.row;
      return true;
    }
    if (e == Event::ArrowLeft || e == Event::Character('h')) {
      select_column(-1);
      return true;
    }
    if (e == Event::ArrowRight || e == Event::Character('l')) {
      select_column(1);
      return true;
    }
    if (e == Event::Character('H') || e == Event::Character('<')) {
      move_card(-1);
      return true;
    }
    if (e == Event::Character('L') || e == Event::Character('>')) {
      move_card(1);
      return true;
    }
    if (e == Event::Character('i') || e == Event::Character('/')) {
      command_mode = true;
      return true;
    }
    // Swallow anything else printable so it never lands in the command input.
    return e.is_character();
  });

  app.Loop(root);
  return 0;
}

} // namespace cb::ui
