#include <gtest/gtest.h>

#include "board/board.h"

namespace cb {
namespace {

TEST(BoardTest, AddPlacesItemInRequestedColumn) {
  Board board;
  int id = board.add("Write docs", Status::InProgress);

  ASSERT_NE(board.find(id), nullptr);
  EXPECT_EQ(board.find(id)->status, Status::InProgress);
  EXPECT_EQ(board.column(Status::InProgress).size(), 1u);
  EXPECT_TRUE(board.column(Status::Todo).empty());
}

TEST(BoardTest, AddDefaultsToTodo) {
  Board board;
  int id = board.add("Write docs");
  EXPECT_EQ(board.find(id)->status, Status::Todo);
}

TEST(BoardTest, MoveChangesStatus) {
  Board board;
  int id = board.add("Write docs");
  EXPECT_TRUE(board.move(id, Status::Done));
  EXPECT_EQ(board.find(id)->status, Status::Done);
}

TEST(BoardTest, MoveUnknownIdFails) {
  Board board;
  EXPECT_FALSE(board.move(42, Status::Done));
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
  EXPECT_EQ(parse_status("In Progress"), Status::InProgress);
  EXPECT_EQ(parse_status("blocked"), std::nullopt);
}

} // namespace
} // namespace cb
