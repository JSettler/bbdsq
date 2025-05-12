// bbdsq/piece.h
#ifndef PIECE_H
#define PIECE_H

#include <string>
#include <vector> 
#include "bitboard.h" // For U64, NUM_SQUARES, get_square_index, pop_lsb etc.

// Forward declaration for the Move struct, which will be fully defined in movegen.h
// This is needed because BoardState::apply_move will take a const Move&.
struct Move; 

enum Player {
    NO_PLAYER = 0, 
    PLAYER_1,      // Computer, starts at top, Grey pieces
    PLAYER_2       // Human, starts at bottom, Brown pieces
};

enum PieceType {
    NO_PIECE_TYPE = 0, 
    RAT,          
    CAT,          
    DOG,          
    WOLF,         
    PANTHER,      
    TIGER,        
    LION,         
    ELEPHANT,     
    NUM_PIECE_TYPES // For array sizing (value will be 9)
};

// Ranks used for capture resolution
const int PIECE_RANKS[NUM_PIECE_TYPES] = {
    0, // NO_PIECE_TYPE
    1, // RAT
    2, // CAT
    3, // DOG
    4, // WOLF
    5, // PANTHER
    6, // TIGER
    7, // LION
    8  // ELEPHANT
};

// Character representation for each piece type
const char PIECE_CHARS[NUM_PIECE_TYPES] = {
    ' ', // NO_PIECE_TYPE
    'R', // RAT
    'C', // CAT
    'D', // DOG
    'W', // WOLF
    'P', // PANTHER
    'T', // TIGER
    'L', // LION
    'E'  // ELEPHANT
};

// Evaluation values for pieces (distinct from ranks used for capture)
const int PIECE_VALUES[NUM_PIECE_TYPES] = {
    0,    // NO_PIECE_TYPE
    6500, // RAT
    2000, // CAT
    3000, // DOG
    4000, // WOLF
    5000, // PANTHER
    7500, // TIGER
    8500, // LION
    9000  // ELEPHANT
};

// Represents a piece on the board, combining its type and player
struct Piece {
    PieceType type;
    Player player;

    Piece(PieceType t = NO_PIECE_TYPE, Player p = NO_PLAYER) : type(t), player(p) {}

    bool operator==(const Piece& other) const {
        return type == other.type && player == other.player;
    }
     bool operator!=(const Piece& other) const {
        return !(*this == other);
    }
};

// Structure to hold the state of all pieces on the board using bitboards
struct BoardState {
    U64 piece_bbs[NUM_PIECE_TYPES][3]; // [PieceType][Player: 0=NO_PLAYER, 1=P1, 2=P2]
    U64 occupancy_bbs[3];             // [Player: 0=AllOccupancy, 1=P1, 2=P2]
    Player side_to_move;
    U64 zobrist_hash;                 // <<<< NEW: Current Zobrist hash of this state

    BoardState(); 

    // Low-level bitboard manipulation, should update hash incrementally
    void add_piece(int sq, PieceType pt, Player p);    
    void remove_piece(int sq, PieceType pt, Player p); 
    
    // High-level function to apply a move, handles piece movement, captures, 
    // side_to_move update, and Zobrist hash update.
    void apply_move(const Move& move); // <<<< NEW/REPLACES old move_piece

    Piece get_piece_at(int sq) const;
    // Updates occupancy_bbs based on piece_bbs. 
    // Does NOT change zobrist_hash by itself (hash changes with piece moves/side change).
    void update_occupancy_boards(); 
    
    // Sets up initial board and calculates initial Zobrist hash
    void setup_initial_board(); 
    // Utility to recalculate the hash from scratch (e.g., for debugging or after complex state changes)
    void force_recalculate_hash(); // <<<< NEW
};

// Utility function (defined in piece.cpp)
// Converts square index (0-62) to algebraic notation (e.g., "a1", "g9")
std::string square_to_algebraic(int sq);


#endif // PIECE_H


