// bbdsq/movegen.h
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h" // For U64, board constants, and square_to_algebraic (used in Move::to_string)
#include "piece.h"    // For Player enum, PieceType, PIECE_CHARS, BoardState (for generate_all_legal_moves)
#include <vector>     // For std::vector<Move>
#include <string>     // For std::string in Move::to_string

// Represents a single game move
struct Move {
    int from_sq;
    int to_sq;
    PieceType piece_moved;    // Type of the piece that moved
    PieceType piece_captured; // Type of the piece captured (NO_PIECE_TYPE if none)

    Move(int from = -1, int to = -1, PieceType moved = NO_PIECE_TYPE, PieceType captured = NO_PIECE_TYPE)
        : from_sq(from), to_sq(to), piece_moved(moved), piece_captured(captured) {}

    bool operator==(const Move& other) const {
        return from_sq == other.from_sq && to_sq == other.to_sq &&
               piece_moved == other.piece_moved && piece_captured == other.piece_captured;
    }

    std::string to_string() const {
        std::string s = "";
        if (from_sq != -1 && to_sq != -1) {
            s += square_to_algebraic(from_sq);
            s += (piece_captured != NO_PIECE_TYPE ? "x" : "-"); 
            s += square_to_algebraic(to_sq);
            if (piece_moved != NO_PIECE_TYPE) {
                 s += " (";
                 s += PIECE_CHARS[piece_moved];
                 s += ")";
            }
        } else {
            s = "NullMove";
        }
        return s;
    }
};


// Generates one-step orthogonal moves for a piece.
// Landing square can be empty or occupied by an enemy (capture).
// Piece cannot move onto a square occupied by a friendly piece.
// Piece cannot move into its own den.
// Piece must land on a land square.
U64 generate_orthogonal_step_moves(
    int piece_square_idx, 
    U64 friendly_occupancy, 
    U64 land_squares_mask,
    U64 own_den_mask
);

// Generates moves for a Rat.
// Rat can move to adjacent land or water squares.
// Landing square can be empty or occupied by an enemy (capture).
// Cannot move to a square occupied by a friendly piece.
// Cannot move into its own den.
U64 generate_rat_moves(
    int piece_square_idx,
    U64 friendly_occupancy, 
    U64 own_den_mask,
    U64 lake_squares_mask   
);

// Generates JUMP moves for Lion or Tiger.
// Can jump over a river (straight jumps over 2 or 3 river squares).
// Landing square can be empty or occupied by an enemy (capture).
// Cannot jump if a rat (any player's rat) is in the intervening river squares.
// Cannot jump to a square occupied by a friendly piece.
// Cannot jump into its own den.
U64 generate_lion_tiger_jump_moves(
    int piece_square_idx,
    Player moving_player,   
    U64 friendly_occupancy, 
    U64 all_rat_occupancy,  
    U64 own_den_mask,
    U64 lake_squares_mask   
);

// Generates all legal moves for the given player from the current board state,
// including checks for 3-fold repetition.
std::vector<Move> generate_all_legal_moves(
    const BoardState& board_state,          // Current state to generate moves from
    Player player_to_move,                  // Player whose moves are being generated
    const std::vector<BoardState>& game_history // Full game history to check for repetitions
);


#endif // MOVEGEN_H


