// bbdsq/ai.h
#ifndef AI_H
#define AI_H

#include "piece.h"       // For BoardState, Player, PieceType, etc.
#include "movegen.h"     // For Move struct and generate_all_legal_moves
#include "evaluation.h"  // For evaluate_board and WIN_SCORE/LOSS_SCORE
#include <vector>
#include <limits>       // For std::numeric_limits

// --- AI Configuration ---
const int DEFAULT_AI_SEARCH_DEPTH = 6;  // recommended depths: for release-version: 6-7 , for debug-version: 5

// Struct to hold the results of the AI's move search
struct AiMoveResult {
    Move best_move;
    int final_score;        
    long long nodes_searched; 
    double time_taken_ms;   
    int root_moves_count;   

    AiMoveResult() : 
        final_score(std::numeric_limits<int>::min()), 
        nodes_searched(0), 
        time_taken_ms(0.0), 
        root_moves_count(0) 
    {
        best_move = Move(); 
    }
};


// --- Helper Function ---
// Applies a move to a given board state and returns the new state.
BoardState make_move_on_copy(const BoardState& current_board_state, const Move& move);


// --- Alpha-Beta Search Function ---
// Returns the evaluation of the board from the perspective of 'player_for_whom_to_maximize'.
// 'nodes_searched_ref' is passed by reference to accumulate the node count.
// 'game_history_ref' provides the history of board states for repetition checking.
int alpha_beta_search(
    BoardState board_state, 
    int depth,
    int alpha,
    int beta,
    Player player_for_whom_to_maximize, 
    Player current_turn_in_state,
    long long& nodes_searched_ref,
    const std::vector<BoardState>& game_history_ref // <<<< ADDED for repetition checking
);


// --- Root AI Move Selection Function ---
// Finds the best move for the AI (PLAYER_1) and gathers search statistics.
// 'game_history_ref' provides the history of board states for repetition checking.
AiMoveResult find_best_ai_move(
    const BoardState& current_board_state, 
    int search_depth,
    const std::vector<BoardState>& game_history_ref // <<<< ADDED for repetition checking
);


#endif // AI_H


