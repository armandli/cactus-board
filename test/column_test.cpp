#include <gtest/gtest.h>

#include "board/column.h"

namespace cb {
namespace {

Item make(std::string title, int priority, std::string_view deadline,
          Status status = Status::Ready) {
  Item item;
  item.title = std::move(title);
  item.priority = priority;
  item.deadline = parse_date(deadline);
  item.status = status;
  return item;
}

std::vector<std::string> titles(const Column& col) {
  std::vector<std::string> out;
  for (const auto* item : col.items) out.push_back(item->title);
  return out;
}

TEST(ColumnTest, LowerPrioritySortsFirst) {
  Board board;
  board.add(make("c", 3, ""));
  board.add(make("a", 1, ""));
  board.add(make("b", 2, ""));

  EXPECT_EQ(titles(column_for(board, Status::Ready)),
            (std::vector<std::string>{"a", "b", "c"}));
}

TEST(ColumnTest, EqualPriorityOrdersByNearerDeadline) {
  Board board;
  board.add(make("later", 2, "2026-12-01"));
  board.add(make("sooner", 2, "2026-10-14"));

  EXPECT_EQ(titles(column_for(board, Status::Ready)),
            (std::vector<std::string>{"sooner", "later"}));
}

TEST(ColumnTest, DeadlineOutranksNoDeadlineAtEqualPriority) {
  Board board;
  board.add(make("undated", 2, ""));
  board.add(make("dated", 2, "2027-01-01"));

  EXPECT_EQ(titles(column_for(board, Status::Ready)),
            (std::vector<std::string>{"dated", "undated"}));
}

TEST(ColumnTest, PriorityBeatsDeadline) {
  Board board;
  board.add(make("urgent-later", 1, "2027-01-01"));
  board.add(make("minor-sooner", 5, "2026-10-01"));

  EXPECT_EQ(titles(column_for(board, Status::Ready)),
            (std::vector<std::string>{"urgent-later", "minor-sooner"}));
}

TEST(ColumnTest, FallsBackToTitleWhenPriorityAndDeadlineTie) {
  Board board;
  board.add(make("zeta", 2, ""));
  board.add(make("alpha", 2, ""));
  board.add(make("mid", 2, ""));

  EXPECT_EQ(titles(column_for(board, Status::Ready)),
            (std::vector<std::string>{"alpha", "mid", "zeta"}));
}

TEST(ColumnTest, IdenticalDeadlinesFallBackToTitle) {
  Board board;
  board.add(make("zeta", 2, "2026-10-14"));
  board.add(make("alpha", 2, "2026-10-14"));

  EXPECT_EQ(titles(column_for(board, Status::Ready)),
            (std::vector<std::string>{"alpha", "zeta"}));
}

TEST(ColumnTest, ColumnsCoverEveryStatusInOrder) {
  Board board;
  board.add(make("r", 1, "", Status::Ready));
  board.add(make("p", 1, "", Status::Progressing));
  board.add(make("c", 1, "", Status::Complete));

  auto cols = columns(board);
  ASSERT_EQ(cols.size(), kAllStatuses.size());
  for (std::size_t i = 0; i < cols.size(); ++i) {
    EXPECT_EQ(cols[i].status, kAllStatuses[i]);
    EXPECT_EQ(cols[i].name, display_name(kAllStatuses[i]));
    EXPECT_EQ(cols[i].items.size(), 1u);
    EXPECT_EQ(cols[i].items.front()->status, kAllStatuses[i]);
  }
  EXPECT_EQ(cols[0].name, "Ready");
  EXPECT_EQ(cols[1].name, "Progressing");
  EXPECT_EQ(cols[2].name, "Complete");
}

TEST(ColumnTest, EmptyColumnIsPresentButEmpty) {
  Board board;
  board.add(make("only", 1, "", Status::Ready));

  auto cols = columns(board);
  ASSERT_EQ(cols.size(), 3u);
  EXPECT_EQ(cols[0].items.size(), 1u);
  EXPECT_TRUE(cols[1].items.empty());
  EXPECT_TRUE(cols[2].items.empty());
}

TEST(ColumnTest, StatusIndexMatchesColumnOrder) {
  EXPECT_EQ(status_index(Status::Ready), 0);
  EXPECT_EQ(status_index(Status::Progressing), 1);
  EXPECT_EQ(status_index(Status::Complete), 2);
}

} // namespace
} // namespace cb
