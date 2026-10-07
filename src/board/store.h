#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

#include "board/board.h"

namespace cb {

// nlohmann ADL hook, so std::vector<Item> serializes for free. Loading does not use the
// matching from_json hook because its errors have to name the offending element and field.
void to_json(nlohmann::ordered_json& j, const Item& item);

// A missing file yields an empty board. Anything present but invalid is an error, so a bad
// path or a broken hand-edit is never silently replaced by a fresh board.
std::expected<Board, std::string> load_board(const std::filesystem::path& path);

std::expected<void, std::string> save_board(const std::filesystem::path& path,
                                            const Board& board);

} // namespace cb
