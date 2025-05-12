// bbdsq/evaluation.cpp
#include "evaluation.h" // Also includes piece.h, bitboard.h, and now pst.h
// #include <cmath>     // No longer strictly needed if std::abs was only for lion distance
#include <iostream>  // For potential debug prints

// Helper function to calculate the total material value for a single player
int calculate_material(const BoardState& board_state, Player player) {
    int material = 0;
    if (player == NO_PLAYER) return 0;

    for (int pt_idx = RAT; pt_idx < NUM_PIECE_TYPES; ++pt_idx) {
        PieceType pt = static_cast<PieceType>(pt_idx);
        U64 player_piece_bb = board_state.piece_bbs[pt][player];
        material += pop_count(player_piece_bb) * PIECE_VALUES[pt];
    }
    return material;
}

// Helper function to calculate the total Piece-Square Table score for a player
int calculate_pst_score(const BoardState& board_state, Player player) {
    int total_pst_score = 0;
    if (player == NO_PLAYER) return 0;

    U64 piece_bb;
    // Using a pointer to a const array of NUM_SQUARES ints
    const int (*current_pst_array_ptr)[NUM_SQUARES]; 

    for (int pt_idx = RAT; pt_idx < NUM_PIECE_TYPES; ++pt_idx) {
        PieceType pt = static_cast<PieceType>(pt_idx);
        piece_bb = board_state.piece_bbs[pt][player];
        
        // Select the correct PST based on piece type and player
        if (player == PLAYER_1) {
            switch (pt) {
                case RAT:      current_pst_array_ptr = &PST_RAT_P1;      break;
                case CAT:      current_pst_array_ptr = &PST_CAT_P1;      break;
                case DOG:      current_pst_array_ptr = &PST_DOG_P1;      break;
                case WOLF:     current_pst_array_ptr = &PST_WOLF_P1;     break;
                case PANTHER:  current_pst_array_ptr = &PST_PANTHER_P1;  break;
                case TIGER:    current_pst_array_ptr = &PST_TIGER_P1;    break;
                case LION:     current_pst_array_ptr = &PST_LION_P1;     break;
                case ELEPHANT: current_pst_array_ptr = &PST_ELEPHANT_P1; break;
                default:       continue; 
            }
        } else { // PLAYER_2
            switch (pt) {
                case RAT:      current_pst_array_ptr = &PST_RAT_P2;      break;
                case CAT:      current_pst_array_ptr = &PST_CAT_P2;      break;
                case DOG:      current_pst_array_ptr = &PST_DOG_P2;      break;
                case WOLF:     current_pst_array_ptr = &PST_WOLF_P2;     break;
                case PANTHER:  current_pst_array_ptr = &PST_PANTHER_P2;  break;
                case TIGER:    current_pst_array_ptr = &PST_TIGER_P2;    break;
                case LION:     current_pst_array_ptr = &PST_LION_P2;     break;
                case ELEPHANT: current_pst_array_ptr = &PST_ELEPHANT_P2; break;
                default:       continue; 
            }
        }

        U64 temp_bb = piece_bb;
        while (temp_bb > 0) {
            int sq = pop_lsb(temp_bb);
            if (sq != -1) {
                // Dereference the pointer to array, then access element
                total_pst_score += (*current_pst_array_ptr)[sq];
            }
        }
    }
    return total_pst_score;
}


int evaluate_board(const BoardState& board_state, Player perspective_player) {
    if (perspective_player == NO_PLAYER) {
        return 0; 
    }

    Player opponent_player = (perspective_player == PLAYER_1) ? PLAYER_2 : PLAYER_1;

    // 1. Check for den entry win/loss (highest priority)
    U64 perspective_player_target_den = (perspective_player == PLAYER_1) ? P2_DEN_SQUARE_MASK : P1_DEN_SQUARE_MASK;
    U64 opponent_player_target_den = (perspective_player == PLAYER_1) ? P1_DEN_SQUARE_MASK : P2_DEN_SQUARE_MASK; 

    if ((board_state.occupancy_bbs[perspective_player] & perspective_player_target_den) != 0ULL) {
        return WIN_SCORE;
    }
    if ((board_state.occupancy_bbs[opponent_player] & opponent_player_target_den) != 0ULL) {
        return LOSS_SCORE;
    }

    // 2. Calculate material for both sides
    int perspective_material = calculate_material(board_state, perspective_player);
    int opponent_material = calculate_material(board_state, opponent_player);

    // 3. Check for wipeout win/loss 
    bool perspective_player_has_pieces = (board_state.occupancy_bbs[perspective_player] != 0ULL);
    bool opponent_player_has_pieces = (board_state.occupancy_bbs[opponent_player] != 0ULL);

    if (!opponent_player_has_pieces && perspective_player_has_pieces) return WIN_SCORE;
    if (!perspective_player_has_pieces && opponent_player_has_pieces) return LOSS_SCORE;
    if (!perspective_player_has_pieces && !opponent_player_has_pieces) return DRAW_SCORE;


    // 4. Calculate Piece-Square Table scores
    int perspective_pst_score = calculate_pst_score(board_state, perspective_player);
    int opponent_pst_score = calculate_pst_score(board_state, opponent_player);

    // 5. Final score: material difference + net PST advantage
    int score = (perspective_material - opponent_material) + 
                (perspective_pst_score - opponent_pst_score);
    
    // Debug print for evaluation components (can be enabled if needed)
    // if (perspective_player == PLAYER_1) { 
    //     std::cout << "Eval for P" << perspective_player 
    //               << ": MatDiff(" << perspective_material - opponent_material 
    //               << ") MyPST(" << perspective_pst_score      // Changed from MyLionBonus
    //               << ") OppPST(" << opponent_pst_score     // Changed from OppLionBonus
    //               << ") NetPST(" << perspective_pst_score - opponent_pst_score
    //               << ") Total(" << score << ")" << std::endl;
    // }
              
    return score;
}


