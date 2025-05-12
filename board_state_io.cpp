// bbdsq/board_state_io.cpp
#include "board_state_io.h"
#include <fstream>   // For std::ofstream, std::ifstream
#include <sstream>   // For std::stringstream
#include <iomanip>   // For std::hex, std::setw, std::setfill
#include <iostream>  // For error messages and success messages
#include <stdexcept> // For std::stoi, std::stoull exceptions

// A simple version marker for the save file format
const int CURRENT_SAVE_FORMAT_VERSION = 1;

// Helper to write a U64 to stream as a hex string on a new line
void write_u64_hex_to_stream(std::ostream& os, U64 val) {
    os << "0x" << std::hex << std::setw(16) << std::setfill('0') << val << std::dec << std::endl;
}

// Helper to read a U64 from stream (expects "0x" prefixed hex string on its own line)
bool read_u64_hex_from_stream(std::istream& is, U64& val) {
    std::string line;
    if (!std::getline(is, line) || line.empty()) {
        // std::cerr << "Read Error: Failed to read line for U64." << std::endl;
        return false;
    }
    
    line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
    line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

    if (line.rfind("0x", 0) != 0 && line.rfind("0X", 0) != 0) { 
        std::cerr << "Read Error: Expected hex prefix '0x' for U64, got: \"" << line << "\"" << std::endl;
        return false; 
    }
    std::stringstream ss(line.substr(2)); 
    ss >> std::hex >> val;
    if (ss.fail() || !ss.eof()) { 
        std::cerr << "Read Error: Failed to parse hex U64 from: \"" << line.substr(2) << "\"" << std::endl;
        return false;
    }
    return true;
}

// Helper to save a single BoardState object
void save_single_board_state(std::ostream& os, const BoardState& bs) {
    for (int pt = 0; pt < NUM_PIECE_TYPES; ++pt) {
        write_u64_hex_to_stream(os, bs.piece_bbs[pt][PLAYER_1]);
        write_u64_hex_to_stream(os, bs.piece_bbs[pt][PLAYER_2]);
    }
    os << static_cast<int>(bs.side_to_move) << std::endl;
}

// Helper to load a single BoardState object
bool load_single_board_state(std::istream& is, BoardState& bs) {
    for (int pt = 0; pt < NUM_PIECE_TYPES; ++pt) {
        if (!read_u64_hex_from_stream(is, bs.piece_bbs[pt][PLAYER_1])) return false;
        if (!read_u64_hex_from_stream(is, bs.piece_bbs[pt][PLAYER_2])) return false;
        bs.piece_bbs[pt][NO_PLAYER] = 0ULL; 
    }
    int side_to_move_int;
    std::string line;
    if (!std::getline(is, line)) return false;
    std::stringstream ss_side(line);
    if (!(ss_side >> side_to_move_int) || !ss_side.eof()) {
        std::cerr << "Read Error: Failed to parse side_to_move for board state from line: \"" << line << "\"" << std::endl;
        return false;
    }
    bs.side_to_move = static_cast<Player>(side_to_move_int);
    
    bs.update_occupancy_boards(); 
    return true;
}


bool save_game_state(
    const BoardState& board_state_to_save,
    const std::vector<BoardState>& history_to_save,
    int current_history_ply_to_save,
    bool game_over_status_to_save,
    Player winner_status_to_save,
    const std::string& filename) {

    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file for saving: " << filename << std::endl;
        return false;
    }

    outfile << "DSQSaveFormatVersion: " << CURRENT_SAVE_FORMAT_VERSION << std::endl;
    
    outfile << "CurrentBoardStateMarker:" << std::endl; 
    save_single_board_state(outfile, board_state_to_save);

    outfile << "GameOverStatus: " << (game_over_status_to_save ? 1 : 0) << std::endl;
    outfile << "WinnerStatus: " << static_cast<int>(winner_status_to_save) << std::endl;

    outfile << "HistorySize: " << history_to_save.size() << std::endl;
    outfile << "CurrentHistoryPly: " << current_history_ply_to_save << std::endl;
    
    outfile << "HistoryStatesMarker:" << std::endl; 
    for (const auto& hist_state : history_to_save) {
        save_single_board_state(outfile, hist_state);
    }

    outfile.close();
    std::cout << "Game state saved to " << filename << std::endl;
    return true;
}


bool load_game_state(
    BoardState& board_state_to_load_into,
    std::vector<BoardState>& history_to_load_into,
    int& current_history_ply_to_load_into,
    bool& game_over_status_to_load_into,
    Player& winner_status_to_load_into,
    const std::string& filename) {

    std::ifstream infile(filename);
    if (!infile.is_open()) {
        return false; 
    }

    std::string line;
    int version = 0;
    
    if (!std::getline(infile, line)) { infile.close(); return false; }
    if (line.find("DSQSaveFormatVersion: ") == 0) {
        try {
            version = std::stoi(line.substr(std::string("DSQSaveFormatVersion: ").length()));
        } catch (const std::exception& e) {
            std::cerr << "Error parsing save file version: " << e.what() << " from line: \"" << line << "\"" << std::endl;
            infile.close(); return false;
        }
    } else {
        std::cerr << "Error: Missing or malformed save file version line: \"" << line << "\"" << std::endl;
        infile.close(); return false;
    }

    if (version != CURRENT_SAVE_FORMAT_VERSION) {
        std::cerr << "Error: Unknown save file version " << version << ". Expected " << CURRENT_SAVE_FORMAT_VERSION << "." << std::endl;
        infile.close(); return false;
    }

    if (!std::getline(infile, line) || line != "CurrentBoardStateMarker:") { 
        std::cerr << "Error: Missing CurrentBoardStateMarker. Got: \"" << line << "\"" << std::endl; infile.close(); return false; 
    }
    if (!load_single_board_state(infile, board_state_to_load_into)) {
        std::cerr << "Error loading current board state." << std::endl; infile.close(); return false;
    }

    if (!std::getline(infile, line) || line.find("GameOverStatus: ") != 0) { 
        std::cerr << "Error: Missing or malformed GameOverStatus line: \"" << line << "\"" << std::endl; infile.close(); return false; 
    }
    try {
        game_over_status_to_load_into = (std::stoi(line.substr(std::string("GameOverStatus: ").length())) == 1);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing GameOverStatus: " << e.what() << " from line: \"" << line << "\"" << std::endl;
        infile.close(); return false;
    }
    
    if (!std::getline(infile, line) || line.find("WinnerStatus: ") != 0) {
        std::cerr << "Error: Missing or malformed WinnerStatus line: \"" << line << "\"" << std::endl; infile.close(); return false; 
    }
    try {
        winner_status_to_load_into = static_cast<Player>(std::stoi(line.substr(std::string("WinnerStatus: ").length())));
    } catch (const std::exception& e) {
        std::cerr << "Error parsing WinnerStatus: " << e.what() << " from line: \"" << line << "\"" << std::endl;
        infile.close(); return false;
    }

    history_to_load_into.clear();
    size_t history_size = 0;
    if (!std::getline(infile, line) || line.find("HistorySize: ") != 0) { 
        std::cerr << "Error: Missing or malformed HistorySize line: \"" << line << "\"" << std::endl; infile.close(); return false; 
    }
    try {
        history_size = std::stoull(line.substr(std::string("HistorySize: ").length()));
    } catch (const std::exception& e) {
        std::cerr << "Error parsing HistorySize: " << e.what() << " from line: \"" << line << "\"" << std::endl;
        infile.close(); return false;
    }


    if (!std::getline(infile, line) || line.find("CurrentHistoryPly: ") != 0) { 
        std::cerr << "Error: Missing or malformed CurrentHistoryPly line: \"" << line << "\"" << std::endl; infile.close(); return false; 
    }
    try {
        current_history_ply_to_load_into = std::stoi(line.substr(std::string("CurrentHistoryPly: ").length()));
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CurrentHistoryPly: " << e.what() << " from line: \"" << line << "\"" << std::endl;
        infile.close(); return false;
    }
    
    if (!std::getline(infile, line) || line != "HistoryStatesMarker:") { 
        std::cerr << "Error: Missing HistoryStatesMarker. Got: \"" << line << "\"" << std::endl; infile.close(); return false; 
    }

    history_to_load_into.reserve(history_size);
    for (size_t i = 0; i < history_size; ++i) {
        BoardState hist_state; 
        if (!load_single_board_state(infile, hist_state)) {
             std::cerr << "Error loading history state #" << i << std::endl; infile.close(); return false;
        }
        history_to_load_into.push_back(hist_state);
    }

    // Check if we read exactly the number of history states expected
    if (history_to_load_into.size() != history_size) {
        std::cerr << "Error: Mismatch in history size. Expected " << history_size << " states, loaded " << history_to_load_into.size() << std::endl;
        // This could happen if EOF is reached prematurely or other read errors occurred within load_single_board_state
        infile.close(); return false;
    }
    
    // Check for any trailing garbage data, though successful reads above should consume lines.
    // A simple check: try to read one more line. If it's not EOF and not empty, maybe an issue.
    // std::string junk;
    // if (std::getline(infile, junk) && !junk.empty()) {
    //     std::cerr << "Warning: Trailing data found in save file: " << junk << std::endl;
    // }
    
    infile.close();
    // Check stream state after closing (though usually not necessary if all reads were checked)
    // if (infile.fail() && !infile.eof()){ 
    //      std::cerr << "Error reading from save file after closing (unexpected stream failure)." << std::endl;
    //      return false;
    // }

    std::cout << "Game state loaded from " << filename << std::endl;
    return true;
}


