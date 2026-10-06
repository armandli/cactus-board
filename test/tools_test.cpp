#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "nl/tools.h"

namespace cb {
namespace {

using nlohmann::json;

// Shape of a real Needle 3 response, trimmed to the fields we use.
json response(json calls) {
  return {{"type", "call"}, {"function_calls", std::move(calls)}, {"confidence", 0.7}};
}

TEST(ToolsTest, ToolsJsonDeclaresAllTools) {
  auto tools = json::parse(board_tools_json());
  ASSERT_TRUE(tools.is_array());
  std::vector<std::string> names;
  for (const auto& t : tools) names.push_back(t["name"]);
  EXPECT_EQ(names, (std::vector<std::string>{"add_item", "move_item", "remove_item"}));
}

TEST(ToolsTest, AddItem) {
  Board board;
  auto r = apply_function_calls(board, response({{{"name", "add_item"}, {"arguments", {{"title", "Write docs"}, {"status", "todo"}}}}}));

  EXPECT_TRUE(r.errors.empty());
  ASSERT_NE(board.find_by_title("Write docs"), nullptr);
  EXPECT_EQ(board.find_by_title("Write docs")->status, Status::Ready);
}

TEST(ToolsTest, AddItemAcceptsCurrentWireNames) {
  Board board;
  auto r = apply_function_calls(board, response({{{"name", "add_item"}, {"arguments", {{"title", "Ship it"}, {"status", "progressing"}}}}}));

  EXPECT_TRUE(r.errors.empty());
  ASSERT_NE(board.find_by_title("Ship it"), nullptr);
  EXPECT_EQ(board.find_by_title("Ship it")->status, Status::Progressing);
}

TEST(ToolsTest, MoveItemMatchesTitleCaseInsensitively) {
  Board board;
  int id = board.add("fix login bug");
  auto r = apply_function_calls(board, response({{{"name", "move_item"}, {"arguments", {{"title", "Fix login bug"}, {"status", "done"}}}}}));

  EXPECT_TRUE(r.errors.empty());
  EXPECT_EQ(board.find(id)->status, Status::Complete);
}

TEST(ToolsTest, RemoveItem) {
  Board board;
  board.add("Write docs");
  auto r = apply_function_calls(board, response({{{"name", "remove_item"}, {"arguments", {{"title", "write docs"}}}}}));

  EXPECT_TRUE(r.errors.empty());
  EXPECT_TRUE(board.items().empty());
}

TEST(ToolsTest, MultipleCallsApplyInOrder) {
  Board board;
  auto r = apply_function_calls(board, response({
    {{"name", "add_item"}, {"arguments", {{"title", "Ship it"}}}},
    {{"name", "move_item"}, {"arguments", {{"title", "Ship it"}, {"status", "in_progress"}}}},
  }));

  EXPECT_EQ(r.applied.size(), 2u);
  EXPECT_EQ(board.find_by_title("Ship it")->status, Status::Progressing);
}

TEST(ToolsTest, UnknownCardIsReportedNotThrown) {
  Board board;
  auto r = apply_function_calls(board, response({{{"name", "move_item"}, {"arguments", {{"title", "Nope"}, {"status", "done"}}}}}));

  EXPECT_TRUE(r.applied.empty());
  EXPECT_EQ(r.errors.size(), 1u);
}

TEST(ToolsTest, EmptyCallListChangesNothing) {
  Board board;
  board.add("Write docs");
  auto r = apply_function_calls(board, response(json::array()));

  EXPECT_TRUE(r.applied.empty());
  EXPECT_TRUE(r.errors.empty());
  EXPECT_EQ(board.items().size(), 1u);
}

} // namespace
} // namespace cb
