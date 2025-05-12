// bbdsq/gui.cpp
#include "gui.h"
#include <iostream> // For std::cerr, std::cout

namespace GUI {

    // --- Definitions for Calculated GUI Dimensions & Resources ---
    // These are defined here and extern declared in gui.h
    int TILE_GFX_SIZE_ACTUAL;
    int TILE_GAP_ACTUAL;
    int SQUARE_SIZE_CALCULATED; 
    int WINDOW_WIDTH_CALCULATED;
    int WINDOW_HEIGHT_CALCULATED;
    sf::Vector2f GAME_WORLD_SIZE_CALCULATED;

    // Internal static variables for font and view
    static sf::Font global_gui_font;
    static sf::View global_game_view;
    static bool gui_initialized = false;

    // --- Color Constants Definitions ---
    const sf::Color COLOR_GAP_BORDER = sf::Color::Black;
    const sf::Color COLOR_LAND_TILE = sf::Color(0, 80, 0);       
    const sf::Color COLOR_LAKE_TILE = sf::Color(0, 0, 100);      
    const sf::Color COLOR_TRAP_TILE = sf::Color(100, 0, 0);      
    const sf::Color COLOR_DEN_TILE = sf::Color(30, 30, 30);      
    const sf::Color COLOR_PLAYER1_PIECE_BG = sf::Color(100, 100, 100); 
    const sf::Color COLOR_PLAYER1_PIECE_FG = sf::Color::Black;       
    const sf::Color COLOR_PLAYER2_PIECE_BG = sf::Color(139, 69, 19);   
    const sf::Color COLOR_PLAYER2_PIECE_FG = sf::Color::Yellow;      
    const sf::Color COLOR_POSSIBLE_MOVE_HIGHLIGHT = sf::Color(0, 200, 0, 100); 
    const sf::Color COLOR_STATUS_TEXT = sf::Color(160, 160, 160); 
    const sf::Color COLOR_WIN_TEXT = sf::Color::Red;
    const sf::Color COLOR_LAST_AI_MOVE_HIGHLIGHT = sf::Color(100, 100, 255, 120); 
    const sf::Color COLOR_QUIT_CONFIRM_BG = sf::Color(50, 50, 50, 200);
    const sf::Color COLOR_QUIT_CONFIRM_TEXT = sf::Color::White;


    // --- Initialization and Management ---
    bool initialize(const std::string& local_font_path, 
                    const std::string& system_font_path,
                    int tile_gfx_size, 
                    int tile_gap) {
        TILE_GFX_SIZE_ACTUAL = tile_gfx_size;
        TILE_GAP_ACTUAL = tile_gap;
        SQUARE_SIZE_CALCULATED = TILE_GFX_SIZE_ACTUAL + TILE_GAP_ACTUAL;
        
        WINDOW_WIDTH_CALCULATED = BOARD_WIDTH * SQUARE_SIZE_CALCULATED - TILE_GAP_ACTUAL;
        WINDOW_HEIGHT_CALCULATED = BOARD_HEIGHT * SQUARE_SIZE_CALCULATED - TILE_GAP_ACTUAL;
        GAME_WORLD_SIZE_CALCULATED = sf::Vector2f(static_cast<float>(WINDOW_WIDTH_CALCULATED), 
                                                 static_cast<float>(WINDOW_HEIGHT_CALCULATED));

        if (!global_gui_font.loadFromFile(local_font_path)) {
            std::cerr << "GUI Warning: Could not load font from local path: " << local_font_path << std::endl;
            std::cerr << "GUI Attempting to load from system path: " << system_font_path << std::endl;
            if (!global_gui_font.loadFromFile(system_font_path)) {
                std::cerr << "GUI Error: Could not load font from system path either. UI text will not be visible." << std::endl;
                gui_initialized = false;
                return false;
            } else { 
                std::cout << "GUI Successfully loaded font from: " << system_font_path << std::endl; 
            }
        } else { 
            std::cout << "GUI Successfully loaded font from: " << local_font_path << std::endl; 
        }

        // Setup the Game View
        global_game_view.setSize(GAME_WORLD_SIZE_CALCULATED);
        global_game_view.setCenter(GAME_WORLD_SIZE_CALCULATED.x / 2.f, GAME_WORLD_SIZE_CALCULATED.y / 2.f);
        // Initial viewport setup (assuming window starts at the calculated size)
        global_game_view.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f)); // Full window initially

        gui_initialized = true;
        return true;
    }

    sf::Font& get_font() {
        return global_gui_font;
    }

    sf::View& get_game_view() {
        return global_game_view;
    }

    void handle_resize(unsigned int new_window_width, unsigned int new_window_height) {
        if (!gui_initialized) return;

        float world_ar = GAME_WORLD_SIZE_CALCULATED.x / GAME_WORLD_SIZE_CALCULATED.y;
        float window_ar = static_cast<float>(new_window_width) / static_cast<float>(new_window_height);
        sf::FloatRect vp(0.f, 0.f, 1.f, 1.f);

        if (window_ar > world_ar) { // Window wider than world (pillarbox)
            vp.width = world_ar / window_ar;
            vp.left = (1.f - vp.width) / 2.f;
        } else if (window_ar < world_ar) { // Window taller than world (letterbox)
            vp.height = window_ar / world_ar;
            vp.top = (1.f - vp.height) / 2.f;
        }
        global_game_view.setViewport(vp);
    }

    int get_initial_window_width() {
        return WINDOW_WIDTH_CALCULATED;
    }
    int get_initial_window_height() {
        return WINDOW_HEIGHT_CALCULATED;
    }

    // --- Drawing Functions ---
    void draw_board_layout(sf::RenderWindow& window) {
        if (!gui_initialized) return;
        sf::RectangleShape tile_shape(sf::Vector2f(static_cast<float>(TILE_GFX_SIZE_ACTUAL), static_cast<float>(TILE_GFX_SIZE_ACTUAL)));
        for (int r_model = 0; r_model < BOARD_HEIGHT; ++r_model) {
            for (int c_model = 0; c_model < BOARD_WIDTH; ++c_model) {
                int sq_idx = get_square_index(c_model, r_model);
                float world_y_pos_tile_top = static_cast<float>((BOARD_HEIGHT - 1 - r_model) * SQUARE_SIZE_CALCULATED); 
                float world_x_pos_tile_left = static_cast<float>(c_model * SQUARE_SIZE_CALCULATED); 
                tile_shape.setPosition(world_x_pos_tile_left, world_y_pos_tile_top);
                if (get_bit(LAKE_SQUARES_MASK, sq_idx)) tile_shape.setFillColor(COLOR_LAKE_TILE);
                else if (get_bit(P1_DEN_SQUARE_MASK, sq_idx) || get_bit(P2_DEN_SQUARE_MASK, sq_idx)) tile_shape.setFillColor(COLOR_DEN_TILE); 
                else if (get_bit(TRAPS_NEAR_P1_DEN_MASK, sq_idx) || get_bit(TRAPS_NEAR_P2_DEN_MASK, sq_idx)) tile_shape.setFillColor(COLOR_TRAP_TILE);
                else tile_shape.setFillColor(COLOR_LAND_TILE);
                window.draw(tile_shape);
            }
        }
    }

    void draw_pieces(sf::RenderWindow& window, const BoardState& board_state) {
        if (!gui_initialized) return;
        sf::CircleShape piece_bg_shape(TILE_GFX_SIZE_ACTUAL / 2.f * 0.85f); 
        sf::Text piece_text;
        piece_text.setFont(global_gui_font);
        unsigned int char_size = static_cast<unsigned int>(TILE_GFX_SIZE_ACTUAL * 0.55f); 
        piece_text.setCharacterSize(char_size); 
        for (int r_model = 0; r_model < BOARD_HEIGHT; ++r_model) {
            for (int c_model = 0; c_model < BOARD_WIDTH; ++c_model) {
                int sq_idx = get_square_index(c_model, r_model);
                Piece p = board_state.get_piece_at(sq_idx);
                if (p.type != NO_PIECE_TYPE) {
                    float world_y_pos_tile_top = static_cast<float>((BOARD_HEIGHT - 1 - r_model) * SQUARE_SIZE_CALCULATED); 
                    float world_x_pos_tile_left = static_cast<float>(c_model * SQUARE_SIZE_CALCULATED); 
                    float circle_center_x = world_x_pos_tile_left + TILE_GFX_SIZE_ACTUAL / 2.f;
                    float circle_center_y = world_y_pos_tile_top + TILE_GFX_SIZE_ACTUAL / 2.f;
                    piece_bg_shape.setPosition(circle_center_x - piece_bg_shape.getRadius(), circle_center_y - piece_bg_shape.getRadius());
                    piece_text.setString(std::string(1, PIECE_CHARS[p.type]));
                    if (p.player == PLAYER_1) { 
                        piece_bg_shape.setFillColor(COLOR_PLAYER1_PIECE_BG);
                        piece_text.setFillColor(COLOR_PLAYER1_PIECE_FG);
                    } else { 
                        piece_bg_shape.setFillColor(COLOR_PLAYER2_PIECE_BG);
                        piece_text.setFillColor(COLOR_PLAYER2_PIECE_FG);
                    }
                    window.draw(piece_bg_shape);
                    sf::FloatRect text_rect = piece_text.getLocalBounds();
                    piece_text.setOrigin(text_rect.left + text_rect.width / 2.0f, text_rect.top + text_rect.height / 2.0f); 
                    piece_text.setPosition(circle_center_x, circle_center_y);
                    window.draw(piece_text);
                }
            }
        }
    }

    void draw_selection_highlight(sf::RenderWindow& window, int sq_idx) {
        if (!gui_initialized || sq_idx < 0 || sq_idx >= NUM_SQUARES) return;
        std::pair<int, int> cr = get_col_row(sq_idx);
        int c_model = cr.first; int r_model = cr.second;
        float world_y_pos_tile_top = static_cast<float>((BOARD_HEIGHT - 1 - r_model) * SQUARE_SIZE_CALCULATED); 
        float world_x_pos_tile_left = static_cast<float>(c_model * SQUARE_SIZE_CALCULATED); 
        sf::RectangleShape highlight(sf::Vector2f(static_cast<float>(TILE_GFX_SIZE_ACTUAL), static_cast<float>(TILE_GFX_SIZE_ACTUAL)));
        highlight.setPosition(world_x_pos_tile_left, world_y_pos_tile_top); 
        highlight.setFillColor(sf::Color::Transparent); 
        highlight.setOutlineThickness(3); 
        highlight.setOutlineColor(sf::Color::Yellow); 
        window.draw(highlight);
    }

    void draw_possible_moves(sf::RenderWindow& window, U64 moves_bb) {
        if (!gui_initialized || moves_bb == 0ULL) return;
        sf::CircleShape move_marker(TILE_GFX_SIZE_ACTUAL / 5.f); 
        move_marker.setFillColor(COLOR_POSSIBLE_MOVE_HIGHLIGHT);
        U64 temp_moves_bb = moves_bb; 
        while(temp_moves_bb > 0) {
            int sq_idx = pop_lsb(temp_moves_bb); 
            if (sq_idx != -1) {
                std::pair<int, int> cr = get_col_row(sq_idx);
                int c_model = cr.first; int r_model = cr.second;
                float world_y_pos_tile_top = static_cast<float>((BOARD_HEIGHT - 1 - r_model) * SQUARE_SIZE_CALCULATED);
                float world_x_pos_tile_left = static_cast<float>(c_model * SQUARE_SIZE_CALCULATED);
                float marker_center_x = world_x_pos_tile_left + TILE_GFX_SIZE_ACTUAL / 2.f;
                float marker_center_y = world_y_pos_tile_top + TILE_GFX_SIZE_ACTUAL / 2.f;
                move_marker.setOrigin(move_marker.getRadius(), move_marker.getRadius());
                move_marker.setPosition(marker_center_x, marker_center_y);
                window.draw(move_marker);
            }
        }
    }

    void draw_last_ai_move_highlight(sf::RenderWindow& window, const Move& move) {
        if (!gui_initialized || move.from_sq == -1 || move.to_sq == -1) return;
        sf::RectangleShape from_highlight(sf::Vector2f(static_cast<float>(TILE_GFX_SIZE_ACTUAL), static_cast<float>(TILE_GFX_SIZE_ACTUAL)));
        sf::RectangleShape to_highlight(sf::Vector2f(static_cast<float>(TILE_GFX_SIZE_ACTUAL), static_cast<float>(TILE_GFX_SIZE_ACTUAL)));
        from_highlight.setFillColor(sf::Color::Transparent);
        to_highlight.setFillColor(sf::Color::Transparent);
        from_highlight.setOutlineThickness(2); 
        to_highlight.setOutlineThickness(2);
        from_highlight.setOutlineColor(COLOR_LAST_AI_MOVE_HIGHLIGHT);
        to_highlight.setOutlineColor(COLOR_LAST_AI_MOVE_HIGHLIGHT);
        std::pair<int, int> cr_from = get_col_row(move.from_sq);
        float from_y = static_cast<float>((BOARD_HEIGHT - 1 - cr_from.second) * SQUARE_SIZE_CALCULATED);
        float from_x = static_cast<float>(cr_from.first * SQUARE_SIZE_CALCULATED);
        from_highlight.setPosition(from_x, from_y);
        std::pair<int, int> cr_to = get_col_row(move.to_sq);
        float to_y = static_cast<float>((BOARD_HEIGHT - 1 - cr_to.second) * SQUARE_SIZE_CALCULATED);
        float to_x = static_cast<float>(cr_to.first * SQUARE_SIZE_CALCULATED);
        to_highlight.setPosition(to_x, to_y);
        window.draw(from_highlight);
        window.draw(to_highlight);
    }

    void draw_ui_text_elements(sf::RenderWindow& window, const BoardState& board_state, bool is_game_over, Player winning_player) {
        if (!gui_initialized || global_gui_font.getInfo().family.empty()) return; // Need font for text
        
        static sf::Text ui_text_obj; // Static to avoid re-creation, but font needs to be set
        ui_text_obj.setFont(global_gui_font);

        std::string message;
        if (is_game_over) {
            if (winning_player == PLAYER_1) message = "Player 1 (AI - Grey) WINS!";
            else if (winning_player == PLAYER_2) message = "Player 2 (Human - Brown) WINS!";
            else if (winning_player == NO_PLAYER) message = "Game Over - STALEMATE/DRAW!"; 
            else message = "Game Over - DRAW?"; 
            ui_text_obj.setFillColor(COLOR_WIN_TEXT); 
            ui_text_obj.setStyle(sf::Text::Bold);
        } else {
            if (board_state.side_to_move == PLAYER_1) message = "Turn: Player 1 (AI - Grey)";
            else if (board_state.side_to_move == PLAYER_2) message = "Turn: Player 2 (Human - Brown)";
            else message = "Turn: ???"; 
            ui_text_obj.setFillColor(COLOR_STATUS_TEXT); 
            ui_text_obj.setStyle(sf::Text::Regular);
        }
        ui_text_obj.setString(message);
        ui_text_obj.setCharacterSize(18); 
        ui_text_obj.setOrigin(0,0); ui_text_obj.setPosition(0,0); ui_text_obj.setRotation(0); ui_text_obj.setScale(1,1);
        sf::FloatRect text_bounds = ui_text_obj.getLocalBounds();
        ui_text_obj.setPosition(10.f, static_cast<float>(window.getSize().y) - text_bounds.height - 10.f); 
        window.draw(ui_text_obj);
    }

    void draw_quit_confirmation(sf::RenderWindow& window, bool confirm_quit_active_status) {
        if (!gui_initialized || !confirm_quit_active_status || global_gui_font.getInfo().family.empty()) return;

        static sf::Text quit_text_obj; // Static for efficiency
        quit_text_obj.setFont(global_gui_font); 
        quit_text_obj.setString("Quit? (Y/N)");
        quit_text_obj.setCharacterSize(30); 
        quit_text_obj.setFillColor(COLOR_QUIT_CONFIRM_TEXT);
        quit_text_obj.setStyle(sf::Text::Bold);
        
        quit_text_obj.setOrigin(0,0); quit_text_obj.setPosition(0,0); quit_text_obj.setRotation(0); quit_text_obj.setScale(1,1);
        sf::FloatRect text_rect = quit_text_obj.getLocalBounds();
        sf::Vector2u current_window_size = window.getSize();
        
        quit_text_obj.setOrigin(text_rect.left + text_rect.width / 2.0f, text_rect.top + text_rect.height / 2.0f);
        quit_text_obj.setPosition(static_cast<float>(current_window_size.x) / 2.0f, static_cast<float>(current_window_size.y) / 2.0f);

        static sf::RectangleShape bg_rect_obj; // Static for efficiency
        bg_rect_obj.setSize(sf::Vector2f(text_rect.width + 40, text_rect.height + 40));
        bg_rect_obj.setFillColor(COLOR_QUIT_CONFIRM_BG); 
        bg_rect_obj.setOrigin(bg_rect_obj.getSize().x / 2.0f, bg_rect_obj.getSize().y / 2.0f); 
        bg_rect_obj.setPosition(static_cast<float>(current_window_size.x) / 2.0f, static_cast<float>(current_window_size.y) / 2.0f);
        
        window.draw(bg_rect_obj);
        window.draw(quit_text_obj);
    }

} // namespace GUI


