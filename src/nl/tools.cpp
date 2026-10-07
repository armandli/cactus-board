#include "nl/tools.h"

#include <charconv>
#include <format>
#include <stdexcept>
#include <utility>

namespace cb {

namespace {

using nlohmann::json;
// Needle is sensitive to key order in tool schemas (name first), so build them with ordered_json.
using ordered_json = nlohmann::ordered_json;

ordered_json text_param(std::string_view description) {
  return {
    {"type", "string"},
    {"description", description},
  };
}

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

std::string join(const std::vector<std::string>& parts) {
  std::string out;
  for (const auto& part : parts) {
    if (!out.empty()) out += "; ";
    out += part;
  }
  return out;
}

int priority_arg(std::string_view name, const json& value) {
  int priority = 0;
  if (value.is_number_integer()) {
    priority = value.get<int>();
  } else if (value.is_string()) {
    // A small model emits "1" about as often as 1.
    auto digits = value.get<std::string>();
    const char* end = digits.data() + digits.size();
    auto [stop, ec] = std::from_chars(digits.data(), end, priority);
    if (ec != std::errc{} || stop != end)
      throw std::runtime_error(std::format("{}: priority '{}' is not a number", name, digits));
  } else {
    throw std::runtime_error(std::format("{}: priority must be a number", name));
  }
  if (priority < 1)
    throw std::runtime_error(std::format("{}: priority {} must be 1 or more", name, priority));
  return priority;
}

// Returns the phrase for the call in the infinitive, so callers can pick the tense: the
// preview prefixes "will ", the post-hoc summary prefixes "done: ".
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
  auto text_of = [&](const char* key) {
    auto it = args.find(key);
    if (it == args.end() || it->is_null()) return std::string{};
    if (!it->is_string()) throw std::runtime_error(std::format("{}: {} must be a string", name, key));
    return it->get<std::string>();
  };

  if (name == "add_item") {
    Item item;
    item.title = title;
    item.status = status_of(Status::Ready);
    item.owner = text_of("owner");
    item.category = text_of("category");
    item.description = text_of("description");
    if (auto p = args.find("priority"); p != args.end() && !p->is_null())
      item.priority = priority_arg(name, *p);
    if (auto d = args.find("deadline"); d != args.end() && !d->is_null()) {
      auto date = parse_date(text_of("deadline"));
      if (!date)
        throw std::runtime_error(std::format("{}: bad deadline '{}', expected YYYY-MM-DD", name,
                                             d->dump()));
      item.deadline = *date;
    }
    Status placed = item.status;
    board.add(std::move(item));
    return std::format("add '{}' to {}", title, display_name(placed));
  }
  if (name == "move_item") {
    const Item* item = existing();
    Status s = status_of(item->status);
    std::string found = item->title;
    board.move(item->id, s);
    return std::format("move '{}' to {}", found, display_name(s));
  }
  if (name == "remove_item") {
    const Item* item = existing();
    std::string found = item->title;
    board.remove(item->id);
    return std::format("remove '{}'", found);
  }
  throw std::runtime_error(std::format("unknown tool '{}'", name));
}

} // namespace

std::string board_tools_json() {
  ordered_json title = text_param("Title of the card");
  ordered_json tools = ordered_json::array({
    tool("add_item", "Add a new card to the kanban board.",
         {{"title", title},
          {"status", status_param("Column to place the card in")},
          {"owner", text_param("Person responsible for the card")},
          {"category", text_param("Project the card belongs to")},
          {"priority", {{"type", "integer"}, {"description", "1 is the highest priority; defaults to 3"}}},
          {"deadline", text_param("Due date as YYYY-MM-DD")},
          {"description", text_param("Detail text about the card")}},
         {"title"}),
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

std::string Plan::describe() const {
  if (executable()) return "will " + join(summaries);
  if (!errors.empty()) return "cannot: " + join(errors);
  return "no matching action";
}

Plan plan_response(const Board& board, const json& response) {
  Board scratch = board;
  auto result = apply_function_calls(scratch, response);

  Plan plan;
  auto calls = response.find("function_calls");
  plan.calls = calls != response.end() && calls->is_array() ? *calls : json::array();
  plan.summaries = std::move(result.applied);
  plan.errors = std::move(result.errors);
  return plan;
}

std::expected<Plan, std::string> propose_command(NeedleClient& needle, const Board& board,
                                                 const std::string& command) {
  auto response = needle.complete(command);
  if (!response) return std::unexpected(response.error());

  auto parsed = json::parse(*response, nullptr, false);
  if (parsed.is_discarded()) return std::unexpected("Needle did not reply with valid JSON");

  Plan plan = plan_response(board, parsed);
  plan.command = command;
  return plan;
}

ApplyResult apply_plan(Board& board, const Plan& plan) {
  return apply_function_calls(board, json{{"function_calls", plan.calls}});
}

std::string summarize(const ApplyResult& result) {
  std::string out;
  if (!result.applied.empty()) out = "done: " + join(result.applied);
  for (const auto& err : result.errors) {
    if (!out.empty()) out += "; ";
    out += "error: " + err;
  }
  return out.empty() ? "no matching action" : out;
}

std::string apply_command(NeedleClient& needle, Board& board, const std::string& command) {
  auto proposed = propose_command(needle, board, command);
  if (!proposed) return "error: " + proposed.error();
  if (!proposed->executable()) return proposed->describe();
  return summarize(apply_plan(board, *proposed));
}

} // namespace cb
