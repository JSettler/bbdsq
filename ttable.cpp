// bbdsq/ttable.cpp
#include "ttable.h"
#include <vector>
#include <iostream> // For messages
#include <cstring>  // For std::memset (though not used if default constructing entries)
#include <algorithm>// For std::fill_n if used for clearing

namespace TranspositionTable {

    // --- Transposition Table Data ---
    static std::vector<TTEntry> tt_table; 
    static size_t tt_num_entries = 0;     
    static bool tt_initialized = false;

    // --- Transposition Table Management ---

    void initialize_tt(size_t size_mb) {
        if (size_mb == 0) {
            tt_num_entries = 0;
            tt_table.clear(); // Ensure vector is empty
            tt_table.shrink_to_fit(); // Release memory
            tt_initialized = false;
            std::cout << "Transposition Table disabled (size 0 MB)." << std::endl;
            return;
        }

        size_t table_size_bytes = size_mb * 1024 * 1024;
        size_t calculated_num_entries = table_size_bytes / sizeof(TTEntry);

        if (calculated_num_entries == 0 && size_mb > 0) { 
            calculated_num_entries = 1; 
            std::cout << "Warning: TT size in MB is too small for even one entry. Setting to 1 entry." << std::endl;
        }
        
        // For efficient modulo indexing, powers of 2 are good, but not strictly necessary with %.
        // If we wanted to use bitwise AND for indexing (hash & (num_entries - 1)),
        // num_entries would need to be a power of 2.
        // For now, tt_num_entries can be any value.
        tt_num_entries = calculated_num_entries;
        
        try {
            // If re-initializing, clear old table first to free memory before resize
            if (tt_initialized) {
                tt_table.clear();
                tt_table.shrink_to_fit();
            }
            tt_table.resize(tt_num_entries); 
            clear_tt(); 
            tt_initialized = true;
            std::cout << "Transposition Table initialized. Target Size: " << size_mb << " MB, Actual Entries: " << tt_num_entries 
                      << " (Entry size: " << sizeof(TTEntry) << " bytes)" << std::endl;
        } catch (const std::bad_alloc& e) {
            std::cerr << "Error: Failed to allocate memory for Transposition Table (" << size_mb << " MB). "
                      << e.what() << std::endl;
            tt_num_entries = 0;
            tt_table.clear();
            tt_initialized = false;
        } catch (const std::exception& e) { // Catch other potential exceptions from resize/vector ops
            std::cerr << "Error during TT initialization: " << e.what() << std::endl;
            tt_num_entries = 0;
            tt_table.clear();
            tt_initialized = false;
        }
    }

    void clear_tt() {
        if (!tt_initialized || tt_num_entries == 0 || tt_table.empty()) return;
        // Using default constructor for each TTEntry is safer than memset
        // as it correctly initializes flags and other members.
        for (size_t i = 0; i < tt_num_entries; ++i) {
            tt_table[i] = TTEntry(); 
        }
        // Alternative using std::fill_n if TTEntry() is cheap and well-defined for this
        // std::fill_n(tt_table.begin(), tt_num_entries, TTEntry());
        // std::cout << "Transposition Table cleared (" << tt_num_entries << " entries reset)." << std::endl;
    }

    TTEntry* probe_tt(U64 zobrist_hash) {
        if (!tt_initialized || tt_num_entries == 0) {
            return nullptr;
        }

        size_t index = zobrist_hash % tt_num_entries; 
        TTEntry* entry = &tt_table[index];

        if (entry->flag != EntryFlag::NO_ENTRY && entry->zobrist_key_check == zobrist_hash) {
            return entry;
        }
        
        return nullptr; 
    }

    void store_tt_entry(U64 zobrist_hash, int score, int depth, EntryFlag flag, const Move& best_move) {
        if (!tt_initialized || tt_num_entries == 0) {
            return;
        }

        size_t index = zobrist_hash % tt_num_entries;
        TTEntry* entry_to_replace = &tt_table[index];

        // Replacement strategy:
        // Overwrite if:
        // 1. Slot is empty.
        // 2. Existing entry is for a different position (hash mismatch).
        // 3. New entry is from a deeper or equally deep search.
        //    (If same depth, new entry might be more accurate, e.g. EXACT vs BOUND)
        // A common strategy is "depth-preferred replacement".
        if (entry_to_replace->flag == EntryFlag::NO_ENTRY || 
            entry_to_replace->zobrist_key_check != zobrist_hash || 
            depth >= entry_to_replace->depth) // Replace if new entry is deeper or same depth
                                              // (could add tie-breaking like preferring EXACT scores)
        {
            entry_to_replace->zobrist_key_check = zobrist_hash;
            entry_to_replace->score = score;
            entry_to_replace->depth = static_cast<short>(depth);
            entry_to_replace->flag = flag;
            entry_to_replace->best_move = best_move; 
        }
    }
    
    void cleanup_tt() {
        tt_table.clear();
        tt_table.shrink_to_fit(); 
        tt_num_entries = 0;
        tt_initialized = false;
        // std::cout << "Transposition Table cleaned up." << std::endl;
    }

    size_t get_tt_num_entries() {
        return tt_num_entries;
    }

    TTStats get_tt_stats() {
        TTStats stats;
        stats.total_entries = tt_num_entries;
        if (!tt_initialized || tt_num_entries == 0) {
            return stats; 
        }

        for (size_t i = 0; i < tt_num_entries; ++i) {
            if (tt_table[i].flag != EntryFlag::NO_ENTRY) {
                stats.used_entries++;
            }
        }

        if (stats.total_entries > 0) {
            stats.utilization_percent = (static_cast<double>(stats.used_entries) / stats.total_entries) * 100.0;
        }
        return stats;
    }

} // namespace TranspositionTable


