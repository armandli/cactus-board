#include "board/board.h"

#include <algorithm>
#include <cctype>
#include <format>
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
    case Status::Ready: return "ready";
    case Status::Progressing: return "progressing";
    case Status::Complete: return "complete";
  }
  return "ready";
}

std::string_view display_name(Status s) {
  switch (s) {
    case Status::Ready: return "Ready";
    case Status::Progressing: return "Progressing";
    case Status::Complete: return "Complete";
  }
  return "Ready";
}

std::optional<Status> parse_status(std::string_view s) {
  auto lower = to_lower(s);
  if (lower == "ready" || lower == "todo" || lower == "to do" || lower == "to_do" ||
      lower == "backlog")
    return Status::Ready;
  if (lower == "progressing" || lower == "in_progress" || lower == "in progress" ||
      lower == "doing")
    return Status::Progressing;
  if (lower == "complete" || lower == "done") return Status::Complete;
  return std::nullopt;
}

std::optional<Date> parse_date(std::string_view s) {
  if (s.size() != 10 || s[4] != '-' || s[7] != '-') return std::nullopt;
  auto number = [s](std::size_t pos, std::size_t len) -> std::optional<int> {
    int value = 0;
    for (std::size_t i = pos; i < pos + len; ++i) {
      if (s[i] < '0' || s[i] > '9') return std::nullopt;
      value = value * 10 + (s[i] - '0');
    }
    return value;
  };

  auto year = number(0, 4);
  auto month = number(5, 2);
  auto day = number(8, 2);
  if (!year || !month || !day) return std::nullopt;

  Date date{std::chrono::year{*year}, std::chrono::month{static_cast<unsigned>(*month)},
            std::chrono::day{static_cast<unsigned>(*day)}};
  if (!date.ok()) return std::nullopt;
  return date;
}

std::string format_date(Date d) {
  return std::format("{:04}-{:02}-{:02}", static_cast<int>(d.year()),
                     static_cast<unsigned>(d.month()), static_cast<unsigned>(d.day()));
}

Board::Board(std::vector<Item> items) : items_(std::move(items)) {
  for (const auto& item : items_) next_id_ = std::max(next_id_, item.id + 1);
}

int Board::add(Item item) {
  item.id = next_id_++;
  int id = item.id;
  items_.push_back(std::move(item));
  return id;
}

int Board::add(std::string title, Status status) {
  Item item;
  item.title = std::move(title);
  item.status = status;
  return add(std::move(item));
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
