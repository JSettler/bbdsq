// bbdsq/zobrist.cpp
#include "zobrist.h"
#include <random>    // For std::mt19937_64 and std::uniform_int_distribution
#include <iostream>  // For potential debug output during initialization

namespace Zobrist {

    // Define the actual Zobrist key arrays
    U64 piece_keys[NUM_PIECE_TYPES][3][NUM_SQUARES];
    U64 side_to_move_key[3]; // side_to_move_key[PLAYER_1], side_to_move_key[PLAYER_2]
                             // side_to_move_key[NO_PLAYER] will be 0

    void initialize_keys() {
        // Use a fixed seed for the pseudo-random number generator to ensure
        // that Zobrist keys are the same every time the program runs.
        // This is crucial for consistent hashing and for transposition tables if used later.
        std::mt19937_64 rng(0xDEADBEEFCAFEBABEULL); // A fixed 64-bit seed
        std::uniform_int_distribution<U64> distrib(0, std::numeric_limits<U64>::max());

        for (int pt_val = 0; pt_val < NUM_PIECE_TYPES; ++pt_val) {
            PieceType pt = static_cast<PieceType>(pt_val);
            for (int p_val = 0; p_val < 3; ++p_val) { // Iterate 0, 1, 2 for NO_PLAYER, PLAYER_1, PLAYER_2
                Player p = static_cast<Player>(p_val);
                for (int sq = 0; sq < NUM_SQUARES; ++sq) {
                    if (pt == NO_PIECE_TYPE || p == NO_PLAYER) {
                        piece_keys[pt_val][p_val][sq] = 0ULL; // No key for "no piece" or "no player"
                    } else {
                        // Ensure keys are non-zero for actual pieces/players to avoid hash collisions with empty states
                        U64 random_val = 0ULL;
                        while(random_val == 0ULL) { // Extremely unlikely to get 0, but good practice
                            random_val = distrib(rng);
                        }
                        piece_keys[pt_val][p_val][sq] = random_val;
                    }
                }
            }
        }

        side_to_move_key[NO_PLAYER] = 0ULL; // No hash contribution if game is over or side is undefined
        
        U64 p1_key = 0ULL;
        while(p1_key == 0ULL) p1_key = distrib(rng);
        side_to_move_key[PLAYER_1] = p1_key;

        U64 p2_key = 0ULL;
        while(p2_key == 0ULL || p2_key == p1_key) p2_key = distrib(rng); // Ensure P2 key is different from P1
        side_to_move_key[PLAYER_2] = p2_key;

        // std::cout << "Zobrist keys initialized." << std::endl;
        // std::cout << "Sample P1 Rat key at A1: 0x" << std::hex << piece_keys[RAT][PLAYER_1][0] << std::dec << std::endl;
        // std::cout << "Side to move P1 key: 0x" << std::hex << side_to_move_key[PLAYER_1] << std::dec << std::endl;
        // std::cout << "Side to move P2 key: 0x" << std::hex << side_to_move_key[PLAYER_2] << std::dec << std::endl;
    }

    // Calculates the Zobrist hash for a given board state from scratch.
    U64 calculate_initial_hash(const BoardState& board_state) {
        U64 current_hash = 0ULL;

        // XOR in keys for all pieces on the board
        for (int pt_val = RAT; pt_val < NUM_PIECE_TYPES; ++pt_val) { // Iterate actual piece types
            PieceType pt = static_cast<PieceType>(pt_val);
            for (int p_val = PLAYER_1; p_val <= PLAYER_2; ++p_val) { // Iterate actual players
                Player player = static_cast<Player>(p_val);
                U64 bb = board_state.piece_bbs[pt][player];
                U64 temp_bb = bb;
                while (temp_bb > 0) {
                    int sq = pop_lsb(temp_bb);
                    if (sq != -1) { // Should always be true if temp_bb > 0
                        current_hash ^= piece_keys[pt_val][p_val][sq];
                    }
                }
            }
        }
        
        // XOR in the key for the side to move
        if (board_state.side_to_move != NO_PLAYER) {
             current_hash ^= side_to_move_key[board_state.side_to_move];
        }
        // Note: If other state like castling rights or en-passant square (for chess)
        // were part of BoardState, their Zobrist keys would also be XORed here.

        return current_hash;
    }

} // namespace Zobrist