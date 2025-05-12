// bbdsq/bitboard.cpp

#include "bitboard.h"
#include <iostream>  // For std::cout in main (if testing here) or debug prints
#include <algorithm> // For std::reverse in pretty_print_bb's hex conversion
#include <vector>    // For initializing masks if needed (not strictly here, but good include)
#include <iomanip>   // For std::hex and std::setw / std::setfill for hex printing

// --- Definition of Pre-calculated Masks ---
U64 FILE_A_MASK = 0ULL;
U64 FILE_H_MASK = 0ULL; // G-File
U64 RANK_1_MASK = 0ULL;
U64 RANK_9_MASK = 0ULL;

U64 LAKE_SQUARES_MASK = 0ULL;

// Player 1 (Computer, Top)
U64 P1_DEN_SQUARE_MASK = 0ULL;

// Player 2 (Human, Bottom)
U64 P2_DEN_SQUARE_MASK = 0ULL;

// Trap Squares
U64 TRAPS_NEAR_P1_DEN_MASK = 0ULL;
U64 TRAPS_NEAR_P2_DEN_MASK = 0ULL;

U64 LAND_SQUARES_MASK = 0ULL;


void init_masks() {
    // Clear all masks first in case init_masks is called multiple times (though it shouldn't be)
    FILE_A_MASK = 0ULL;
    FILE_H_MASK = 0ULL;
    RANK_1_MASK = 0ULL;
    RANK_9_MASK = 0ULL;
    LAKE_SQUARES_MASK = 0ULL;
    P1_DEN_SQUARE_MASK = 0ULL;
    P2_DEN_SQUARE_MASK = 0ULL;
    TRAPS_NEAR_P1_DEN_MASK = 0ULL;
    TRAPS_NEAR_P2_DEN_MASK = 0ULL;
    LAND_SQUARES_MASK = 0ULL;

    // File Masks (A=col 0, G=col 6)
    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        set_bit(FILE_A_MASK, get_square_index(0, r)); // File A
        set_bit(FILE_H_MASK, get_square_index(BOARD_WIDTH - 1, r)); // File G
    }

    // Rank Masks (1=row 0, 9=row 8)
    for (int c = 0; c < BOARD_WIDTH; ++c) {
        set_bit(RANK_1_MASK, get_square_index(c, 0)); // Rank 1 (Bottom, P2/Human)
        set_bit(RANK_9_MASK, get_square_index(c, BOARD_HEIGHT - 1)); // Rank 9 (Top, P1/Computer)
    }

    // Lake Squares (DSQ specific)
    // Rows 3, 4, 5 (0-indexed, so rows 4, 5, 6 in 1-indexed notation)
    // Columns B, C, E, F (0-indexed, so cols 1, 2, 4, 5)
    int lake_rows[] = {3, 4, 5}; // 0-indexed rows
    int lake_cols[] = {1, 2, 4, 5}; // 0-indexed columns
    for (int r_idx : lake_rows) {
        for (int c_idx : lake_cols) {
            set_bit(LAKE_SQUARES_MASK, get_square_index(c_idx, r_idx));
        }
    }

    // Player 1 (Computer, Top, Den at D9 (col 3, row 8))
    set_bit(P1_DEN_SQUARE_MASK, get_square_index(3, 8)); // D9

    // Player 2 (Human, Bottom, Den at D1 (col 3, row 0))
    set_bit(P2_DEN_SQUARE_MASK, get_square_index(3, 0)); // D1

    // Traps near P1's Den (top of board: C9, E9, D8)
    // Indices: (2,8); (4,8); (3,7)
    set_bit(TRAPS_NEAR_P1_DEN_MASK, get_square_index(2, 8)); // C9
    set_bit(TRAPS_NEAR_P1_DEN_MASK, get_square_index(4, 8)); // E9
    set_bit(TRAPS_NEAR_P1_DEN_MASK, get_square_index(3, 7)); // D8

    // Traps near P2's Den (bottom of board: C1, E1, D2)
    // Indices: (2,0); (4,0); (3,1)
    set_bit(TRAPS_NEAR_P2_DEN_MASK, get_square_index(2, 0)); // C1
    set_bit(TRAPS_NEAR_P2_DEN_MASK, get_square_index(4, 0)); // E1
    set_bit(TRAPS_NEAR_P2_DEN_MASK, get_square_index(3, 1)); // D2

    // Land Squares
    // Create a mask for all squares on the board first
    U64 all_board_squares_temp = 0ULL;
    for(int i=0; i < NUM_SQUARES; ++i) {
        set_bit(all_board_squares_temp, i);
    }
    // Land is all board squares that are not lake squares
    LAND_SQUARES_MASK = all_board_squares_temp & (~LAKE_SQUARES_MASK);
}


std::string pretty_print_bb(U64 bb) {
    std::string s = "";
    s += "\n  +---------------+\n"; // Adjusted for 7 columns
    for (int r_model = BOARD_HEIGHT - 1; r_model >= 0; --r_model) { // Print from model row 8 down to 0
        s += std::to_string(r_model + 1) + " | "; // Row number (1-9)
        for (int c_model = 0; c_model < BOARD_WIDTH; ++c_model) {
            int sq = get_square_index(c_model, r_model);
            if (get_bit(bb, sq)) {
                s += "1 ";
            } else {
                s += ". ";
            }
        }
        s += "|\n";
    }
    s += "  +---------------+\n";
    s += "    A B C D E F G\n";
    
    // Hex representation
    // Using stringstream for formatted hex output
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << bb;
    s += "Bitboard Value (Hex): " + ss.str() + "\n";
    
    return s;
}


