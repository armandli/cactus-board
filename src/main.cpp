#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>

#include "board/board.h"
#include "board/column.h"
#include "board/store.h"
#include "nl/needle_client.h"
#include "nl/tools.h"
#include "ui/app.h"

namespace {

std::string model_path() {
  if (const char* env = std::getenv("NEEDLE_MODEL")) return env;
  return NEEDLE_MODEL_PATH;
}

void print_board(const cb::Board& board) {
  for (const auto& col : cb::columns(board)) {
    std::println("{} ({}):", col.name, col.items.size());
    for (const auto* item : col.items) {
      std::println("  #{} {} [P{}] {} · {}{}", item->id, item->title, item->priority,
                   item->owner.empty() ? "unassigned" : item->owner,
                   item->category.empty() ? "none" : item->category,
                   item->deadline ? " · due " + cb::format_date(*item->deadline) : "");
    }
  }
}

} // namespace

int main(int argc, char** argv) {
  std::filesystem::path path = "board.json";
  std::optional<std::string> nl_command;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--file" && i + 1 < argc) {
      path = argv[++i];
    } else if (arg == "--nl" && i + 1 < argc) {
      nl_command = argv[++i];
    } else {
      std::println(std::cerr, "usage: cactus-board [--file <path>] [--nl <command>]");
      return 2;
    }
  }

  // Load before Needle starts, so a bad file fails fast instead of after a slow model load.
  auto board = cb::load_board(path);
  if (!board) {
    std::println(std::cerr, "{}", board.error());
    return 1;
  }

  auto needle = cb::NeedleClient::create(model_path(), cb::board_tools_json());
  if (!needle) {
    std::println(std::cerr, "failed to start Needle: {}", needle.error());
    return 1;
  }

  // Headless mode: apply a single command and print the board.
  if (nl_command) {
    std::println("{}", cb::apply_command(*needle, *board, *nl_command));
    print_board(*board);
  } else if (int rc = cb::ui::run_tui(*needle, *board); rc != 0) {
    return rc;
  }

  // Reported after the fullscreen screen is torn down, so the message is actually visible.
  if (auto saved = cb::save_board(path, *board); !saved) {
    std::println(std::cerr, "{}", saved.error());
    return 1;
  }
  return 0;
}
