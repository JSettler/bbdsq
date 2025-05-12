// bbdsq/movegen.cpp
#include "movegen.h" // Includes piece.h (for BoardState, PieceType, Player, PIECE_RANKS, etc.)
                     // and bitboard.h (for U64, masks, etc.)
#include <vector>
#include <iostream> 

// Note: Global masks are extern U64 declared in bitboard.h and defined in bitboard.cpp.
// They must be initialized (via init_masks()) before these functions are reliably used.

U64 generate_orthogonal_step_moves(
    int piece_square_idx, 
    U64 friendly_occupancy, 
    U64 land_squares_mask,  
    U64 own_den_mask        
) {
    U64 moves_bb = 0ULL;
    if (piece_square_idx < 0 || piece_square_idx >= NUM_SQUARES) return 0ULL;
    U64 piece_bb = (1ULL << piece_square_idx); 

    U64 north_target_sq_bb = 0ULL;
    U64 south_target_sq_bb = 0ULL;
    U64 east_target_sq_bb = 0ULL;
    U64 west_target_sq_bb = 0ULL;

    if (!(piece_bb & RANK_9_MASK)) north_target_sq_bb = (piece_bb << BOARD_WIDTH); 
    if (!(piece_bb & RANK_1_MASK)) south_target_sq_bb = (piece_bb >> BOARD_WIDTH);
    if (!(piece_bb & FILE_H_MASK)) east_target_sq_bb = (piece_bb << 1);
    if (!(piece_bb & FILE_A_MASK)) west_target_sq_bb = (piece_bb >> 1);
    
    U64 potential_moves = north_target_sq_bb | south_target_sq_bb | east_target_sq_bb | west_target_sq_bb;
    moves_bb = potential_moves & land_squares_mask & ~friendly_occupancy & ~own_den_mask;
    return moves_bb;
}

U64 generate_rat_moves(
    int piece_square_idx,
    U64 friendly_occupancy, 
    U64 own_den_mask,
    U64 lake_squares_mask 
) {
    (void)lake_squares_mask; 

    U64 moves_bb = 0ULL;
    if (piece_square_idx < 0 || piece_square_idx >= NUM_SQUARES) return 0ULL;
    U64 piece_bb = (1ULL << piece_square_idx);

    U64 north_target_sq_bb = 0ULL;
    U64 south_target_sq_bb = 0ULL;
    U64 east_target_sq_bb = 0ULL;
    U64 west_target_sq_bb = 0ULL;

    if (!(piece_bb & RANK_9_MASK)) north_target_sq_bb = (piece_bb << BOARD_WIDTH);
    if (!(piece_bb & RANK_1_MASK)) south_target_sq_bb = (piece_bb >> BOARD_WIDTH);
    if (!(piece_bb & FILE_H_MASK)) east_target_sq_bb = (piece_bb << 1);
    if (!(piece_bb & FILE_A_MASK)) west_target_sq_bb = (piece_bb >> 1);
    
    U64 potential_moves = north_target_sq_bb | south_target_sq_bb | east_target_sq_bb | west_target_sq_bb;
    moves_bb = potential_moves & ~friendly_occupancy & ~own_den_mask;
    return moves_bb;
}

U64 generate_lion_tiger_jump_moves(
    int piece_square_idx,
    Player moving_player, 
    U64 friendly_occupancy, 
    U64 all_rat_occupancy, 
    U64 own_den_mask,
    U64 lake_squares_mask) {

    (void)moving_player; 

    U64 jump_moves_bb = 0ULL;
    if (piece_square_idx < 0 || piece_square_idx >= NUM_SQUARES) return 0ULL;

    std::pair<int, int> cr = get_col_row(piece_square_idx);
    int col = cr.first;
    int row = cr.second;

    if (get_bit(lake_squares_mask, piece_square_idx)) {
        return 0ULL; 
    }

    // Vertical Jumps:
    if (row == 2) { 
        int target_row = 6; 
        if (col == 1 || col == 2 || col == 4 || col == 5) { 
            U64 path_mask = 0ULL;
            set_bit(path_mask, get_square_index(col, 3)); 
            set_bit(path_mask, get_square_index(col, 4)); 
            set_bit(path_mask, get_square_index(col, 5)); 
            
            if ((path_mask & lake_squares_mask) == path_mask && 
                (path_mask & all_rat_occupancy) == 0ULL) {    
                int target_sq = get_square_index(col, target_row);
                U64 target_bb = (1ULL << target_sq);
                if (get_bit(LAND_SQUARES_MASK, target_sq) && 
                    !(target_bb & friendly_occupancy) &&  
                    !(target_bb & own_den_mask)) {
                    set_bit(jump_moves_bb, target_sq);
                }
            }
        }
    }
    else if (row == 6) {
        int target_row = 2; 
        if (col == 1 || col == 2 || col == 4 || col == 5) { 
            U64 path_mask = 0ULL;
            set_bit(path_mask, get_square_index(col, 5)); 
            set_bit(path_mask, get_square_index(col, 4)); 
            set_bit(path_mask, get_square_index(col, 3)); 

            if ((path_mask & lake_squares_mask) == path_mask &&
                (path_mask & all_rat_occupancy) == 0ULL) {
                int target_sq = get_square_index(col, target_row);
                U64 target_bb = (1ULL << target_sq);
                if (get_bit(LAND_SQUARES_MASK, target_sq) &&
                    !(target_bb & friendly_occupancy) && 
                    !(target_bb & own_den_mask)) {
                    set_bit(jump_moves_bb, target_sq);
                }
            }
        }
    }

    // Horizontal Jumps:
    if (col == 0) {
        int target_col = 3;
        if (row == 3 || row == 4 || row == 5) { 
            U64 path_mask = 0ULL;
            set_bit(path_mask, get_square_index(1, row)); 
            set_bit(path_mask, get_square_index(2, row)); 
            if ((path_mask & lake_squares_mask) == path_mask &&
                (path_mask & all_rat_occupancy) == 0ULL) {
                int target_sq = get_square_index(target_col, row);
                U64 target_bb = (1ULL << target_sq);
                if (get_bit(LAND_SQUARES_MASK, target_sq) &&
                    !(target_bb & friendly_occupancy) && 
                    !(target_bb & own_den_mask)) {
                    set_bit(jump_moves_bb, target_sq);
                }
            }
        }
    }
    else if (col == 6) {
        int target_col = 3;
        if (row == 3 || row == 4 || row == 5) { 
            U64 path_mask = 0ULL;
            set_bit(path_mask, get_square_index(5, row)); 
            set_bit(path_mask, get_square_index(4, row)); 
            if ((path_mask & lake_squares_mask) == path_mask &&
                (path_mask & all_rat_occupancy) == 0ULL) {
                int target_sq = get_square_index(target_col, row);
                U64 target_bb = (1ULL << target_sq);
                if (get_bit(LAND_SQUARES_MASK, target_sq) &&
                    !(target_bb & friendly_occupancy) && 
                    !(target_bb & own_den_mask)) {
                    set_bit(jump_moves_bb, target_sq);
                }
            }
        }
    }
    else if (col == 3) {
        if (row == 3 || row == 4 || row == 5) { 
            int target_col_west = 0;
            U64 path_mask_west = 0ULL;
            set_bit(path_mask_west, get_square_index(2, row)); 
            set_bit(path_mask_west, get_square_index(1, row)); 
            if ((path_mask_west & lake_squares_mask) == path_mask_west &&
                (path_mask_west & all_rat_occupancy) == 0ULL) {
                int target_sq_w = get_square_index(target_col_west, row);
                U64 target_bb_w = (1ULL << target_sq_w);
                if (get_bit(LAND_SQUARES_MASK, target_sq_w) &&
                    !(target_bb_w & friendly_occupancy) && 
                    !(target_bb_w & own_den_mask)) {
                    set_bit(jump_moves_bb, target_sq_w);
                }
            }

            int target_col_east = 6;
            U64 path_mask_east = 0ULL;
            set_bit(path_mask_east, get_square_index(4, row)); 
            set_bit(path_mask_east, get_square_index(5, row)); 
            if ((path_mask_east & lake_squares_mask) == path_mask_east &&
                (path_mask_east & all_rat_occupancy) == 0ULL) {
                int target_sq_e = get_square_index(target_col_east, row);
                U64 target_bb_e = (1ULL << target_sq_e);
                 if (get_bit(LAND_SQUARES_MASK, target_sq_e) &&
                    !(target_bb_e & friendly_occupancy) && 
                    !(target_bb_e & own_den_mask)) {
                    set_bit(jump_moves_bb, target_sq_e);
                }
            }
        }
    }
    return jump_moves_bb;
}


std::vector<Move> generate_all_legal_moves(
    const BoardState& board_state, 
    Player player_to_move,
    const std::vector<BoardState>& game_history_for_rep_check // Pass the actual game history
) {
    std::vector<Move> pseudo_legal_moves;
    if (player_to_move == NO_PLAYER) {
        return pseudo_legal_moves; // Return empty list
    }

    U64 friendly_occupancy = board_state.occupancy_bbs[player_to_move];
    U64 own_den_mask = (player_to_move == PLAYER_1) ? P1_DEN_SQUARE_MASK : P2_DEN_SQUARE_MASK;

    for (int pt_idx = RAT; pt_idx < NUM_PIECE_TYPES; ++pt_idx) {
        PieceType piece_type_moving = static_cast<PieceType>(pt_idx);
        U64 piece_locations_bb = board_state.piece_bbs[piece_type_moving][player_to_move];
        U64 temp_piece_locations_bb = piece_locations_bb;

        while (temp_piece_locations_bb > 0) {
            int from_sq = pop_lsb(temp_piece_locations_bb);
            if (from_sq == -1) break; 

            U64 possible_landing_squares_bb = 0ULL;
            // Piece attacker_piece_obj = Piece(piece_type_moving, player_to_move); // Not needed here

            if (piece_type_moving == CAT || piece_type_moving == DOG || piece_type_moving == WOLF || 
                piece_type_moving == PANTHER || piece_type_moving == ELEPHANT) {
                possible_landing_squares_bb = generate_orthogonal_step_moves(
                    from_sq, friendly_occupancy, LAND_SQUARES_MASK, own_den_mask);
            } else if (piece_type_moving == RAT) {
                possible_landing_squares_bb = generate_rat_moves(
                    from_sq, friendly_occupancy, own_den_mask, LAKE_SQUARES_MASK);
            } else if (piece_type_moving == LION || piece_type_moving == TIGER) {
                possible_landing_squares_bb = generate_orthogonal_step_moves(
                    from_sq, friendly_occupancy, LAND_SQUARES_MASK, own_den_mask);
                U64 all_rats_bb = board_state.piece_bbs[RAT][PLAYER_1] | board_state.piece_bbs[RAT][PLAYER_2];
                possible_landing_squares_bb |= generate_lion_tiger_jump_moves(
                    from_sq, player_to_move, friendly_occupancy, all_rats_bb, own_den_mask, LAKE_SQUARES_MASK);
            }

            U64 temp_targets_bb = possible_landing_squares_bb;
            while (temp_targets_bb > 0) {
                int to_sq = pop_lsb(temp_targets_bb);
                if (to_sq == -1) break;

                Piece defender_on_target = board_state.get_piece_at(to_sq);
                PieceType captured_piece_type = NO_PIECE_TYPE;
                bool is_capture_attempt = (defender_on_target.player != NO_PLAYER && defender_on_target.player != player_to_move);
                bool move_is_board_legal = false;

                if (!is_capture_attempt) { // Moving to an empty square
                    move_is_board_legal = true;
                } else { // Attempting to capture an enemy piece
                    bool can_capture_this_enemy = false;
                    int attacker_rank = PIECE_RANKS[piece_type_moving]; // Attacker is piece_type_moving
                    int defender_rank = PIECE_RANKS[defender_on_target.type];

                    bool defender_is_weakened_by_trap = false;
                    U64 traps_that_weaken_defender = (defender_on_target.player == PLAYER_1) ? TRAPS_NEAR_P2_DEN_MASK : TRAPS_NEAR_P1_DEN_MASK;
                    if (get_bit(traps_that_weaken_defender, to_sq)) {
                        defender_is_weakened_by_trap = true;
                    }
                    
                    if (defender_is_weakened_by_trap) {
                        can_capture_this_enemy = true; 
                    } else {
                        if (attacker_rank >= defender_rank) can_capture_this_enemy = true;
                        if (piece_type_moving == RAT && defender_on_target.type == ELEPHANT) can_capture_this_enemy = true;
                        if (piece_type_moving == ELEPHANT && defender_on_target.type == RAT) can_capture_this_enemy = false; 
                    }
                    
                    if (piece_type_moving == RAT && can_capture_this_enemy) {
                        bool attacker_on_water = get_bit(LAKE_SQUARES_MASK, from_sq);
                        bool defender_on_water = get_bit(LAKE_SQUARES_MASK, to_sq);
                        if (attacker_on_water != defender_on_water) {
                            can_capture_this_enemy = false; 
                        }
                    }
                    if (can_capture_this_enemy) {
                        move_is_board_legal = true;
                        captured_piece_type = defender_on_target.type;
                    }
                } 
                if (move_is_board_legal) {
                    pseudo_legal_moves.push_back(Move(from_sq, to_sq, piece_type_moving, captured_piece_type));
                }
            } 
        } 
    } 

    // Now, filter pseudo_legal_moves for 3-fold repetition
    std::vector<Move> truly_legal_moves;
    truly_legal_moves.reserve(pseudo_legal_moves.size());

    for (const Move& move : pseudo_legal_moves) {
        BoardState next_state_after_move = board_state; // Create a copy to simulate the move
        next_state_after_move.apply_move(move);      // This updates next_state_after_move.zobrist_hash

        int repetition_count = 0;
        // Count occurrences of the resulting hash in the actual game history
        // The game_history_for_rep_check should contain states *before* the current one.
        for (const BoardState& historical_state : game_history_for_rep_check) {
            if (historical_state.zobrist_hash == next_state_after_move.zobrist_hash &&
                historical_state.side_to_move == next_state_after_move.side_to_move) { // Ensure side to move also matches for true repetition
                repetition_count++;
            }
        }
        // According to Arimaa rules (and common chess), making a move that results in the
        // position appearing for the third time is illegal.
        // So, if the hash has appeared twice *before* this move, this move is illegal.
        if (repetition_count < 2) { 
            truly_legal_moves.push_back(move);
        } else {
            // std::cout << "Repetition Detected: Move " << move.to_string() 
            //           << " would lead to 3rd occurrence of hash 0x" << std::hex 
            //           << next_state_after_move.zobrist_hash << std::dec << std::endl;
        }
    }
    return truly_legal_moves;
}


