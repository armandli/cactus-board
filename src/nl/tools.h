#pragma once

#include <expected>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "board/board.h"
#include "nl/needle_client.h"

namespace cb {

// JSON array of the tools Needle may call to change the board.
std::string board_tools_json();

struct ApplyResult {
  std::vector<std::string> applied; // human-readable description of each change
  std::vector<std::string> errors;
};

// Applies the "function_calls" array of a Needle response to the board.
ApplyResult apply_function_calls(Board& board, const nlohmann::json& response);

// A Needle response already dry-run against a copy of the board, so what it would do can be
// shown before the real board changes.
struct Plan {
  std::string command;                // the text that produced it
  nlohmann::json calls;               // the response's function_calls, replayed on confirm
  std::vector<std::string> summaries; // what each call would do, in order
  std::vector<std::string> errors;

  bool executable() const { return errors.empty() && !summaries.empty(); }
  // One line for under the input box: "will move 'docs' to Complete", "cannot: ..." or
  // "no matching action".
  std::string describe() const;
};

// Dry-runs the response against a copy of `board`; `board` itself is not touched.
Plan plan_response(const Board& board, const nlohmann::json& response);

// Sends one command through Needle and dry-runs the reply.
std::expected<Plan, std::string> propose_command(NeedleClient& needle, const Board& board,
                                                 const std::string& command);

// Replays a plan on the real board.
ApplyResult apply_plan(Board& board, const Plan& plan);

// One line describing what apply_plan did: "done: move 'docs' to Complete".
std::string summarize(const ApplyResult& result);

// Proposes and immediately applies one English command, for the headless path where there is
// no terminal to confirm in.
std::string apply_command(NeedleClient& needle, Board& board, const std::string& command);

} // namespace cb
