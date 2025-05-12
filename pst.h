// bbdsq/pst.h
#ifndef PST_H
#define PST_H

#include "bitboard.h" // For NUM_SQUARES
#include "piece.h"    // For PieceType, Player enums

// Piece-Square Tables
// Values represent bonuses/penalties for a piece being on a square.
// Indexed by square index (0-62).

// Player 1 (AI, Grey, starts at top, aims for bottom den)
extern const int PST_RAT_P1[NUM_SQUARES];
extern const int PST_CAT_P1[NUM_SQUARES];
extern const int PST_DOG_P1[NUM_SQUARES];
extern const int PST_WOLF_P1[NUM_SQUARES];
extern const int PST_PANTHER_P1[NUM_SQUARES];
extern const int PST_TIGER_P1[NUM_SQUARES];
extern const int PST_LION_P1[NUM_SQUARES];
extern const int PST_ELEPHANT_P1[NUM_SQUARES];

// Player 2 (Human, Brown, starts at bottom, aims for top den)
extern const int PST_RAT_P2[NUM_SQUARES];
extern const int PST_CAT_P2[NUM_SQUARES];
extern const int PST_DOG_P2[NUM_SQUARES];
extern const int PST_WOLF_P2[NUM_SQUARES];
extern const int PST_PANTHER_P2[NUM_SQUARES];
extern const int PST_TIGER_P2[NUM_SQUARES];
extern const int PST_LION_P2[NUM_SQUARES];
extern const int PST_ELEPHANT_P2[NUM_SQUARES];

// Helper function to initialize all PSTs (if needed, or they can be const initialized)
// For now, they will be const initialized directly in pst.cpp
// void init_psts(); 

#endif // PST_H