// bbdsq/evaluation.h
#ifndef EVALUATION_H
#define EVALUATION_H

#include "piece.h"       // For BoardState, PIECE_VALUES, Player, PieceType, etc.
#include "bitboard.h"    // For den masks, pop_count, NUM_SQUARES, etc.
#include "pst.h"         // For the Piece-Square Table declarations

// Define scores for decisive game outcomes
const int WIN_SCORE = 1000000000;  // 1 Billion
const int LOSS_SCORE = -1000000000; // -1 Billion
const int DRAW_SCORE = 0; 

// Calculates the total Piece-Square Table score for a given player.
int calculate_pst_score(const BoardState& board_state, Player player);

// Evaluates the board from the perspective of 'perspective_player'.
// A positive score is good for perspective_player, negative is bad.
// This function considers:
// 1. Den entry (immediate win/loss).
// 2. Wipeout of all opponent's pieces (win).
// 3. Wipeout of all perspective_player's pieces (loss).
// 4. Material difference.
// 5. Piece-Square Table scores.
int evaluate_board(const BoardState& board_state, Player perspective_player);

#endif // EVALUATION_H


