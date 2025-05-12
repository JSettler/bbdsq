// bbdsq/piece.cpp
#include "piece.h"
#include "zobrist.h" // For Zobrist keys and hash calculation/update functions
#include "movegen.h" // For the Move struct definition (used by apply_move)
#include <stdexcept> 
#include <iostream>  

// --- BoardState Member Function Definitions ---

BoardState::BoardState() : 
    side_to_move(PLAYER_1), 
    zobrist_hash(0ULL) // Initial hash is 0, will be properly set by setup or load
{
    // Initialize all bitboards to empty
    for (int pt = 0; pt < NUM_PIECE_TYPES; ++pt) {
        piece_bbs[pt][NO_PLAYER] = 0ULL; // Should always be 0
        piece_bbs[pt][PLAYER_1] = 0ULL;
        piece_bbs[pt][PLAYER_2] = 0ULL;
    }
    // Initialize occupancy bitboards to empty
    for (int p = 0; p < 3; ++p) { // Iterate NO_PLAYER, PLAYER_1, PLAYER_2
        occupancy_bbs[p] = 0ULL;
    }
    // Note: A full hash calculation will be done by setup_initial_board()
    // or when a game is loaded.
}

void BoardState::force_recalculate_hash() {
    // This calculates the hash from scratch based on current piece positions and side to move
    this->zobrist_hash = Zobrist::calculate_initial_hash(*this);
}

// Low-level function, primarily for setup_initial_board.
// Assumes square is currently empty by this player for this piece type.
void BoardState::add_piece(int sq, PieceType pt, Player p) {
    if (pt == NO_PIECE_TYPE || p == NO_PLAYER || sq < 0 || sq >= NUM_SQUARES) {
        std::cerr << "Warning (add_piece): Invalid parameters. sq=" << sq << " pt=" << pt << " p=" << p << std::endl;
        return;
    }
    // Check if piece already exists to prevent double XORing if called incorrectly
    if (!get_bit(this->piece_bbs[pt][p], sq)) { 
        set_bit(this->piece_bbs[pt][p], sq);
        Zobrist::xor_piece_at_sq(this->zobrist_hash, pt, p, sq); // Update hash
    } else {
        // std::cerr << "Warning (add_piece): Piece already exists at " << square_to_algebraic(sq) << std::endl;
    }
    // Note: update_occupancy_boards() should be called after all pieces are set up.
}

// Low-level function.
// Assumes piece actually exists at the square.
void BoardState::remove_piece(int sq, PieceType pt, Player p) {
    if (pt == NO_PIECE_TYPE || p == NO_PLAYER || sq < 0 || sq >= NUM_SQUARES) {
        std::cerr << "Warning (remove_piece): Invalid parameters. sq=" << sq << " pt=" << pt << " p=" << p << std::endl;
        return;
    }
    // Check if piece actually exists to prevent XORing if called incorrectly
    if (get_bit(this->piece_bbs[pt][p], sq)) { 
        clear_bit(this->piece_bbs[pt][p], sq);
        Zobrist::xor_piece_at_sq(this->zobrist_hash, pt, p, sq); // Update hash
    } else {
        // std::cerr << "Warning (remove_piece): No such piece to remove at " << square_to_algebraic(sq) << std::endl;
    }
    // Note: update_occupancy_boards() should be called after all piece modifications.
}

// Applies a move, updates piece positions, side to move, and Zobrist hash.
void BoardState::apply_move(const Move& move) {
    if (move.from_sq == -1 || move.to_sq == -1 || move.piece_moved == NO_PIECE_TYPE) {
        std::cerr << "Error (apply_move): Attempting to apply an invalid or null move: " << move.to_string() << std::endl;
        return;
    }

    Player player_making_move = this->side_to_move; 

    // Sanity check: piece being moved should exist at from_sq and belong to player_making_move
    if (!get_bit(this->piece_bbs[move.piece_moved][player_making_move], move.from_sq)) {
        std::cerr << "Error (apply_move): Piece " << PIECE_CHARS[move.piece_moved] 
                  << " for player " << player_making_move 
                  << " not found at source square " << square_to_algebraic(move.from_sq) << std::endl;
        this->force_recalculate_hash(); // Hash is suspect, recalculate fully
        return; 
    }

    // 1. Update hash: XOR out the piece from its original square
    Zobrist::xor_piece_at_sq(this->zobrist_hash, move.piece_moved, player_making_move, move.from_sq);
    // Update bitboard: Clear piece from original square
    clear_bit(this->piece_bbs[move.piece_moved][player_making_move], move.from_sq);

    // 2. If it was a capture, update hash and bitboard for the captured piece
    if (move.piece_captured != NO_PIECE_TYPE) {
        Player opponent = (player_making_move == PLAYER_1) ? PLAYER_2 : PLAYER_1;
        // Sanity check: ensure the captured piece is indeed on the target square and belongs to opponent
        if (get_bit(this->piece_bbs[move.piece_captured][opponent], move.to_sq)) {
            Zobrist::xor_piece_at_sq(this->zobrist_hash, move.piece_captured, opponent, move.to_sq);
            clear_bit(this->piece_bbs[move.piece_captured][opponent], move.to_sq);
        } else {
            std::cerr << "Warning (apply_move): Move object indicated capture of "
                      << PIECE_CHARS[move.piece_captured] << " (Player " << opponent << ") at " 
                      << square_to_algebraic(move.to_sq) << ", but piece not found or wrong owner." << std::endl;
            // If this happens, the hash might become inconsistent if we don't correct it.
            // A full recalculation might be the safest if this state is reached,
            // or the Move object generation needs to be more robust.
            // For now, we proceed, but this indicates a potential earlier issue.
        }
    }

    // 3. Update hash: XOR in the piece at its new square
    Zobrist::xor_piece_at_sq(this->zobrist_hash, move.piece_moved, player_making_move, move.to_sq);
    // Update bitboard: Set piece at new square
    set_bit(this->piece_bbs[move.piece_moved][player_making_move], move.to_sq);

    // 4. Update hash: XOR out old side_to_move key, XOR in new side_to_move key
    if (this->side_to_move != NO_PLAYER) { 
        Zobrist::xor_side_to_move(this->zobrist_hash, this->side_to_move);
    }
    this->side_to_move = (player_making_move == PLAYER_1) ? PLAYER_2 : PLAYER_1;
    if (this->side_to_move != NO_PLAYER) { 
        Zobrist::xor_side_to_move(this->zobrist_hash, this->side_to_move);
    }
    
    this->update_occupancy_boards(); 
}


Piece BoardState::get_piece_at(int sq) const {
    if (sq < 0 || sq >= NUM_SQUARES) {
        return Piece(NO_PIECE_TYPE, NO_PLAYER); 
    }
    for (int p_val = PLAYER_1; p_val <= PLAYER_2; ++p_val) { 
        Player player = static_cast<Player>(p_val);
        for (int pt_val = RAT; pt_val < NUM_PIECE_TYPES; ++pt_val) { 
            PieceType pt = static_cast<PieceType>(pt_val);
            if (get_bit(piece_bbs[pt][player], sq)) {
                return Piece(pt, player);
            }
        }
    }
    return Piece(NO_PIECE_TYPE, NO_PLAYER);
}
    
void BoardState::update_occupancy_boards() {
    occupancy_bbs[PLAYER_1] = 0ULL;
    occupancy_bbs[PLAYER_2] = 0ULL;
    for (int pt_idx = RAT; pt_idx < NUM_PIECE_TYPES; ++pt_idx) { 
        occupancy_bbs[PLAYER_1] |= piece_bbs[pt_idx][PLAYER_1];
        occupancy_bbs[PLAYER_2] |= piece_bbs[pt_idx][PLAYER_2];
    }
    occupancy_bbs[NO_PLAYER] = occupancy_bbs[PLAYER_1] | occupancy_bbs[PLAYER_2];
}

void BoardState::setup_initial_board() {
    for (int pt_idx = 0; pt_idx < NUM_PIECE_TYPES; ++pt_idx) {
        piece_bbs[pt_idx][PLAYER_1] = 0ULL;
        piece_bbs[pt_idx][PLAYER_2] = 0ULL;
        piece_bbs[pt_idx][NO_PLAYER] = 0ULL;
    }
    zobrist_hash = 0ULL; // Reset hash before adding pieces

    // Player 1 (Computer, Top)
    add_piece(get_square_index(0, 8), LION, PLAYER_1);     
    add_piece(get_square_index(6, 8), TIGER, PLAYER_1);    
    add_piece(get_square_index(1, 7), DOG, PLAYER_1);      
    add_piece(get_square_index(5, 7), CAT, PLAYER_1);      
    add_piece(get_square_index(4, 6), WOLF, PLAYER_1);     
    add_piece(get_square_index(2, 6), PANTHER, PLAYER_1);  
    add_piece(get_square_index(0, 6), RAT, PLAYER_1);      
    add_piece(get_square_index(6, 6), ELEPHANT, PLAYER_1); 

    // Player 2 (Human, Bottom)
    add_piece(get_square_index(6, 0), LION, PLAYER_2);     
    add_piece(get_square_index(0, 0), TIGER, PLAYER_2);    
    add_piece(get_square_index(5, 1), DOG, PLAYER_2);      
    add_piece(get_square_index(1, 1), CAT, PLAYER_2);      
    add_piece(get_square_index(2, 2), WOLF, PLAYER_2);     
    add_piece(get_square_index(4, 2), PANTHER, PLAYER_2);  
    add_piece(get_square_index(6, 2), RAT, PLAYER_2);      
    add_piece(get_square_index(0, 2), ELEPHANT, PLAYER_2); 
    
    side_to_move = PLAYER_1;
    // Add side to move key to the hash *after* all pieces are placed
    if (side_to_move != NO_PLAYER) { 
        zobrist_hash ^= Zobrist::side_to_move_key[side_to_move];
    }
    
    update_occupancy_boards(); 
    // At this point, zobrist_hash should be correct due to incremental updates.
    // For verification during development, you could compare with a full recalc:
    // U64 full_recalc_hash = Zobrist::calculate_initial_hash(*this);
    // if (zobrist_hash != full_recalc_hash) {
    //     std::cerr << "CRITICAL: Zobrist hash mismatch after setup_initial_board!" << std::endl;
    //     std::cerr << "Incremental: 0x" << std::hex << zobrist_hash << std::endl;
    //     std::cerr << "Full Recalc: 0x" << std::hex << full_recalc_hash << std::dec << std::endl;
    // }
}


std::string square_to_algebraic(int sq) {
    if (sq < 0 || sq >= NUM_SQUARES) {
        return "??"; 
    }
    std::pair<int, int> cr = get_col_row(sq); 
    char file_char = 'a' + cr.first;
    char rank_char = '1' + cr.second; 
    return std::string(1, file_char) + rank_char;
}


