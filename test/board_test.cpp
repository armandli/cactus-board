#include <gtest/gtest.h>

#include "board/board.h"

namespace cb {
namespace {

TEST(BoardTest, AddPlacesItemInRequestedColumn) {
  Board board;
  int id = board.add("Write docs", Status::Progressing);

  ASSERT_NE(board.find(id), nullptr);
  EXPECT_EQ(board.find(id)->status, Status::Progressing);
  EXPECT_EQ(board.column(Status::Progressing).size(), 1u);
  EXPECT_TRUE(board.column(Status::Ready).empty());
}

TEST(BoardTest, AddDefaultsToReady) {
  Board board;
  int id = board.add("Write docs");
  EXPECT_EQ(board.find(id)->status, Status::Ready);
}

TEST(BoardTest, AddItemKeepsFieldsAndAssignsFreshId) {
  Board board;
  Item item;
  item.id = 99; // must be ignored
  item.title = "auth";
  item.owner = "sam";
  item.description = "rotate keys";
  item.category = "platform";
  item.priority = 1;
  item.deadline = parse_date("2026-11-30");
  item.status = Status::Progressing;

  int id = board.add(item);
  ASSERT_NE(board.find(id), nullptr);
  const Item& stored = *board.find(id);

  EXPECT_NE(id, 99);
  EXPECT_EQ(stored.id, id);
  EXPECT_EQ(stored.title, "auth");
  EXPECT_EQ(stored.owner, "sam");
  EXPECT_EQ(stored.description, "rotate keys");
  EXPECT_EQ(stored.category, "platform");
  EXPECT_EQ(stored.priority, 1);
  EXPECT_EQ(stored.deadline, parse_date("2026-11-30"));
  EXPECT_EQ(stored.status, Status::Progressing);
}

TEST(BoardTest, MoveChangesStatus) {
  Board board;
  int id = board.add("Write docs");
  EXPECT_TRUE(board.move(id, Status::Complete));
  EXPECT_EQ(board.find(id)->status, Status::Complete);
}

TEST(BoardTest, MoveUnknownIdFails) {
  Board board;
  EXPECT_FALSE(board.move(42, Status::Complete));
}

TEST(BoardTest, RemoveDeletesItem) {
  Board board;
  int id = board.add("Write docs");
  EXPECT_TRUE(board.remove(id));
  EXPECT_EQ(board.find(id), nullptr);
  EXPECT_FALSE(board.remove(id));
}

TEST(BoardTest, FindByTitleIsCaseInsensitive) {
  Board board;
  int id = board.add("Fix login bug");
  ASSERT_NE(board.find_by_title("fix LOGIN bug"), nullptr);
  EXPECT_EQ(board.find_by_title("fix LOGIN bug")->id, id);
}

TEST(BoardTest, FindByTitleFallsBackToUniqueSubstring) {
  Board board;
  int id = board.add("Fix login bug");
  board.add("Write docs");
  ASSERT_NE(board.find_by_title("login"), nullptr);
  EXPECT_EQ(board.find_by_title("login")->id, id);
}

TEST(BoardTest, FindByTitleRejectsAmbiguousSubstring) {
  Board board;
  board.add("Fix login bug");
  board.add("Fix logout bug");
  EXPECT_EQ(board.find_by_title("fix"), nullptr);
}

TEST(StatusTest, ParseRoundTrips) {
  for (auto s : kAllStatuses) {
    EXPECT_EQ(parse_status(to_string(s)), s);
  }
  EXPECT_EQ(parse_status("Progressing"), Status::Progressing);
  EXPECT_EQ(parse_status("blocked"), std::nullopt);
}

TEST(StatusTest, ParseAcceptsLegacySpellings) {
  EXPECT_EQ(parse_status("todo"), Status::Ready);
  EXPECT_EQ(parse_status("to do"), Status::Ready);
  EXPECT_EQ(parse_status("backlog"), Status::Ready);
  EXPECT_EQ(parse_status("in_progress"), Status::Progressing);
  EXPECT_EQ(parse_status("in progress"), Status::Progressing);
  EXPECT_EQ(parse_status("doing"), Status::Progressing);
  EXPECT_EQ(parse_status("done"), Status::Complete);
}

TEST(DateTest, ParsesIsoDates) {
  auto d = parse_date("2026-11-30");
  ASSERT_TRUE(d.has_value());
  EXPECT_EQ(static_cast<int>(d->year()), 2026);
  EXPECT_EQ(static_cast<unsigned>(d->month()), 11u);
  EXPECT_EQ(static_cast<unsigned>(d->day()), 30u);
  EXPECT_EQ(format_date(*d), "2026-11-30");
}

TEST(DateTest, RejectsMalformedAndImpossibleDates) {
  EXPECT_EQ(parse_date(""), std::nullopt);
  EXPECT_EQ(parse_date("2026-13-45"), std::nullopt);
  EXPECT_EQ(parse_date("2026-02-30"), std::nullopt);
  EXPECT_EQ(parse_date("nonsense"), std::nullopt);
  EXPECT_EQ(parse_date("2026/11/30"), std::nullopt);
  EXPECT_EQ(parse_date("26-11-30"), std::nullopt);
}

} // namespace
} // namespace cb
