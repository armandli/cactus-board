#include <cstdlib>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <nlohmann/json.hpp>

#include "board/board.h"
#include "nl/needle_client.h"
#include "nl/tools.h"

namespace {

using namespace ftxui;

std::string model_path() {
  if (const char* env = std::getenv("NEEDLE_MODEL")) return env;
  return NEEDLE_MODEL_PATH;
}

cb::Board sample_board() {
  cb::Board board;
  board.add("Set up build", cb::Status::Done);
  board.add("Fix login bug", cb::Status::InProgress);
  board.add("Write docs");
  board.add("API refactor");
  return board;
}

// Sends one English command through Needle and applies the resulting tool calls.
std::string run_command(cb::NeedleClient& needle, cb::Board& board, const std::string& command) {
  auto response = needle.complete(command);
  if (!response) return "error: " + response.error();

  auto result = cb::apply_function_calls(board, nlohmann::json::parse(*response));
  std::string status;
  for (const auto& msg : result.applied) status += msg + "; ";
  for (const auto& err : result.errors) status += "error: " + err + "; ";
  return status.empty() ? "no matching action" : status;
}

void print_board(const cb::Board& board) {
  for (auto s : cb::kAllStatuses) {
    std::println("{}:", cb::display_name(s));
    for (const auto* item : board.column(s)) std::println("  #{} {}", item->id, item->title);
  }
}

Element render_column(const cb::Board& board, cb::Status status) {
  Elements cards;
  for (const auto* item : board.column(status)) {
    cards.push_back(text(item->title) | border);
  }
  return window(text(std::string(cb::display_name(status))) | bold | center, vbox(std::move(cards)) | flex)
       | flex;
}

int run_tui(cb::NeedleClient& needle, cb::Board& board) {
  auto app = App::Fullscreen();

  std::string command;
  std::string status_line = "Type a command (e.g. \"move write docs to in progress\") and press Enter. Esc quits.";

  InputOption input_opt;
  input_opt.multiline = false;
  input_opt.on_enter = [&] {
    if (command.empty()) return;
    status_line = run_command(needle, board, command);
    command.clear();
  };
  auto input = Input(&command, "command...", input_opt);

  auto root = Renderer(input, [&] {
    Elements columns;
    for (auto s : cb::kAllStatuses) columns.push_back(render_column(board, s));
    return vbox({
      text("cactus-board") | bold | center,
      hbox(std::move(columns)) | flex,
      hbox({text("> "), input->Render()}) | border,
      text(status_line) | dim,
    });
  });
  root |= CatchEvent([&](const Event& e) {
    if (e == Event::Escape) {
      app.Exit();
      return true;
    }
    return false;
  });

  app.Loop(root);
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  std::optional<std::string> nl_command;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--nl" && i + 1 < argc) nl_command = argv[++i];
  }

  auto needle = cb::NeedleClient::create(model_path(), cb::board_tools_json());
  if (!needle) {
    std::println(std::cerr, "failed to start Needle: {}", needle.error());
    return 1;
  }

  auto board = sample_board();

  // Headless mode: apply a single command and print the board.
  if (nl_command) {
    std::println("{}", run_command(*needle, board, *nl_command));
    print_board(board);
    return 0;
  }

  return run_tui(*needle, board);
}
