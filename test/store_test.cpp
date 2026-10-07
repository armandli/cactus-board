#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "board/store.h"

namespace cb {
namespace {

namespace fs = std::filesystem;

class StoreTest : public ::testing::Test {
protected:
  void SetUp() override {
    dir_ = fs::temp_directory_path() /
           ("cb-store-" + std::string(::testing::UnitTest::GetInstance()
                                          ->current_test_info()
                                          ->name()));
    fs::remove_all(dir_);
    fs::create_directories(dir_);
    path_ = dir_ / "board.json";
  }

  void TearDown() override { fs::remove_all(dir_); }

  void write(std::string_view contents) {
    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    out << contents;
  }

  std::string read() {
    std::ifstream in(path_, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
  }

  fs::path dir_;
  fs::path path_;
};

Item full_item() {
  Item item;
  item.id = 7;
  item.title = "auth";
  item.owner = "sam";
  item.category = "platform";
  item.priority = 1;
  item.deadline = parse_date("2026-10-20");
  item.status = Status::Progressing;
  item.description = "Rotate keys daily.";
  return item;
}

TEST_F(StoreTest, RoundTripsEveryField) {
  Board original({full_item()});
  ASSERT_TRUE(save_board(path_, original));

  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  ASSERT_EQ(loaded->items().size(), 1u);

  const Item& item = loaded->items().front();
  EXPECT_EQ(item.id, 7);
  EXPECT_EQ(item.title, "auth");
  EXPECT_EQ(item.owner, "sam");
  EXPECT_EQ(item.category, "platform");
  EXPECT_EQ(item.priority, 1);
  EXPECT_EQ(item.deadline, parse_date("2026-10-20"));
  EXPECT_EQ(item.status, Status::Progressing);
  EXPECT_EQ(item.description, "Rotate keys daily.");
}

TEST_F(StoreTest, AbsentDeadlineIsOmittedFromTheFile) {
  Item item = full_item();
  item.deadline.reset();
  ASSERT_TRUE(save_board(path_, Board({item})));

  EXPECT_EQ(read().find("deadline"), std::string::npos);

  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_FALSE(loaded->items().front().deadline.has_value());
}

TEST_F(StoreTest, AddAfterLoadDoesNotReuseAnId) {
  ASSERT_TRUE(save_board(path_, Board({full_item()})));

  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_EQ(loaded->add("next"), 8);
}

TEST_F(StoreTest, EmptyBoardRoundTrips) {
  ASSERT_TRUE(save_board(path_, Board{}));
  EXPECT_EQ(read(), "[]\n");

  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_TRUE(loaded->items().empty());
}

TEST_F(StoreTest, MissingFileLoadsAnEmptyBoardWithoutError) {
  auto loaded = load_board(dir_ / "absent.json");
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_TRUE(loaded->items().empty());
}

TEST_F(StoreTest, MalformedJsonIsAnError) {
  write("not json at all");
  EXPECT_FALSE(load_board(path_));
}

TEST_F(StoreTest, TopLevelObjectIsAnError) {
  write(R"({"not": "an array"})");
  auto loaded = load_board(path_);
  ASSERT_FALSE(loaded);
  EXPECT_NE(loaded.error().find("array"), std::string::npos);
}

TEST_F(StoreTest, NonObjectElementIsAnError) {
  write("[42]");
  EXPECT_FALSE(load_board(path_));
}

TEST_F(StoreTest, DuplicateIdsAreAnError) {
  write(R"([{"id":7,"title":"a","status":"ready"},{"id":7,"title":"b","status":"ready"}])");
  auto loaded = load_board(path_);
  ASSERT_FALSE(loaded);
  EXPECT_NE(loaded.error().find("duplicate id 7"), std::string::npos);
}

TEST_F(StoreTest, NonPositiveIdIsAnError) {
  write(R"([{"id":0,"title":"a","status":"ready"}])");
  EXPECT_FALSE(load_board(path_));
}

TEST_F(StoreTest, UnknownStatusIsAnError) {
  write(R"([{"id":1,"title":"a","status":"blocked"}])");
  auto loaded = load_board(path_);
  ASSERT_FALSE(loaded);
  EXPECT_NE(loaded.error().find("blocked"), std::string::npos);
}

TEST_F(StoreTest, MissingTitleIsAnError) {
  write(R"([{"id":1,"status":"ready"}])");
  EXPECT_FALSE(load_board(path_));
}

TEST_F(StoreTest, EmptyTitleIsAnError) {
  write(R"([{"id":1,"title":"","status":"ready"}])");
  EXPECT_FALSE(load_board(path_));
}

TEST_F(StoreTest, BadDeadlineIsAnError) {
  write(R"([{"id":1,"title":"a","status":"ready","deadline":"2026-13-45"}])");
  auto loaded = load_board(path_);
  ASSERT_FALSE(loaded);
  EXPECT_NE(loaded.error().find("2026-13-45"), std::string::npos);
}

TEST_F(StoreTest, ErrorsNameTheFileAndTheElement) {
  write(R"([{"id":1,"title":"a","status":"ready"},{"id":2,"title":"b","status":"nope"}])");
  auto loaded = load_board(path_);
  ASSERT_FALSE(loaded);
  EXPECT_NE(loaded.error().find(path_.string()), std::string::npos);
  EXPECT_NE(loaded.error().find("item 1"), std::string::npos);
}

TEST_F(StoreTest, NullDeadlineLoadsAsNoDeadline) {
  write(R"([{"id":1,"title":"a","status":"ready","deadline":null}])");
  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_FALSE(loaded->items().front().deadline.has_value());
}

TEST_F(StoreTest, LegacyStatusSpellingsLoadAndNormalizeOnSave) {
  write(R"([{"id":1,"title":"a","status":"todo"},
            {"id":2,"title":"b","status":"in progress"},
            {"id":3,"title":"c","status":"done"}])");
  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_EQ(loaded->items()[0].status, Status::Ready);
  EXPECT_EQ(loaded->items()[1].status, Status::Progressing);
  EXPECT_EQ(loaded->items()[2].status, Status::Complete);

  ASSERT_TRUE(save_board(path_, *loaded));
  auto text = read();
  EXPECT_NE(text.find(R"("status": "progressing")"), std::string::npos);
  EXPECT_EQ(text.find(R"("status": "in progress")"), std::string::npos);
}

TEST_F(StoreTest, UnknownKeysAreIgnored) {
  write(R"([{"id":1,"title":"a","status":"ready","note":"hand-added"}])");
  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_EQ(loaded->items().front().title, "a");
}

TEST_F(StoreTest, MissingPriorityDefaultsToThree) {
  write(R"([{"id":1,"title":"a","status":"ready"}])");
  auto loaded = load_board(path_);
  ASSERT_TRUE(loaded) << loaded.error();
  EXPECT_EQ(loaded->items().front().priority, 3);
}

TEST_F(StoreTest, SaveLeavesNoTempFileBehind) {
  ASSERT_TRUE(save_board(path_, Board({full_item()})));
  EXPECT_FALSE(fs::exists(path_.string() + ".tmp"));
}

TEST_F(StoreTest, SaveReplacesALongerPreviousFileEntirely) {
  Board big({full_item()});
  big.add("second");
  big.add("third");
  ASSERT_TRUE(save_board(path_, big));

  ASSERT_TRUE(save_board(path_, Board{}));
  EXPECT_EQ(read(), "[]\n");
}

} // namespace
} // namespace cb
