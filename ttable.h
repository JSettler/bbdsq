// bbdsq/ttable.h
#ifndef TTABLE_H
#define TTABLE_H

#include "bitboard.h" // For U64
#include "movegen.h"  // For Move struct 
                     // (Ensure Move struct is defined in movegen.h or its own move.h included by movegen.h)

namespace TranspositionTable {

    // Enum for the type of score stored in a TT entry
    enum class EntryFlag : unsigned char { 
        NO_ENTRY,    
        EXACT_SCORE, 
        LOWER_BOUND, 
        UPPER_BOUND  
    };

    struct TTEntry {
        U64 zobrist_key_check; 
        Move best_move;        
        int score;             
        short depth;           
        EntryFlag flag;        
        // unsigned char age; // Optional for future replacement strategies

        TTEntry() : 
            zobrist_key_check(0ULL), 
            score(0),                // Or some sentinel value like -WIN_SCORE - 1 if 0 is a valid score
            depth(-1),               // -1 indicates invalid/unused depth
            flag(EntryFlag::NO_ENTRY) 
        {
            // best_move is default constructed (from_sq/to_sq = -1)
        }
    };

    // Struct to hold TT statistics
    struct TTStats {
        size_t used_entries;
        size_t total_entries;
        double utilization_percent;

        TTStats() : used_entries(0), total_entries(0), utilization_percent(0.0) {}
    };


    // --- Transposition Table Management ---

    // Initializes the transposition table.
    // size_mb: desired table size in Megabytes.
    void initialize_tt(size_t size_mb);

    // Clears all entries in the transposition table.
    void clear_tt();

    // Probes the TT for a given Zobrist hash.
    // Returns a pointer to the TTEntry if found and valid, otherwise nullptr.
    TTEntry* probe_tt(U64 zobrist_hash);

    // Stores an entry into the transposition table.
    void store_tt_entry(U64 zobrist_hash, int score, int depth, EntryFlag flag, const Move& best_move);
    
    // Call to clean up TT if dynamically allocated.
    void cleanup_tt();

    // --- Helpers ---
    // Get current number of entries in TT (total capacity).
    size_t get_tt_num_entries();

    // Gets current TT utilization statistics.
    TTStats get_tt_stats();


} // namespace TranspositionTable

#endif // TTABLE_H


