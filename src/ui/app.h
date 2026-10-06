#pragma once

#include "board/board.h"
#include "nl/needle_client.h"

namespace cb::ui {

// Runs the interactive kanban board until the user quits. Returns a process exit code.
int run_tui(NeedleClient& needle, Board& board);

} // namespace cb::ui
