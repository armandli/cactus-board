#include "board/board.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace cb {

namespace {

std::string to_lower(std::string_view s) {
  std::string out(s);
  std::ranges::transform(out, out.begin(), [](unsigned char c) { return std::tolower(c); });
  return out;
}

} // namespace

std::string_view to_string(Status s) {
  switch (s) {
    case Status::Todo: return "todo";
    case Status::InProgress: return "in_progress";
    case Status::Done: return "done";
  }
  return "todo";
}

std::string_view display_name(Status s) {
  switch (s) {
    case Status::Todo: return "To Do";
    case Status::InProgress: return "In Progress";
    case Status::Done: return "Done";
  }
  return "To Do";
}

std::optional<Status> parse_status(std::string_view s) {
  auto lower = to_lower(s);
  if (lower == "todo" || lower == "to do" || lower == "to_do") return Status::Todo;
  if (lower == "in_progress" || lower == "in progress" || lower == "doing") return Status::InProgress;
  if (lower == "done") return Status::Done;
  return std::nullopt;
}

int Board::add(std::string title, Status status) {
  int id = next_id_++;
  items_.push_back({id, std::move(title), status});
  return id;
}

bool Board::move(int id, Status status) {
  Item* item = find_mut(id);
  if (!item) return false;
  item->status = status;
  return true;
}

bool Board::remove(int id) {
  return std::erase_if(items_, [id](const Item& i) { return i.id == id; }) > 0;
}

const Item* Board::find(int id) const {
  auto it = std::ranges::find(items_, id, &Item::id);
  return it == items_.end() ? nullptr : &*it;
}

Item* Board::find_mut(int id) {
  return const_cast<Item*>(std::as_const(*this).find(id));
}

const Item* Board::find_by_title(std::string_view title) const {
  auto needle = to_lower(title);
  for (const auto& item : items_) {
    if (to_lower(item.title) == needle) return &item;
  }
  const Item* match = nullptr;
  for (const auto& item : items_) {
    if (to_lower(item.title).contains(needle)) {
      if (match) return nullptr; // ambiguous
      match = &item;
    }
  }
  return match;
}

std::vector<const Item*> Board::column(Status status) const {
  std::vector<const Item*> out;
  for (const auto& item : items_) {
    if (item.status == status) out.push_back(&item);
  }
  return out;
}

} // namespace cb
