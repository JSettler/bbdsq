// bbdsq/board_state_io.h
#ifndef BOARD_STATE_IO_H
#define BOARD_STATE_IO_H

#include "piece.h" // For BoardState, Player enum
#include <string>
#include <vector> // For std::vector<BoardState>

const std::string DEFAULT_SAVE_FILENAME = "bbdsq_savegame.txt";

// Saves the current game state, history, and game status to a file.
// Returns true on success, false on failure.
bool save_game_state(
    const BoardState& board_state_to_save,         // The current board state
    const std::vector<BoardState>& history_to_save, // The game history
    int current_history_ply_to_save,                // Current index in history
    bool game_over_status_to_save,                  // Current game_over flag
    Player winner_status_to_save,                   // Current winner
    const std::string& filename = DEFAULT_SAVE_FILENAME
);

// Loads a game state, history, and game status from a file.
// Modifies the passed-by-reference arguments.
// Returns true on success, false on failure (e.g., file not found, format error).
bool load_game_state(
    BoardState& board_state_to_load_into,       // Will be populated with loaded current board state
    std::vector<BoardState>& history_to_load_into, // Will be populated with loaded history
    int& current_history_ply_to_load_into,         // Will be populated with loaded history index
    bool& game_over_status_to_load_into,          // Will be populated with loaded game_over flag
    Player& winner_status_to_load_into,           // Will be populated with loaded winner
    const std::string& filename = DEFAULT_SAVE_FILENAME
);

#endif // BOARD_STATE_IO_H


