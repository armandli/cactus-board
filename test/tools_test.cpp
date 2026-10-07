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

json one_call(json name, json arguments) {
  return response({{{"name", std::move(name)}, {"arguments", std::move(arguments)}}});
}

bool mentions(const std::string& text, std::string_view fragment) {
  return text.find(fragment) != std::string::npos;
}

TEST(ToolsTest, ToolsJsonDeclaresAllTools) {
  auto tools = json::parse(board_tools_json());
  ASSERT_TRUE(tools.is_array());
  std::vector<std::string> names;
  for (const auto& t : tools) names.push_back(t["name"]);
  EXPECT_EQ(names, (std::vector<std::string>{"add_item", "move_item", "remove_item"}));
}

TEST(ToolsTest, ToolsJsonExposesEveryAddItemField) {
  auto tools = json::parse(board_tools_json());
  const auto& props = tools[0]["parameters"]["properties"];
  for (const char* key : {"title", "status", "owner", "category", "priority", "deadline",
                          "description"})
    EXPECT_TRUE(props.contains(key)) << key;
  EXPECT_EQ(props.size(), 7u);
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

TEST(PlanTest, DryRunLeavesTheBoardUntouched) {
  Board board;
  int keep = board.add("Write docs");
  board.add("Ship it", Status::Progressing);

  auto plan = plan_response(board, response({
    {{"name", "add_item"}, {"arguments", {{"title", "Deploy"}}}},
    {{"name", "move_item"}, {"arguments", {{"title", "Write docs"}, {"status", "complete"}}}},
    {{"name", "remove_item"}, {"arguments", {{"title", "Ship it"}}}},
  }));

  EXPECT_TRUE(plan.executable());
  EXPECT_EQ(plan.summaries.size(), 3u);
  EXPECT_EQ(board.items().size(), 2u);
  EXPECT_EQ(board.find(keep)->status, Status::Ready);
  EXPECT_NE(board.find_by_title("Ship it"), nullptr);
  EXPECT_EQ(board.find_by_title("Deploy"), nullptr);
}

TEST(PlanTest, ApplyPlanMatchesWhatWasPreviewed) {
  Board board;
  board.add("Write docs");

  auto plan = plan_response(board, one_call("move_item", {{"title", "Write docs"}, {"status", "complete"}}));
  auto applied = apply_plan(board, plan);

  EXPECT_EQ(applied.applied, plan.summaries);
  EXPECT_TRUE(applied.errors.empty());
  EXPECT_EQ(board.find_by_title("Write docs")->status, Status::Complete);
}

TEST(PlanTest, AddThenMoveInOneResponseIsExecutable) {
  Board board;
  auto plan = plan_response(board, response({
    {{"name", "add_item"}, {"arguments", {{"title", "Ship it"}}}},
    {{"name", "move_item"}, {"arguments", {{"title", "Ship it"}, {"status", "progressing"}}}},
  }));

  EXPECT_TRUE(plan.executable());
  EXPECT_TRUE(board.items().empty());

  apply_plan(board, plan);
  EXPECT_EQ(board.find_by_title("Ship it")->status, Status::Progressing);
}

TEST(PlanTest, UnknownCardIsNotExecutable) {
  Board board;
  auto plan = plan_response(board, one_call("move_item", {{"title", "Nope"}, {"status", "complete"}}));

  EXPECT_FALSE(plan.executable());
  EXPECT_TRUE(mentions(plan.describe(), "Nope"));
}

TEST(PlanTest, UnknownStatusIsNotExecutable) {
  Board board;
  board.add("Write docs");
  auto plan = plan_response(board, one_call("move_item", {{"title", "Write docs"}, {"status", "later"}}));

  EXPECT_FALSE(plan.executable());
  EXPECT_TRUE(mentions(plan.describe(), "later"));
}

TEST(PlanTest, UnparseableDeadlineIsNotExecutable) {
  Board board;
  auto plan = plan_response(board, one_call("add_item", {{"title", "Deploy"}, {"deadline", "next Friday"}}));

  EXPECT_FALSE(plan.executable());
  EXPECT_TRUE(mentions(plan.describe(), "deadline"));
}

TEST(PlanTest, PriorityBelowOneIsNotExecutable) {
  Board board;
  auto plan = plan_response(board, one_call("add_item", {{"title", "Deploy"}, {"priority", 0}}));

  EXPECT_FALSE(plan.executable());
  EXPECT_TRUE(mentions(plan.describe(), "priority"));
}

TEST(PlanTest, UnknownToolIsNotExecutable) {
  Board board;
  auto plan = plan_response(board, one_call("rename_item", {{"title", "Deploy"}}));

  EXPECT_FALSE(plan.executable());
}

TEST(PlanTest, MissingAndEmptyCallListsAreNotExecutable) {
  Board board;
  EXPECT_FALSE(plan_response(board, json{{"type", "text"}}).executable());

  auto empty = plan_response(board, response(json::array()));
  EXPECT_FALSE(empty.executable());
  EXPECT_EQ(empty.describe(), "no matching action");
}

TEST(PlanTest, AddItemCarriesEveryField) {
  Board board;
  auto plan = plan_response(board, one_call("add_item", {{"title", "Deploy"},
                                                         {"status", "progressing"},
                                                         {"owner", "sam"},
                                                         {"category", "platform"},
                                                         {"priority", 1},
                                                         {"deadline", "2026-12-01"},
                                                         {"description", "Cut the release."}}));
  ASSERT_TRUE(plan.executable());
  apply_plan(board, plan);

  const Item* item = board.find_by_title("Deploy");
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->status, Status::Progressing);
  EXPECT_EQ(item->owner, "sam");
  EXPECT_EQ(item->category, "platform");
  EXPECT_EQ(item->priority, 1);
  ASSERT_TRUE(item->deadline.has_value());
  EXPECT_EQ(format_date(*item->deadline), "2026-12-01");
  EXPECT_EQ(item->description, "Cut the release.");
}

TEST(PlanTest, PriorityAcceptsNumberOrNumericString) {
  Board board;
  apply_plan(board, plan_response(board, one_call("add_item", {{"title", "one"}, {"priority", 1}})));
  apply_plan(board, plan_response(board, one_call("add_item", {{"title", "two"}, {"priority", "2"}})));

  EXPECT_EQ(board.find_by_title("one")->priority, 1);
  EXPECT_EQ(board.find_by_title("two")->priority, 2);
}

TEST(PlanTest, OmittedFieldsKeepItemDefaults) {
  Board board;
  apply_plan(board, plan_response(board, one_call("add_item", {{"title", "Deploy"}})));

  const Item* item = board.find_by_title("Deploy");
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->status, Status::Ready);
  EXPECT_EQ(item->priority, 3);
  EXPECT_TRUE(item->owner.empty());
  EXPECT_TRUE(item->category.empty());
  EXPECT_TRUE(item->description.empty());
  EXPECT_FALSE(item->deadline.has_value());
}

TEST(PlanTest, WordingIsTensedByTheCaller) {
  Board board;
  board.add("Write docs");
  auto plan = plan_response(board, one_call("move_item", {{"title", "Write docs"}, {"status", "complete"}}));

  EXPECT_EQ(plan.describe(), "will move 'Write docs' to Complete");
  EXPECT_EQ(summarize(apply_plan(board, plan)), "done: move 'Write docs' to Complete");

  auto rejected = plan_response(board, one_call("remove_item", {{"title", "Nope"}}));
  EXPECT_TRUE(rejected.describe().starts_with("cannot: "));
}

} // namespace
} // namespace cb
