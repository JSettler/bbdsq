// bbdsq/main.cpp

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string> 
#include <vector>      
#include <iomanip>     
#include <stdexcept>   

#include "bitboard.h" 
#include "piece.h"    
#include "movegen.h" 
#include "ai.h"       
#include "evaluation.h" 
#include "board_state_io.h" 
#include "gui.h"            
#include "zobrist.h" 
#include "ttable.h" 

// --- Debug Logging Macros ---
#ifndef NDEBUG 
    #define DEBUG_LOG (std::cout) 
#else 
    struct NullStreamBuffer_for_debug_log : public std::streambuf { 
        int overflow(int c) override { return c; } 
    };
    struct NullStream_for_debug_log : public std::ostream { 
        NullStream_for_debug_log() : std::ostream(&m_sb) {}
    private:
        NullStreamBuffer_for_debug_log m_sb;
    };
    inline NullStream_for_debug_log& get_null_stream_for_debug_log() { 
        static NullStream_for_debug_log ns;
        return ns;
    }
    #define DEBUG_LOG (get_null_stream_for_debug_log())
#endif

// --- Global variable for search depth & TT size, can be overridden by command line ---
int g_search_depth = DEFAULT_AI_SEARCH_DEPTH; 
bool g_human_starts_game = false; 
size_t g_tt_size_mb = 256; // Default TT size in MB

// --- Game State Variables ---
BoardState current_board_state; 
int selected_square = -1; 
U64 possible_moves_bb = 0ULL; 
std::vector<Move> current_player_valid_moves; 

bool confirm_quit_active = false;
bool game_over = false;
Player winner = NO_PLAYER; 
Move last_ai_move; 

std::vector<BoardState> game_history; 
int current_history_index = -1;    
bool ai_should_think_automatically = true; 


// --- Helper function to print usage instructions ---
void print_help_message(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --depth <number>   Set AI search depth in plies (1-50)." << std::endl;
    std::cout << "                     Defaults to " << DEFAULT_AI_SEARCH_DEPTH << " if not specified." << std::endl;
    std::cout << "  --ttsize <MB>      Set Transposition Table size in Megabytes (1-16384)." << std::endl;
    std::cout << "                     Defaults to 256 MB if not specified." << std::endl;
    std::cout << "  --me               Human player (Player 2, Brown) makes the first move." << std::endl;
    std::cout << "  -h, --help         Show this help message and exit." << std::endl;
}


// --- History Management ---
void record_current_state_in_history() { 
    bool history_was_truncated = false;
    if (current_history_index >= -1 && current_history_index < static_cast<int>(game_history.size()) - 1) {
        game_history.erase(game_history.begin() + current_history_index + 1, game_history.end());
        history_was_truncated = true;
        DEBUG_LOG << "DEBUG: History truncated from index " << current_history_index + 1 << std::endl;
    }

    if (history_was_truncated) { // New move after undoing past some states
        TranspositionTable::clear_tt();
        std::cout << "Info: History diverged due to new move after undo, Transposition Table cleared." << std::endl;
    }

    BoardState state_to_record = current_board_state; 
    game_history.push_back(state_to_record); 
    current_history_index = static_cast<int>(game_history.size()) - 1;
    DEBUG_LOG << "DEBUG: Recorded state. History size: " << game_history.size() << ", Idx: " << current_history_index 
              << ", Side: P" << static_cast<int>(state_to_record.side_to_move) 
              << ", Hash: 0x" << std::hex << state_to_record.zobrist_hash << std::dec << std::endl;
}

void apply_state_from_history(int history_idx) {
    if (history_idx >= 0 && history_idx < static_cast<int>(game_history.size())) {
        current_board_state = game_history[history_idx]; 
        current_history_index = history_idx;
        
        game_over = false; 
        winner = NO_PLAYER;
        Player p_to_move_in_restored_state = current_board_state.side_to_move;

        if (p_to_move_in_restored_state == NO_PLAYER) { 
             game_over = true; 
             if ((current_board_state.occupancy_bbs[PLAYER_2] & P1_DEN_SQUARE_MASK) != 0ULL) winner = PLAYER_2;
             else if ((current_board_state.occupancy_bbs[PLAYER_1] & P2_DEN_SQUARE_MASK) != 0ULL) winner = PLAYER_1;
             else if (current_board_state.occupancy_bbs[PLAYER_1] == 0ULL && current_board_state.occupancy_bbs[PLAYER_2] != 0ULL) winner = PLAYER_2;
             else if (current_board_state.occupancy_bbs[PLAYER_2] == 0ULL && current_board_state.occupancy_bbs[PLAYER_1] != 0ULL) winner = PLAYER_1;
        } else {
            Player last_player_who_moved = (p_to_move_in_restored_state == PLAYER_1) ? PLAYER_2 : PLAYER_1;
            if (last_player_who_moved != NO_PLAYER) { 
                U64 last_player_den_target = (last_player_who_moved == PLAYER_1) ? P2_DEN_SQUARE_MASK : P1_DEN_SQUARE_MASK;
                if ((current_board_state.occupancy_bbs[last_player_who_moved] & last_player_den_target) != 0ULL) {
                    U64 den_pieces = current_board_state.occupancy_bbs[last_player_who_moved] & last_player_den_target;
                    if (den_pieces != 0ULL) { game_over = true; winner = last_player_who_moved; }
                }
                if (!game_over) {
                    Player opponent_of_last_player = p_to_move_in_restored_state;
                    if (current_board_state.occupancy_bbs[opponent_of_last_player] == 0ULL && current_board_state.occupancy_bbs[last_player_who_moved] != 0ULL) {
                        game_over = true; winner = last_player_who_moved;
                    }
                }
            }
        }

        selected_square = -1;
        possible_moves_bb = 0ULL;
        current_player_valid_moves.clear();
        last_ai_move = Move(); 
        // TT is NOT cleared here, as we are just navigating existing history.
        // It's cleared if a *new move* is made from an undone state (handled in record_current_state_in_history).
        std::cout << "Applied state from history index " << history_idx << ". Side to move: P" << static_cast<int>(current_board_state.side_to_move) 
                  << ". Game over: " << (game_over ? "true" : "false") << ", Winner: " << static_cast<int>(winner) << std::endl;
    } else {
        std::cout << "Invalid history index for apply: " << history_idx << std::endl;
    }
}


int main(int argc, char* argv[]) { 
    // --- Command Line Argument Parsing ---
    std::vector<std::string> args(argv + 1, argv + argc); 
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--help" || arg == "-h") {
            print_help_message(argv[0]);
            return 0; 
        } else if (arg == "--depth") {
            if (i + 1 < args.size()) { 
                try {
                    int depth_val = std::stoi(args[i + 1]);
                    if (depth_val >= 1 && depth_val <= 50) { 
                        g_search_depth = depth_val;
                    } else {
                        std::cerr << "Error: Depth value " << args[i + 1] << " out of range (1-50)." << std::endl;
                        print_help_message(argv[0]);
                        return 1;
                    }
                } catch (const std::invalid_argument& ia) {
                    std::cerr << "Error: Invalid number for --depth: " << args[i + 1] << std::endl;
                    print_help_message(argv[0]);
                    return 1;
                } catch (const std::out_of_range& oor) {
                    std::cerr << "Error: Depth value out of range for integer type: " << args[i + 1] << std::endl;
                    print_help_message(argv[0]);
                    return 1;
                }
                i++; 
            } else {
                std::cerr << "Error: --depth option requires a value." << std::endl;
                print_help_message(argv[0]);
                return 1;
            }
        } else if (arg == "--ttsize") { 
            if (i + 1 < args.size()) {
                try {
                    long long tt_val_ll = std::stoll(args[i + 1]); 
                    if (tt_val_ll >= 1 && tt_val_ll <= 16384) { 
                        g_tt_size_mb = static_cast<size_t>(tt_val_ll);
                    } else {
                        std::cerr << "Error: --ttsize value " << args[i + 1] << " out of range (1-16384 MB)." << std::endl;
                        print_help_message(argv[0]);
                        return 1;
                    }
                } catch (const std::invalid_argument& ia) {
                    std::cerr << "Error: Invalid number for --ttsize: " << args[i + 1] << std::endl;
                    print_help_message(argv[0]);
                    return 1;
                } catch (const std::out_of_range& oor) {
                    std::cerr << "Error: --ttsize value out of range for integer type: " << args[i + 1] << std::endl;
                    print_help_message(argv[0]);
                    return 1;
                }
                i++; 
            } else {
                std::cerr << "Error: --ttsize option requires a value (MB)." << std::endl;
                print_help_message(argv[0]);
                return 1;
            }
        } else if (arg == "--me") {
            g_human_starts_game = true;
        }
         else {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            print_help_message(argv[0]);
            return 1; 
        }
    }
    if (g_search_depth != DEFAULT_AI_SEARCH_DEPTH) { 
        std::cout << "AI search depth set to " << g_search_depth << " plies from command line." << std::endl;
    }
    if (g_tt_size_mb != 256) { // Assuming 256 was the default before this param
        std::cout << "Transposition Table size set to " << g_tt_size_mb << " MB from command line." << std::endl;
    }
    // --- End Command Line Argument Parsing ---

    Zobrist::initialize_keys(); 
    init_masks();               
    TranspositionTable::initialize_tt(g_tt_size_mb); 
    
    const std::string local_font_path = "arial-monospace.ttf"; 
    const std::string system_font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
    if (!GUI::initialize(local_font_path, system_font_path)) {
        std::cerr << "MAIN: GUI Initialization failed. Exiting." << std::endl;
        TranspositionTable::cleanup_tt(); 
        return 1;
    }
    
    if (!load_game_state(current_board_state, game_history, current_history_index, game_over, winner)) {
        std::cout << "No save file found or error loading. Starting new game." << std::endl;
        current_board_state.setup_initial_board(); 
        if (g_human_starts_game) { 
            if (current_board_state.side_to_move == PLAYER_1) { 
                current_board_state.zobrist_hash ^= Zobrist::side_to_move_key[PLAYER_1]; 
                current_board_state.side_to_move = PLAYER_2;
                current_board_state.zobrist_hash ^= Zobrist::side_to_move_key[PLAYER_2]; 
                std::cout << "New game: Player 2 (Human) to move first due to --me flag." << std::endl;
            }
        }
        game_history.clear(); 
        game_over = false; 
        winner = NO_PLAYER;
        current_history_index = -1; 
        ai_should_think_automatically = (current_board_state.side_to_move != PLAYER_1 || game_over); 
        record_current_state_in_history();    
        TranspositionTable::clear_tt(); 
    } else {
        std::cout << "Previous game loaded." << std::endl;
        if (current_history_index >= 0 && current_history_index < static_cast<int>(game_history.size())) {
            current_board_state = game_history[current_history_index];
        } else if (!game_history.empty()) { 
             current_history_index = static_cast<int>(game_history.size()) -1;
             if (current_history_index >=0) current_board_state = game_history[current_history_index];
             else { current_board_state.setup_initial_board(); record_current_state_in_history(); }
        } else { current_board_state.setup_initial_board(); record_current_state_in_history(); }
        if (current_board_state.side_to_move == NO_PLAYER && !game_over) { game_over = true; }
        ai_should_think_automatically = !(current_board_state.side_to_move == PLAYER_1 && !game_over);
        if (current_board_state.side_to_move == PLAYER_1 && !game_over) {
            std::cout << "Game loaded to AI's turn. Press 'G' for AI to move." << std::endl;
        }
        TranspositionTable::clear_tt(); 
    }

    sf::RenderWindow window(sf::VideoMode(GUI::get_initial_window_width(), GUI::get_initial_window_height()), "bbdsq");
    window.setFramerateLimit(60);
    
    GUI::handle_resize(window.getSize().x, window.getSize().y); 
    
    std::cout << std::fixed << std::setprecision(2); 

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                save_game_state(current_board_state, game_history, current_history_index, game_over, winner); 
                window.close();
            }

            if (event.type == sf::Event::Resized) {
                GUI::handle_resize(event.size.width, event.size.height);
            }

            if (confirm_quit_active) { 
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Y) {
                        save_game_state(current_board_state, game_history, current_history_index, game_over, winner); 
                        window.close();
                    }
                    else { confirm_quit_active = false; } 
                }
                if (event.type == sf::Event::MouseButtonPressed) confirm_quit_active = false;
            } else { 
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Escape) {
                        if (!confirm_quit_active) { 
                           confirm_quit_active = true; selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear();
                        }
                    }
                    if (event.key.control && event.key.code == sf::Keyboard::S) {
                        save_game_state(current_board_state, game_history, current_history_index, game_over, winner);
                    }
                    if (event.key.control && event.key.code == sf::Keyboard::L) {
                        if(load_game_state(current_board_state, game_history, current_history_index, game_over, winner)) {
                            selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear(); last_ai_move = Move(); 
                            ai_should_think_automatically = !(current_board_state.side_to_move == PLAYER_1 && !game_over);
                            TranspositionTable::clear_tt(); 
                            if (current_board_state.side_to_move == PLAYER_1 && !game_over) {
                                std::cout << "Game loaded to AI's turn. Press 'G' for AI to move." << std::endl;
                            }
                        } else { std::cout << "Failed to load game or no save file found." << std::endl; }
                    }
                    if (event.key.code == sf::Keyboard::Backspace && !event.key.shift) { 
                        if (current_history_index > 0) { 
                            apply_state_from_history(current_history_index - 1);
                            last_ai_move = Move(); 
                            if (current_board_state.side_to_move == PLAYER_1 && !game_over) {
                                ai_should_think_automatically = false; 
                                std::cout << "Undo to AI's turn. Press 'G' for AI to move." << std::endl;
                            } else { ai_should_think_automatically = true; }
                        } else { std::cout << "Cannot undo further." << std::endl; }
                    }
                    if (event.key.code == sf::Keyboard::Backspace && event.key.shift) { 
                        if (current_history_index < static_cast<int>(game_history.size()) - 1) {
                            apply_state_from_history(current_history_index + 1);
                            last_ai_move = Move(); 
                            if (current_board_state.side_to_move == PLAYER_1 && !game_over) {
                                ai_should_think_automatically = false; 
                                std::cout << "Redo to AI's turn. Press 'G' for AI to move." << std::endl;
                            } else { ai_should_think_automatically = true; }
                        } else { std::cout << "Cannot redo further." << std::endl; }
                    }
                    if (event.key.code == sf::Keyboard::G) { 
                        if (!game_over && current_board_state.side_to_move == PLAYER_1) {
                            ai_should_think_automatically = true; 
                            std::cout << "AI Go command received. AI will think." << std::endl;
                        } else if (current_board_state.side_to_move != PLAYER_1) {
                            std::cout << "Info: 'G' key pressed, but it's not AI's (Player 1) turn." << std::endl;
                        } else if (game_over){
                             std::cout << "Info: 'G' key pressed, but game is over." << std::endl;
                        }
                    }
                } 

                if (!game_over) { 
                    // --- HUMAN PLAYER'S TURN (PLAYER_2) ---
                    if (current_board_state.side_to_move == PLAYER_2) {
                        ai_should_think_automatically = true; 
                        if (event.type == sf::Event::MouseButtonPressed) {
                            if (event.mouseButton.button == sf::Mouse::Left) {
                                sf::Vector2i mouse_pos_window = sf::Mouse::getPosition(window);
                                sf::Vector2f mouse_pos_world = window.mapPixelToCoords(mouse_pos_window, GUI::get_game_view());
                                int cell_col_world = static_cast<int>(mouse_pos_world.x / GUI::SQUARE_SIZE_CALCULATED);
                                int cell_row_world = static_cast<int>(mouse_pos_world.y / GUI::SQUARE_SIZE_CALCULATED); 
                                float cell_top_left_x_world = static_cast<float>(cell_col_world * GUI::SQUARE_SIZE_CALCULATED);
                                float cell_top_left_y_world = static_cast<float>(cell_row_world * GUI::SQUARE_SIZE_CALCULATED);
                                bool clicked_on_gfx_part = (mouse_pos_world.x >= cell_top_left_x_world &&
                                                           mouse_pos_world.x < cell_top_left_x_world + GUI::TILE_GFX_SIZE_ACTUAL &&
                                                           mouse_pos_world.y >= cell_top_left_y_world &&
                                                           mouse_pos_world.y < cell_top_left_y_world + GUI::TILE_GFX_SIZE_ACTUAL);
                                
                                if (clicked_on_gfx_part &&
                                    cell_col_world >= 0 && cell_col_world < BOARD_WIDTH &&
                                    cell_row_world >= 0 && cell_row_world < BOARD_HEIGHT) {
                                    int c_model_clicked = cell_col_world;
                                    int r_model_clicked = BOARD_HEIGHT - 1 - cell_row_world; 
                                    int clicked_sq_idx = get_square_index(c_model_clicked, r_model_clicked);
                                    Player player_whose_turn_it_is = current_board_state.side_to_move; 

                                    Move human_move_to_make; 
                                    bool human_move_is_valid_target = false;
                                    for(const auto& legal_move_from_list : current_player_valid_moves) {
                                        if (legal_move_from_list.from_sq == selected_square && legal_move_from_list.to_sq == clicked_sq_idx) {
                                            human_move_is_valid_target = true;
                                            human_move_to_make = legal_move_from_list;
                                            break;
                                        }
                                    }

                                    if (selected_square != -1 && human_move_is_valid_target) {
                                        if (human_move_to_make.piece_captured != NO_PIECE_TYPE) {
                                            std::cout << "Player 2 (Human) " << PIECE_CHARS[human_move_to_make.piece_moved] << " at " << square_to_algebraic(human_move_to_make.from_sq)
                                                      << " CAPTURES " << PIECE_CHARS[human_move_to_make.piece_captured] << " at " << square_to_algebraic(human_move_to_make.to_sq) << std::endl;
                                        } else {
                                            std::cout << "Player 2 (Human) " << PIECE_CHARS[human_move_to_make.piece_moved] << " at " << square_to_algebraic(human_move_to_make.from_sq)
                                                      << " MOVES to " << square_to_algebraic(human_move_to_make.to_sq) << std::endl;
                                        }
                                        
                                        current_board_state.apply_move(human_move_to_make); 
                                        
                                        if (human_move_to_make.piece_captured != NO_PIECE_TYPE) { 
                                            Player opponent_of_human = PLAYER_1;
                                            if (current_board_state.occupancy_bbs[opponent_of_human] == 0ULL) {
                                                game_over = true; winner = player_whose_turn_it_is; 
                                                DEBUG_LOG << "DEBUG: Human P" << static_cast<int>(player_whose_turn_it_is) << " wins by wipeout." << std::endl;
                                            }
                                        }
                                        if (!game_over) { 
                                            if ((current_board_state.occupancy_bbs[player_whose_turn_it_is] & P1_DEN_SQUARE_MASK) != 0ULL) { 
                                                game_over = true; winner = player_whose_turn_it_is;
                                                DEBUG_LOG << "DEBUG: Human P" << static_cast<int>(player_whose_turn_it_is) << " wins by den entry." << std::endl;
                                            }
                                        }
                                        
                                        DEBUG_LOG << "DEBUG: After Human P" << static_cast<int>(player_whose_turn_it_is) << " move. game_over = " << (game_over ? "true" : "false") 
                                                  << ", winner = " << static_cast<int>(winner) << std::endl;

                                        record_current_state_in_history(); 
                                        if (game_over) {
                                            current_board_state.side_to_move = NO_PLAYER; 
                                            DEBUG_LOG << "DEBUG: Game is over (Human move), side_to_move set to NO_PLAYER." << std::endl;
                                        } else {
                                             DEBUG_LOG << "DEBUG: Switching to PLAYER_1 (AI) turn." << std::endl;
                                        }
                                        
                                        selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear();
                                    } else if (selected_square == clicked_sq_idx) { 
                                        selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear();
                                        std::cout << "Deselected " << square_to_algebraic(clicked_sq_idx) << std::endl;
                                    } else {
                                        Piece p_on_sq = current_board_state.get_piece_at(clicked_sq_idx);
                                        if (p_on_sq.player == player_whose_turn_it_is) { 
                                            selected_square = clicked_sq_idx;
                                            current_player_valid_moves = generate_all_legal_moves(current_board_state, player_whose_turn_it_is, game_history);
                                            
                                            possible_moves_bb = 0ULL; 
                                            bool found_moves_for_this_piece = false;
                                            for(const auto& legal_move : current_player_valid_moves) {
                                                if (legal_move.from_sq == selected_square) {
                                                    set_bit(possible_moves_bb, legal_move.to_sq);
                                                    found_moves_for_this_piece = true;
                                                }
                                            }
                                            std::cout << "Selected Player 2 (Human)" 
                                                      << " piece " << PIECE_CHARS[p_on_sq.type] << " at " << square_to_algebraic(clicked_sq_idx) << std::endl;
                                            if(found_moves_for_this_piece) {
                                                DEBUG_LOG << "Possible moves (rep checked):" << pretty_print_bb(possible_moves_bb) << std::endl;
                                            } else {
                                                std::cout << "No legal moves for selected piece (considering repetitions)." << std::endl;
                                            }
                                        } else { 
                                             selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear();
                                             if (p_on_sq.player != NO_PLAYER) std::cout << "Clicked opponent piece." << std::endl;
                                             else std::cout << "Clicked empty square with no selection." << std::endl;
                                        }
                                    }
                                } else { selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear();
                                     std::cout << "Clicked in gap or outside board. Deselected." << std::endl;
                                }
                            } 
                        }
                    } 
                    // --- AI PLAYER'S TURN (PLAYER_1) ---
                    else if (current_board_state.side_to_move == PLAYER_1) {
                        if (ai_should_think_automatically) {
                            std::cout << "\nPlayer 1 (AI) is thinking..." << std::endl;
                            AiMoveResult ai_result = find_best_ai_move(current_board_state, g_search_depth, game_history); 
                            last_ai_move = ai_result.best_move; 

                            std::cout << "------------------------------------" << std::endl;
                            std::cout << "AI Move Details (Player 1 - Grey):" << std::endl;
                            if (last_ai_move.from_sq != -1) { std::cout << "  Chosen Move: " << last_ai_move.to_string() << std::endl;} 
                            else { std::cout << "  No valid move chosen by AI (or stalemate)." << std::endl; }
                            std::cout << "  Projected Score: " << ai_result.final_score / 2.0 << " mc" << std::endl;
                            std::cout << "  Nodes Searched: " << ai_result.nodes_searched << std::endl;
                            std::cout << "  Time Taken: " << ai_result.time_taken_ms << " ms" << std::endl;
                            std::cout << "  Root Moves Considered: " << ai_result.root_moves_count << std::endl;
                            if (ai_result.time_taken_ms > 0.001) { std::cout << "  Nodes per Second: " << static_cast<long long>(ai_result.nodes_searched / (ai_result.time_taken_ms / 1000.0)) << std::endl;} 
                            else { std::cout << "  Nodes per Second: N/A (time too short)" << std::endl; }
                            // TT Stats
                            TranspositionTable::TTStats tt_stats = TranspositionTable::get_tt_stats();
                            std::cout << "  TT Entries Used: " << tt_stats.used_entries << " / " << tt_stats.total_entries 
                                      << " (" << std::fixed << std::setprecision(1) << tt_stats.utilization_percent << "%)" << std::endl;
                            std::cout << "------------------------------------" << std::endl;


                            if (last_ai_move.from_sq != -1 && last_ai_move.to_sq != -1) { 
                                current_board_state.apply_move(last_ai_move); 
                                
                                if (last_ai_move.piece_captured != NO_PIECE_TYPE) { 
                                    Player opponent_of_ai = PLAYER_2; 
                                    if (current_board_state.occupancy_bbs[opponent_of_ai] == 0ULL) {
                                        game_over = true; winner = PLAYER_1;
                                        DEBUG_LOG << "DEBUG: AI P1 wins by wipeout." << std::endl;
                                    }
                                }
                                if (!game_over) { 
                                    if ((current_board_state.occupancy_bbs[PLAYER_1] & P2_DEN_SQUARE_MASK) != 0ULL) { 
                                        game_over = true; winner = PLAYER_1;
                                        DEBUG_LOG << "DEBUG: AI P1 wins by den entry." << std::endl;
                                    }
                                }
                                
                                DEBUG_LOG << "DEBUG: After AI P1 move. game_over = " << (game_over ? "true" : "false") 
                                          << ", winner = " << static_cast<int>(winner) << std::endl;

                                record_current_state_in_history(); 
                                if (game_over) {
                                    current_board_state.side_to_move = NO_PLAYER;
                                    DEBUG_LOG << "DEBUG: Game is over (AI move), side_to_move set to NO_PLAYER." << std::endl;
                                } else {
                                     DEBUG_LOG << "DEBUG: Switching to PLAYER_2 (Human) turn." << std::endl;
                                }
                            } else { 
                                std::cout << "Player 1 (AI) has no legal moves. Player 2 (Human) WINS!" << std::endl;
                                game_over = true; winner = PLAYER_2; 
                                current_board_state.side_to_move = NO_PLAYER;
                                record_current_state_in_history(); 
                            }
                            selected_square = -1; possible_moves_bb = 0ULL; current_player_valid_moves.clear();
                            ai_should_think_automatically = true; 
                        } 
                    } 
                } else { // Game is over 
                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                        if (!confirm_quit_active) { confirm_quit_active = true; }
                    }
                } 
            } 
        } 

        window.setView(GUI::get_game_view()); 
        window.clear(GUI::COLOR_GAP_BORDER);  
        GUI::draw_board_layout(window); 
        if (last_ai_move.from_sq != -1 && ( (game_over && winner == PLAYER_1) || (!game_over && current_board_state.side_to_move == PLAYER_2) ) ) { 
             GUI::draw_last_ai_move_highlight(window, last_ai_move);
        }
        GUI::draw_pieces(window, current_board_state); 
        if (!confirm_quit_active && selected_square != -1 && current_board_state.side_to_move == PLAYER_2) { 
            GUI::draw_selection_highlight(window, selected_square);
            GUI::draw_possible_moves(window, possible_moves_bb); 
        }
        
        window.setView(window.getDefaultView()); 
        GUI::draw_ui_text_elements(window, current_board_state, game_over, winner); 
        GUI::draw_quit_confirmation(window, confirm_quit_active); 
        
        window.display();
    }

    TranspositionTable::cleanup_tt(); 
    return 0;
}


