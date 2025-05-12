// bbdsq/zobrist.h
#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "bitboard.h" // For U64
#include "piece.h"    // For PieceType, Player, NUM_PIECE_TYPES, NUM_SQUARES
                      // Forward declare BoardState if it's not fully needed here
                      // or include piece.h which defines BoardState.

// Forward declaration of BoardState to avoid circular dependency if BoardState includes zobrist.h later
// However, calculate_hash needs the full definition. So, piece.h is better.
// struct BoardState; // Forward declaration

namespace Zobrist {

    // Zobrist keys:
    // piece_keys[piece_type_index][player_index][square_index]
    // Player index: 0 for NO_PLAYER (keys will be 0), 1 for PLAYER_1, 2 for PLAYER_2
    // PieceType index: 0 for NO_PIECE_TYPE (keys will be 0)
    extern U64 piece_keys[NUM_PIECE_TYPES][3][NUM_SQUARES]; 
    
    // side_to_move_key[player_index]
    // XORed in if it's that player's turn.
    // side_to_move_key[NO_PLAYER] will be 0.
    extern U64 side_to_move_key[3]; 

    // Initializes all Zobrist keys with pseudo-random numbers.
    // Must be called once at the very start of the program.
    void initialize_keys();

    // Calculates the Zobrist hash for a given board state from scratch.
    // This iterates over all pieces on the board.
    U64 calculate_initial_hash(const BoardState& board_state);

    // --- Functions for incremental hash updates (more efficient) ---
    // These would typically be called by BoardState's move/add/remove methods.

    // XORs the key for a piece being placed on or removed from a square.
    inline void xor_piece_at_sq(U64& current_hash, PieceType pt, Player p, int sq) {
        if (pt != NO_PIECE_TYPE && p != NO_PLAYER && sq >= 0 && sq < NUM_SQUARES) {
            current_hash ^= piece_keys[pt][p][sq];
        }
    }

    // XORs the key for the side to move.
    inline void xor_side_to_move(U64& current_hash, Player side) {
        if (side != NO_PLAYER) {
            current_hash ^= side_to_move_key[side];
        }
    }

} // namespace Zobrist

#endif // ZOBRIST_H