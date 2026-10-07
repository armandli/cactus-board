#include "board/store.h"

#include <format>
#include <fstream>
#include <set>
#include <system_error>
#include <utility>

namespace cb {

namespace {

using nlohmann::json;

std::string field_error(std::size_t index, std::string_view message) {
  return std::format("item {}: {}", index, message);
}

std::expected<Item, std::string> item_from(const json& j, std::size_t index) {
  if (!j.is_object()) return std::unexpected(field_error(index, "expected an object"));

  Item item;

  auto id = j.find("id");
  if (id == j.end() || !id->is_number_integer())
    return std::unexpected(field_error(index, "missing integer id"));
  item.id = id->get<int>();
  if (item.id <= 0)
    return std::unexpected(field_error(index, std::format("id {} must be positive", item.id)));

  auto title = j.find("title");
  if (title == j.end() || !title->is_string() || title->get<std::string>().empty())
    return std::unexpected(field_error(index, "missing title"));
  item.title = title->get<std::string>();

  auto status = j.find("status");
  if (status == j.end() || !status->is_string())
    return std::unexpected(field_error(index, "missing status"));
  auto parsed = parse_status(status->get<std::string>());
  if (!parsed)
    return std::unexpected(
        field_error(index, std::format("unknown status '{}'", status->get<std::string>())));
  item.status = *parsed;

  auto priority = j.find("priority");
  if (priority != j.end() && !priority->is_null()) {
    if (!priority->is_number_integer())
      return std::unexpected(field_error(index, "priority must be an integer"));
    item.priority = priority->get<int>();
  }

  auto text = [&](const char* key, std::string& out) -> std::optional<std::string> {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return std::nullopt;
    if (!it->is_string()) return std::format("{} must be a string", key);
    out = it->get<std::string>();
    return std::nullopt;
  };
  for (auto [key, out] : {std::pair{"owner", &item.owner},
                          std::pair{"category", &item.category},
                          std::pair{"description", &item.description}}) {
    if (auto err = text(key, *out)) return std::unexpected(field_error(index, *err));
  }

  auto deadline = j.find("deadline");
  if (deadline != j.end() && !deadline->is_null()) {
    if (!deadline->is_string())
      return std::unexpected(field_error(index, "deadline must be a string"));
    auto date = parse_date(deadline->get<std::string>());
    if (!date)
      return std::unexpected(field_error(
          index, std::format("bad deadline '{}'", deadline->get<std::string>())));
    item.deadline = *date;
  }

  return item;
}

} // namespace

void to_json(nlohmann::ordered_json& j, const Item& item) {
  j = nlohmann::ordered_json{{"id", item.id},
                             {"title", item.title},
                             {"owner", item.owner},
                             {"category", item.category},
                             {"priority", item.priority}};
  if (item.deadline) j["deadline"] = format_date(*item.deadline);
  j["status"] = to_string(item.status);
  j["description"] = item.description;
}

std::expected<Board, std::string> load_board(const std::filesystem::path& path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) return Board{};

  std::ifstream in(path, std::ios::binary);
  if (!in) return std::unexpected(std::format("{}: cannot be read", path.string()));

  json parsed = json::parse(in, nullptr, false);
  if (parsed.is_discarded())
    return std::unexpected(std::format("{}: is not valid JSON", path.string()));
  if (!parsed.is_array())
    return std::unexpected(
        std::format("{}: expected a JSON array of work items", path.string()));

  std::vector<Item> items;
  items.reserve(parsed.size());
  std::set<int> ids;
  for (std::size_t i = 0; i < parsed.size(); ++i) {
    auto item = item_from(parsed[i], i);
    if (!item) return std::unexpected(std::format("{}: {}", path.string(), item.error()));
    if (!ids.insert(item->id).second)
      return std::unexpected(std::format("{}: {}", path.string(),
                                         field_error(i, std::format("duplicate id {}", item->id))));
    items.push_back(std::move(*item));
  }

  return Board{std::move(items)};
}

std::expected<void, std::string> save_board(const std::filesystem::path& path,
                                            const Board& board) {
  auto temp = path;
  temp += ".tmp";

  {
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) return std::unexpected(std::format("{}: cannot be written", temp.string()));
    out << nlohmann::ordered_json(board.items()).dump(2) << '\n';
    if (!out) return std::unexpected(std::format("{}: write failed", temp.string()));
  }

  std::error_code ec;
  std::filesystem::rename(temp, path, ec);
  if (ec) {
    std::filesystem::remove(temp, ec);
    return std::unexpected(std::format("{}: cannot be replaced", path.string()));
  }
  return {};
}

} // namespace cb
