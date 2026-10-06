add_library(cactus_board_core STATIC
  board/board.cpp
  board/column.cpp
  nl/needle_client.cpp
  nl/tools.cpp
)
target_include_directories(cactus_board_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cactus_board_core
  PUBLIC nlohmann_json::nlohmann_json
  PRIVATE needle::needle
)
target_compile_options(cactus_board_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(cactus_board_ui STATIC
  ui/card.cpp
  ui/board_view.cpp
  ui/app.cpp
)
target_link_libraries(cactus_board_ui PUBLIC
  cactus_board_core
  ftxui::screen
  ftxui::dom
  ftxui::component
)
target_compile_options(cactus_board_ui PRIVATE -Wall -Wextra -Wpedantic)

add_executable(cactus-board main.cpp)
target_compile_definitions(cactus-board PRIVATE NEEDLE_MODEL_PATH="${NEEDLE_MODEL_PATH}")
target_link_libraries(cactus-board PRIVATE cactus_board_ui)
target_compile_options(cactus-board PRIVATE -Wall -Wextra -Wpedantic)
