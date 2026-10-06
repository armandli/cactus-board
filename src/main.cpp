#include <cstdlib>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>

#include "board/board.h"
#include "board/column.h"
#include "nl/needle_client.h"
#include "nl/tools.h"
#include "ui/app.h"

namespace {

std::string model_path() {
  if (const char* env = std::getenv("NEEDLE_MODEL")) return env;
  return NEEDLE_MODEL_PATH;
}

cb::Item work_item(std::string title, std::string owner, std::string category, int priority,
                   std::string_view deadline, cb::Status status, std::string description) {
  cb::Item item;
  item.title = std::move(title);
  item.owner = std::move(owner);
  item.category = std::move(category);
  item.priority = priority;
  item.deadline = cb::parse_date(deadline);
  item.status = status;
  item.description = std::move(description);
  return item;
}

cb::Board sample_board() {
  cb::Board board;
  board.add(work_item("auth", "sam", "platform", 1, "2026-10-20", cb::Status::Ready,
                      "Replace the session cookie with a signed token and rotate keys daily."));
  board.add(work_item("docs", "priya", "platform", 2, "2026-10-14", cb::Status::Ready,
                      "Write the getting-started page and document every keybinding."));
  board.add(work_item("refactor", "lee", "core", 2, "", cb::Status::Ready,
                      "Split the parser into a tokenizer and an evaluator."));
  board.add(work_item("ingest", "priya", "pipeline", 1, "2026-11-30", cb::Status::Progressing,
                      "Batch the upstream feed so a slow consumer cannot stall the queue."));
  board.add(work_item("telemetry", "sam", "pipeline", 3, "", cb::Status::Progressing,
                      "Emit per-request latency histograms to the metrics endpoint."));
  board.add(work_item("build", "lee", "core", 2, "2026-09-30", cb::Status::Complete,
                      "CMake presets plus a release target with link-time optimization."));
  return board;
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
    std::println("{}", cb::apply_command(*needle, board, *nl_command));
    print_board(board);
    return 0;
  }

  return cb::ui::run_tui(*needle, board);
}
