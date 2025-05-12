// bbdsq/ai.cpp
#include "ai.h"
#include "movegen.h"    // For generate_all_legal_moves, Move struct
#include "evaluation.h" // For evaluate_board, WIN_SCORE, LOSS_SCORE
#include "piece.h"      // For BoardState, Piece, PIECE_CHARS, square_to_algebraic
#include "bitboard.h"   // For various constants if needed by included headers
#include "ttable.h"     // For Transposition Table
#include <vector>
#include <algorithm>    // For std::max, std::min, std::sort, std::find, std::rotate
#include <limits>       // For std::numeric_limits
#include <iostream>     // For AI thinking debug output
#include <chrono>       // For timing

// Applies a move to a given board state and returns the new state.
BoardState make_move_on_copy(const BoardState& current_board_state, const Move& move) {
    BoardState next_state = current_board_state; 
    if (move.from_sq == -1 || move.to_sq == -1) { 
        // This should ideally not be reached if move generation is correct
        // and find_best_ai_move handles empty move lists.
        std::cerr << "AI Error: make_move_on_copy called with invalid move (" << move.to_string() << ")" << std::endl;
        // To prevent crashes, ensure next_state is still somewhat valid or throw.
        // For now, it returns a copy of the original state, which might lead to AI re-evaluating it.
        return next_state; 
    }
    next_state.apply_move(move); // apply_move handles piece removal, movement, side_to_move, and hash
    return next_state;
}


int alpha_beta_search(
    BoardState board_state, 
    int depth,
    int alpha,
    int beta,
    Player player_for_whom_to_maximize, 
    Player current_turn_in_state,
    long long& nodes_searched_ref,
    const std::vector<BoardState>& game_history_for_this_node 
) {
    nodes_searched_ref++; 
    U64 current_hash = board_state.zobrist_hash; 
    int original_alpha_for_node_entry = alpha; // Store for TT flag determination

    // --- Transposition Table Probe ---
    TranspositionTable::TTEntry* tt_entry = TranspositionTable::probe_tt(current_hash);
    if (tt_entry != nullptr && tt_entry->depth >= depth) {
        // Entry is valid (key matched in probe_tt) and from a search at least as deep
        if (tt_entry->flag == TranspositionTable::EntryFlag::EXACT_SCORE) {
            return tt_entry->score;
        }
        if (tt_entry->flag == TranspositionTable::EntryFlag::LOWER_BOUND) {
            alpha = std::max(alpha, tt_entry->score);
        } else if (tt_entry->flag == TranspositionTable::EntryFlag::UPPER_BOUND) {
            beta = std::min(beta, tt_entry->score);
        }
        if (alpha >= beta) {
            return tt_entry->score; // This bound caused a cutoff
        }
    }
    // --- End TT Probe ---

    int current_eval_score = evaluate_board(board_state, player_for_whom_to_maximize);
    if (depth == 0 || current_eval_score == WIN_SCORE || current_eval_score == LOSS_SCORE) {
        // Store leaf node or terminal state evaluation. Best move is irrelevant here.
        TranspositionTable::store_tt_entry(current_hash, current_eval_score, depth, TranspositionTable::EntryFlag::EXACT_SCORE, Move());
        return current_eval_score;
    }

    std::vector<Move> legal_moves = generate_all_legal_moves(board_state, current_turn_in_state, game_history_for_this_node);

    if (legal_moves.empty()) { 
        int score = (current_turn_in_state == player_for_whom_to_maximize) ? LOSS_SCORE : WIN_SCORE;
        TranspositionTable::store_tt_entry(current_hash, score, depth, TranspositionTable::EntryFlag::EXACT_SCORE, Move());
        return score;  
    }

    // --- Move Ordering: Use TT's best move first if available from a previous (shallower) search ---
    if (tt_entry != nullptr && tt_entry->best_move.from_sq != -1) {
        auto it = std::find(legal_moves.begin(), legal_moves.end(), tt_entry->best_move);
        if (it != legal_moves.end()) {
            // Move the TT best move to the front of the list
            std::rotate(legal_moves.begin(), it, it + 1);
        }
    }
    // (Further move ordering like MVV-LVA for captures could be applied to the rest of legal_moves here)

    Move best_move_found_at_this_node; 
    TranspositionTable::EntryFlag flag_for_tt_store;

    if (current_turn_in_state == player_for_whom_to_maximize) { 
        int max_eval = std::numeric_limits<int>::min(); 
        flag_for_tt_store = TranspositionTable::EntryFlag::UPPER_BOUND; // Assume all moves fail low initially

        for (const Move& move : legal_moves) {
            std::vector<BoardState> next_history = game_history_for_this_node;
            next_history.push_back(board_state); 

            BoardState next_state = make_move_on_copy(board_state, move);
            int eval = alpha_beta_search(next_state, depth - 1, alpha, beta, player_for_whom_to_maximize, next_state.side_to_move, nodes_searched_ref, next_history);
            
            if (eval > max_eval) {
                max_eval = eval;
                best_move_found_at_this_node = move;
            }
            alpha = std::max(alpha, eval);
            if (beta <= alpha) { // Beta cutoff (fail high)
                flag_for_tt_store = TranspositionTable::EntryFlag::LOWER_BOUND;
                break; 
            }
        }
        // Refined flag determination
        if (max_eval > original_alpha_for_node_entry && max_eval < beta) { // Check against original beta passed to this node
             flag_for_tt_store = TranspositionTable::EntryFlag::EXACT_SCORE;
        } // else it remains UPPER_BOUND (if no move raised alpha) or becomes LOWER_BOUND (if beta cutoff)
        
        TranspositionTable::store_tt_entry(current_hash, max_eval, depth, flag_for_tt_store, best_move_found_at_this_node);
        return max_eval;
    } else { // Minimizing player's turn
        int min_eval = std::numeric_limits<int>::max(); 
        flag_for_tt_store = TranspositionTable::EntryFlag::LOWER_BOUND; // Assume all moves fail high initially

        for (const Move& move : legal_moves) {
            std::vector<BoardState> next_history = game_history_for_this_node;
            next_history.push_back(board_state);

            BoardState next_state = make_move_on_copy(board_state, move);
            int eval = alpha_beta_search(next_state, depth - 1, alpha, beta, player_for_whom_to_maximize, next_state.side_to_move, nodes_searched_ref, next_history);
            
            if (eval < min_eval) {
                min_eval = eval;
                best_move_found_at_this_node = move;
            }
            beta = std::min(beta, eval);
            if (beta <= alpha) { // Alpha cutoff (fail low)
                flag_for_tt_store = TranspositionTable::EntryFlag::UPPER_BOUND;
                break; 
            }
        }
        // Refined flag determination
        if (min_eval < beta && min_eval > alpha) { // Check against original alpha passed to this node
            flag_for_tt_store = TranspositionTable::EntryFlag::EXACT_SCORE;
        } // else it remains LOWER_BOUND (if no move lowered beta below original beta) or becomes UPPER_BOUND (if alpha cutoff)

        TranspositionTable::store_tt_entry(current_hash, min_eval, depth, flag_for_tt_store, best_move_found_at_this_node);
        return min_eval;
    }
}


AiMoveResult find_best_ai_move(
    const BoardState& current_board_state, 
    int search_depth,
    const std::vector<BoardState>& game_history_ref 
) {
    AiMoveResult result; 
    Player ai_player = PLAYER_1; 

    std::vector<Move> legal_moves_generated = generate_all_legal_moves(current_board_state, ai_player, game_history_ref);
    result.root_moves_count = static_cast<int>(legal_moves_generated.size());

    if (legal_moves_generated.empty()) {
        return result; 
    }

    // --- Move Ordering Step (Static Eval + TT Best Move) ---
    std::vector<std::pair<int, Move>> scored_root_moves;
    TranspositionTable::TTEntry* root_tt_entry = TranspositionTable::probe_tt(current_board_state.zobrist_hash);
    Move tt_best_move_at_root; // Default invalid
    if (root_tt_entry != nullptr && root_tt_entry->best_move.from_sq != -1) {
        // Check if the TT best move is actually in the list of legal moves for this turn
        // (it might be from a different search depth or a slightly different history context)
        bool tt_move_is_legal = false;
        for(const auto& legal_move : legal_moves_generated) {
            if (legal_move == root_tt_entry->best_move) {
                tt_move_is_legal = true;
                break;
            }
        }
        if (tt_move_is_legal) {
            tt_best_move_at_root = root_tt_entry->best_move;
        }
    }

    for (const Move& move : legal_moves_generated) {
        BoardState temp_next_state = make_move_on_copy(current_board_state, move);
        int static_eval = evaluate_board(temp_next_state, ai_player); 
        if (move == tt_best_move_at_root) { // Check if this is the TT's preferred move
            static_eval += 1000000; // Large bonus to ensure it's sorted first
        }
        scored_root_moves.push_back({static_eval, move});
    }
    std::sort(scored_root_moves.begin(), scored_root_moves.end(), [](const auto& a, const auto& b) {
        return a.first > b.first; 
    });
    // --- End Move Ordering Step ---

    auto time_start = std::chrono::high_resolution_clock::now();
    long long total_nodes_for_search_at_root = 0; 

    int alpha = std::numeric_limits<int>::min();
    int beta = std::numeric_limits<int>::max();
    bool first_move_evaluated = false;
    // result.best_move is already default constructed to invalid

    for (const auto& scored_move_pair : scored_root_moves) { 
        const Move& move = scored_move_pair.second; 
        BoardState next_state_after_ai_move = make_move_on_copy(current_board_state, move);
        long long nodes_for_this_branch = 0; 
        
        std::vector<BoardState> history_for_branch = game_history_ref;
        history_for_branch.push_back(current_board_state); 

        int score_for_this_move = alpha_beta_search(
            next_state_after_ai_move, 
            search_depth - 1, 
            alpha, 
            beta, 
            ai_player, 
            next_state_after_ai_move.side_to_move, 
            nodes_for_this_branch,
            history_for_branch 
        );
        total_nodes_for_search_at_root += nodes_for_this_branch;
        
        if (!first_move_evaluated || score_for_this_move > result.final_score) {
            result.final_score = score_for_this_move;
            result.best_move = move;
            first_move_evaluated = true;
        }
        alpha = std::max(alpha, result.final_score); 
        // No beta cutoff at the root itself, as we want to find the true best move.
        // Alpha is updated to narrow the window for subsequent sibling root moves.
    }

    auto time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_diff_ms = time_end - time_start;
    result.time_taken_ms = time_diff_ms.count();
    result.nodes_searched = total_nodes_for_search_at_root; 

    // Store the result of the root search in TT
    if (result.best_move.from_sq != -1) {
        TranspositionTable::EntryFlag root_flag;
        // This flag determination at root is tricky. If alpha changed from initial -inf, it's at least that good.
        // If result.final_score == alpha (after loop), it could be an exact score or a score that failed high.
        // For simplicity, we often store root results as EXACT if a move was found.
        // More accurately:
        if (result.final_score <= alpha) { // Alpha was original alpha if no move improved it.
             root_flag = TranspositionTable::EntryFlag::UPPER_BOUND; // All moves were bad
        } else if (result.final_score >= beta ) { // Should not happen if beta is max_int at root
             root_flag = TranspositionTable::EntryFlag::LOWER_BOUND;
        }
        else {
             root_flag = TranspositionTable::EntryFlag::EXACT_SCORE;
        }
        TranspositionTable::store_tt_entry(current_board_state.zobrist_hash, result.final_score, search_depth, root_flag, result.best_move);
    }


    if (result.best_move.from_sq == -1 && !legal_moves_generated.empty()) {
        result.best_move = legal_moves_generated[0]; 
        if (result.final_score == std::numeric_limits<int>::min()) { 
             BoardState fallback_state = make_move_on_copy(current_board_state, result.best_move);
             result.final_score = evaluate_board(fallback_state, ai_player); 
        }
    }
    return result;
}


