#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "board/board.h"

namespace cb {

// JSON array of the tools Needle may call to change the board.
std::string board_tools_json();

struct ApplyResult {
  std::vector<std::string> applied; // human-readable description of each change
  std::vector<std::string> errors;
};

// Applies the "function_calls" array of a Needle response to the board.
ApplyResult apply_function_calls(Board& board, const nlohmann::json& response);

} // namespace cb
