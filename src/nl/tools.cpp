#include "nl/tools.h"

#include <format>
#include <stdexcept>

namespace cb {

namespace {

using nlohmann::json;
// Needle is sensitive to key order in tool schemas (name first), so build them with ordered_json.
using ordered_json = nlohmann::ordered_json;

ordered_json status_param(std::string_view description) {
  return {
    {"type", "string"},
    {"enum", {"ready", "progressing", "complete"}},
    {"description", description},
  };
}

ordered_json tool(std::string_view name, std::string_view description, ordered_json properties, ordered_json required) {
  return {
    {"name", name},
    {"description", description},
    {"parameters", {{"type", "object"}, {"properties", std::move(properties)}, {"required", std::move(required)}}},
  };
}

std::string apply_call(Board& board, const json& call) {
  auto name = call.value("name", "");
  const json& args = call.contains("arguments") ? call["arguments"] : json::object();
  auto title = args.value("title", "");
  if (title.empty()) throw std::runtime_error(std::format("{}: missing title", name));

  auto status_of = [&](Status fallback) {
    if (!args.contains("status")) return fallback;
    auto s = parse_status(args["status"].get<std::string>());
    if (!s) throw std::runtime_error(std::format("{}: unknown status '{}'", name, args["status"].get<std::string>()));
    return *s;
  };
  auto existing = [&] {
    const Item* item = board.find_by_title(title);
    if (!item) throw std::runtime_error(std::format("{}: no unique card matching '{}'", name, title));
    return item;
  };

  if (name == "add_item") {
    Status s = status_of(Status::Ready);
    board.add(title, s);
    return std::format("added '{}' to {}", title, display_name(s));
  }
  if (name == "move_item") {
    const Item* item = existing();
    Status s = status_of(item->status);
    std::string found = item->title;
    board.move(item->id, s);
    return std::format("moved '{}' to {}", found, display_name(s));
  }
  if (name == "remove_item") {
    const Item* item = existing();
    std::string found = item->title;
    board.remove(item->id);
    return std::format("removed '{}'", found);
  }
  throw std::runtime_error(std::format("unknown tool '{}'", name));
}

} // namespace

std::string board_tools_json() {
  ordered_json title = {{"type", "string"}, {"description", "Title of the card"}};
  ordered_json tools = ordered_json::array({
    tool("add_item", "Add a new card to the kanban board.",
         {{"title", title}, {"status", status_param("Column to place the card in")}}, {"title"}),
    tool("move_item", "Move an existing card to another column.",
         {{"title", title}, {"status", status_param("Destination column")}}, {"title", "status"}),
    tool("remove_item", "Delete a card from the board.",
         {{"title", title}}, {"title"}),
  });
  return tools.dump();
}

ApplyResult apply_function_calls(Board& board, const json& response) {
  ApplyResult result;
  if (!response.contains("function_calls") || !response["function_calls"].is_array()) {
    result.errors.push_back("response has no function_calls");
    return result;
  }
  for (const auto& call : response["function_calls"]) {
    try {
      result.applied.push_back(apply_call(board, call));
    } catch (const std::exception& e) {
      result.errors.push_back(e.what());
    }
  }
  return result;
}

std::string apply_command(NeedleClient& needle, Board& board, const std::string& command) {
  auto response = needle.complete(command);
  if (!response) return "error: " + response.error();

  auto result = apply_function_calls(board, json::parse(*response));
  std::string status;
  for (const auto& msg : result.applied) status += msg + "; ";
  for (const auto& err : result.errors) status += "error: " + err + "; ";
  return status.empty() ? "no matching action" : status;
}

} // namespace cb
