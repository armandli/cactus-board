# cactus-board
Kanban board driven using Cactus Needle model

A terminal Kanban board (C++26, [FTXUI](https://github.com/ArthurSonzogni/FTXUI)) whose cards can be
changed with plain-English commands, translated into tool calls on-device by
[Cactus Needle 3](https://cactuscompute.com/needle).

## Requirements
- macOS on Apple Silicon (Needle engine is `macos-arm64`)
- A C++26 compiler (e.g. Homebrew LLVM clang), CMake >= 3.28, curl

## Build & run
```sh
make            # downloads Needle 3 into needle3/ (first time), configures and builds
make test       # builds and runs the GoogleTest suite via ctest
make run        # launches the TUI (q quits)
make release    # optimized build in build-release/
make clean      # removes build directories
make distclean  # also removes the downloaded Needle files
```

Headless mode applies one command and prints the board:
```sh
./build/src/cactus-board --nl "move docs to progressing"
```

## Keys
```
h j k l / arrows   move the selection within and between columns
H / L              move the selected card one column left / right
d                  show the selected card's description
i                  type a natural-language command (Esc cancels)
q                  quit
```

## Layout
```
src/board/   Work item model and status columns (items, ordering, moves)
src/ui/      FTXUI rendering and keyboard interaction
src/nl/      Needle C API wrapper + board tool schemas and dispatch
src/main.cpp Argument parsing, seed data and the headless path
test/        GoogleTest unit tests
needle3/     Needle 3 engine, header and weights (downloaded)
cmake/       CMake helpers (Needle imported target)
```
