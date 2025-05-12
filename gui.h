// bbdsq/gui.h
#ifndef GUI_H
#define GUI_H

#include <SFML/Graphics.hpp>
#include <string>
#include "bitboard.h" // For U64, NUM_SQUARES, board dimensions, get_col_row etc.
#include "piece.h"    // For BoardState, Piece, Player, PieceType, PIECE_CHARS
#include "movegen.h"  // For Move struct (used in draw_last_ai_move_highlight)
                      // Note: If Move struct was moved to its own "move.h", include that instead.

namespace GUI {

    // --- GUI Configuration Constants (can be overridden by initialize) ---
    const int DEFAULT_TILE_GFX_SIZE = 58;
    const int DEFAULT_TILE_GAP = 2;

    // --- Calculated GUI Dimensions (set by GUI::initialize) ---
    // These are extern because their values are determined at runtime by initialize()
    // and used by drawing functions. They are defined in gui.cpp.
    extern int TILE_GFX_SIZE_ACTUAL;
    extern int TILE_GAP_ACTUAL;
    extern int SQUARE_SIZE_CALCULATED; 
    extern int WINDOW_WIDTH_CALCULATED;
    extern int WINDOW_HEIGHT_CALCULATED;
    extern sf::Vector2f GAME_WORLD_SIZE_CALCULATED;


    // --- Color Constants (defined in gui.cpp) ---
    extern const sf::Color COLOR_GAP_BORDER;
    extern const sf::Color COLOR_LAND_TILE;
    extern const sf::Color COLOR_LAKE_TILE;
    extern const sf::Color COLOR_TRAP_TILE;
    extern const sf::Color COLOR_DEN_TILE;
    extern const sf::Color COLOR_PLAYER1_PIECE_BG;
    extern const sf::Color COLOR_PLAYER1_PIECE_FG;
    extern const sf::Color COLOR_PLAYER2_PIECE_BG;
    extern const sf::Color COLOR_PLAYER2_PIECE_FG;
    extern const sf::Color COLOR_POSSIBLE_MOVE_HIGHLIGHT;
    extern const sf::Color COLOR_STATUS_TEXT;
    extern const sf::Color COLOR_WIN_TEXT;
    extern const sf::Color COLOR_LAST_AI_MOVE_HIGHLIGHT;
    extern const sf::Color COLOR_QUIT_CONFIRM_BG;
    extern const sf::Color COLOR_QUIT_CONFIRM_TEXT;


    // --- Initialization and Management ---
    // Call this once at the start from main.cpp.
    // Sets up tile sizes, calculates window dimensions, loads font, initializes view.
    // Returns true on success (e.g., font loaded).
    bool initialize(const std::string& local_font_path, 
                    const std::string& system_font_path,
                    int tile_gfx_size = DEFAULT_TILE_GFX_SIZE, 
                    int tile_gap = DEFAULT_TILE_GAP);

    // Access to the loaded font (primarily for internal GUI use).
    sf::Font& get_font(); 

    // View Management
    // Gets the main game view (managed internally by GUI module).
    sf::View& get_game_view(); 
    // Call this from main.cpp's event loop on sf::Event::Resized.
    void handle_resize(unsigned int new_window_width, unsigned int new_window_height); 

    // --- Drawing Functions ---
    // These functions will use the internally managed font, view, and constants.
    void draw_board_layout(sf::RenderWindow& window);
    void draw_pieces(sf::RenderWindow& window, const BoardState& board_state); // Needs BoardState
    void draw_selection_highlight(sf::RenderWindow& window, int sq_idx);       // Needs selected square
    void draw_possible_moves(sf::RenderWindow& window, U64 moves_bb);          // Needs possible moves bb
    void draw_last_ai_move_highlight(sf::RenderWindow& window, const Move& move); // Needs last AI move
    
    // UI Elements (drawn with default view)
    void draw_ui_text_elements(sf::RenderWindow& window, const BoardState& board_state, bool is_game_over, Player winning_player);
    void draw_quit_confirmation(sf::RenderWindow& window, bool confirm_quit_active_status);


    // --- Accessors for Calculated Dimensions ---
    // These are needed by main.cpp to create the sf::RenderWindow initially.
    int get_initial_window_width();
    int get_initial_window_height();

} // namespace GUI

#endif // GUI_H


