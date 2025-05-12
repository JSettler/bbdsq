// bbdsq/bitboard.h

#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint> // For uint64_t
#include <string>  // For pretty_print_bb
#include <vector>  // Potentially for move lists later
#include <utility> // For std::pair

// --- Core Bitboard Type ---
using U64 = uint64_t;

// --- Board Dimensions ---
constexpr int BOARD_WIDTH = 7;
constexpr int BOARD_HEIGHT = 9;
constexpr int NUM_SQUARES = BOARD_WIDTH * BOARD_HEIGHT; // 63

// --- Square Indices Convention ---
// A1=0, B1=1, ..., G1=6, A2=7, ..., G9=62
// Square index = row * BOARD_WIDTH + col
// (0,0) is A1 (col 0, row 0), (6,8) is G9 (col 6, row 8)

// Helper to get square index from (col, row) - 0-indexed
inline constexpr int get_square_index(int col, int row) {
    // Ensure col and row are within bounds if this function is ever called with dynamic values
    // For now, assuming valid inputs during compile-time or controlled runtime use.
    // if (col < 0 || col >= BOARD_WIDTH || row < 0 || row >= BOARD_HEIGHT) return -1; // Or throw
    return row * BOARD_WIDTH + col;
}

// Helper to get (col, row) from square index - 0-indexed
inline constexpr std::pair<int, int> get_col_row(int sq_idx) {
    // if (sq_idx < 0 || sq_idx >= NUM_SQUARES) return {-1, -1}; // Or throw
    return {sq_idx % BOARD_WIDTH, sq_idx / BOARD_WIDTH};
}

// --- Basic Bitwise Operations ---
inline void set_bit(U64 &bb, int sq) {
    if (sq >= 0 && sq < NUM_SQUARES) bb |= (1ULL << sq);
}

inline void clear_bit(U64 &bb, int sq) {
    if (sq >= 0 && sq < NUM_SQUARES) bb &= ~(1ULL << sq);
}

inline U64 get_bit(U64 bb, int sq) {
    if (sq >= 0 && sq < NUM_SQUARES) return bb & (1ULL << sq);
    return 0ULL; // Return 0 if square is out of bounds
}

inline void toggle_bit(U64 &bb, int sq) {
    if (sq >= 0 && sq < NUM_SQUARES) bb ^= (1ULL << sq);
}

#if defined(__GNUC__) || defined(__clang__)
inline int pop_count(U64 bb) {
    return __builtin_popcountll(bb);
}
inline int lsb_index(U64 bb) { // Renamed from pop_lsb to just get index without modifying
    if (bb == 0) return -1;
    return __builtin_ctzll(bb); // Count Trailing Zeros Long Long
}
inline int msb_index(U64 bb) { // Most significant bit index
    if (bb == 0) return -1;
    return 63 - __builtin_clzll(bb); // Count Leading Zeros Long Long
}
#else
// Fallback for pop_count (Kernighan's algorithm)
inline int pop_count(U64 bb) {
    int count = 0;
    while (bb > 0) {
        bb &= (bb - 1);
        count++;
    }
    return count;
}
// Fallback for lsb_index (less efficient)
inline int lsb_index(U64 bb) {
    if (bb == 0) return -1;
    U64 lsb = bb & -bb; // Isolates the LSB
    int index = 0;
    // This simple loop is okay for a fallback but not ideal for performance.
    // De Bruijn multiplication is a common faster software fallback.
    if (lsb == 0) return -1; // Should not happen if bb was not 0
    while (!((lsb >> index) & 1ULL)) {
        index++;
        if (index >= 64) return -1; // Should not happen if lsb was isolated correctly
    }
    return index;
}
// Fallback for msb_index (less efficient)
inline int msb_index(U64 bb) {
    if (bb == 0) return -1;
    int index = 0;
    U64 temp_bb = bb;
    // Find the highest set bit by repeatedly right-shifting until the value is 1 (or 0 if original was 0).
    // The number of shifts gives the position relative to the LSB.
    // Example: 0b1000 (8), index should be 3.
    // temp_bb=8, index=0
    // temp_bb=4, index=1
    // temp_bb=2, index=2
    // temp_bb=1, index=3 -> loop terminates.
    // This logic is for finding the position of the MSB if it were the only bit.
    // A more robust way for general MSB:
    if (bb == 0) return -1;
    int msb = 0;
    bb >>=1; // if bb was 1, it becomes 0. msb remains 0.
    while(bb > 0) {
        bb >>=1;
        msb++;
    }
    return msb;
    // A common software approach is using a loop or lookup table with shifts.
    // For example:
    // int r = 0;
    // if (bb & 0xFFFFFFFF00000000ULL) { r += 32; bb >>= 32; }
    // if (bb & 0x00000000FFFF0000ULL) { r += 16; bb >>= 16; }
    // if (bb & 0x000000000000FF00ULL) { r +=  8; bb >>=  8; }
    // if (bb & 0x00000000000000F0ULL) { r +=  4; bb >>=  4; }
    // if (bb & 0x000000000000000CULL) { r +=  2; bb >>=  2; }
    // if (bb & 0x0000000000000002ULL) { r +=  1; }
    // return r;
    // Given we expect __builtin_clzll, this fallback is less critical but should be correct.
    // The simple loop `while(temp_bb > 1)` was for finding the index of a power of 2.
    // The current `while(bb > 0)` loop is a more standard way to find MSB index.
}
#endif

// Pop LSB (get LSB index and clear it from the bitboard)
inline int pop_lsb(U64 &bb) {
    if (bb == 0) return -1;
    int lsb_idx = lsb_index(bb);
    // lsb_index should not return -1 if bb != 0
    // but as a safeguard:
    if (lsb_idx != -1) {
        // clear_bit(bb, lsb_idx); // Using our function
        bb &= (bb - 1ULL); // Kernighan's trick to clear LSB, often faster
    }
    return lsb_idx;
}


// --- Function to print a bitboard to a string (for debugging) ---
std::string pretty_print_bb(U64 bb);

// --- Pre-calculated Masks (declarations) ---
// We will define these in bitboard.cpp
extern U64 FILE_A_MASK;
extern U64 FILE_H_MASK; // G-File in DSQ (7th file)
extern U64 RANK_1_MASK; // Bottom rank (Player 2 / Human's side)
extern U64 RANK_9_MASK; // Top rank (Player 1 / Computer's side)

extern U64 LAKE_SQUARES_MASK;

// Player 1 (Computer) is at the TOP (Rank 9 side)
extern U64 P1_DEN_SQUARE_MASK;   // P1's Den at the top (D9)

// Player 2 (Human) is at the BOTTOM (Rank 1 side)
extern U64 P2_DEN_SQUARE_MASK;   // P2's Den at the bottom (D1)

// Trap Squares
extern U64 TRAPS_NEAR_P1_DEN_MASK; // Traps adjacent to P1's Den (top of board: C9, E9, D8)
extern U64 TRAPS_NEAR_P2_DEN_MASK; // Traps adjacent to P2's Den (bottom of board: C1, E1, D2)

extern U64 LAND_SQUARES_MASK;

// Function to initialize all masks
void init_masks();

#endif // BITBOARD_H


