#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
// #include "../../include/model/ParchisBros.h" // REMOVED as per previous fix
#include <vector>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <iostream> // For debugging cout, if needed

// --- Constantes Globales ---
const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int NUM_PIECES_PER_COLOR = 2;
const int PROFUNDIDAD_ALFABETA_DEFAULT = 7;
const int PROFUNDIDAD_QUIESCENCE_DEFAULT = 3;

// --- Anonymous Namespace for Helper Structs ---
namespace {
    struct NodoOrdenado {
        Parchis hijo;
        float valor_heuristico_estimado;
        color c;
        int id_ficha;
        int dado_usado;
    };

    bool comparaNodosParaMax(const NodoOrdenado& a, const NodoOrdenado& b) {
        return a.valor_heuristico_estimado > b.valor_heuristico_estimado;
    }
    bool comparaNodosParaMin(const NodoOrdenado& a, const NodoOrdenado& b) {
        return a.valor_heuristico_estimado < b.valor_heuristico_estimado;
    }
}


// --- Implementación de AIPlayer ---
bool AIPlayer::move() {
    color c_piece;
    int id_piece;
    int dice_val;
    think(c_piece, id_piece, dice_val);

    actual->movePiece(c_piece, id_piece, dice_val);
    return true;
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice_val) const {
    float valor_alpha_beta = 0;
    float alpha = menosinf;
    float beta = masinf;
    int jugador_actual_id = actual->getCurrentPlayerId();

    ValoracionTest valoracionTest;
    ValoracionAvanzada valoracionAvanzada;

    id_piece = SKIP_TURN;
    c_piece = actual->getCurrentColor();
    dice_val = 0; // Default for SKIP_TURN, Poda_AlfaBeta should fill if a move is found

    switch (id) {
        case 0:
            valor_alpha_beta = Poda_AlfaBeta(*actual, jugador_actual_id, 0, PROFUNDIDAD_ALFABETA_DEFAULT, c_piece, id_piece, dice_val, alpha, beta, &valoracionTest, false);
            break;
        case 1:
            valor_alpha_beta = Poda_AlfaBeta(*actual, jugador_actual_id, 0, PROFUNDIDAD_ALFABETA_DEFAULT, c_piece, id_piece, dice_val, alpha, beta, &valoracionAvanzada, true);
            break;
        case 2:
            thinkMejorOpcion(c_piece, id_piece, dice_val);
            break;
        case 3:
            thinkFichaMasAdelantada(c_piece, id_piece, dice_val);
            break;
        default:
            thinkAleatorio(c_piece, id_piece, dice_val);
            break;
    }
    if (id_piece == SKIP_TURN) { // Ensure color is correct if skipping
        c_piece = actual->getCurrentColor();
    }
}

// --- AGENTES SIMPLES ---
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice_val) const {
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    // REMOVED: Call to getAvailableSpecialDices
    // if (current_dices.empty()) {
    //     current_dices = actual->getAvailableSpecialDices(player);
    // }

    if (current_dices.empty()) {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
        return;
    }
    // It's possible rand() % 0 if current_dices.size() is 0, guard against it (already done by previous check)
    dice_val = current_dices[rand() % current_dices.size()];

    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice_val);

    if (!current_pieces.empty()) {
        int random_id = rand() % current_pieces.size();
        id_piece = std::get<1>(current_pieces[random_id]);
        c_piece = std::get<0>(current_pieces[random_id]);
    } else {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    }
}

void AIPlayer::thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice_val) const {
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    // REMOVED: Call to getAvailableSpecialDices
    //  if (current_dices.empty()) {
    //     current_dices = actual->getAvailableSpecialDices(player);
    // }

    if (current_dices.empty()) {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
        return;
    }
    dice_val = current_dices[rand() % current_dices.size()];

    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice_val);

    int id_ficha_mas_adelantada = -1;
    color col_ficha_mas_adelantada = none;
    int min_distancia_meta = 99999;

    for (const auto& piece_tuple : current_pieces) {
        color col = std::get<0>(piece_tuple);
        int id_val = std::get<1>(piece_tuple);
        int distancia_meta = actual->distanceToGoal(col, id_val);
        if (distancia_meta < min_distancia_meta) {
            min_distancia_meta = distancia_meta;
            id_ficha_mas_adelantada = id_val;
            col_ficha_mas_adelantada = col;
        }
    }

    if (id_ficha_mas_adelantada != -1) {
        id_piece = id_ficha_mas_adelantada;
        c_piece = col_ficha_mas_adelantada;
    } else {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    }
}

void AIPlayer::thinkMejorOpcion(color &c_piece, int &id_piece, int &dice_val) const {
    ParchisBros hijos = actual->getChildren(); // ParchisBros comes from Parchis.h
    bool found_move = false;
    float mejor_valor_heuristico = menosinf;
    int current_player_id = actual->getCurrentPlayerId();

    auto evalua_simple = [&](const Parchis& estado_hijo, int jugador_perspectiva) {
        float valor = 0;
        if (estado_hijo.isEatingMove()) valor += 200;
        if (estado_hijo.isGoalMove()) valor += 100;

        std::vector<color> mis_colores = estado_hijo.getPlayerColors(jugador_perspectiva);
        for (color c_eval : mis_colores) {
            for (int i = 0; i < NUM_PIECES_PER_COLOR; ++i) {
                valor -= estado_hijo.distanceToGoal(c_eval, i);
                if(estado_hijo.isSafePiece(c_eval, i)) valor += 5;
                Box piece_box = estado_hijo.getBoard().getPiece(c_eval, i).get_box(); // Get box once
                if(piece_box.type == goal) valor += 50;
                else if(piece_box.type == home) valor -= 100;
            }
        }
        return valor;
    };

    ParchisBros::Iterator best_it = hijos.end();

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        Parchis hijo = *it;
        float valor_actual = evalua_simple(hijo, current_player_id);

        if (valor_actual > mejor_valor_heuristico) {
            mejor_valor_heuristico = valor_actual;
            best_it = it;
            found_move = true;
        }
    }

    if (found_move) {
        c_piece = best_it.getMovedColor();
        id_piece = best_it.getMovedPieceId();
        dice_val = best_it.getMovedDiceValue();
    } else if (hijos.begin() != hijos.end()) {
        ParchisBros::Iterator it = hijos.begin();
        c_piece = it.getMovedColor();
        id_piece = it.getMovedPieceId();
        dice_val = it.getMovedDiceValue();
    } else {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
    }
}


// --- HEURÍSTICA DE PRUEBA (ValoracionTest) ---
float ValoracionTest::getHeuristic(const Parchis& estado, int jugador_evaluando_para) const {
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;

    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    float puntuacion_jugador = 0;
    float puntuacion_oponente = 0;

    std::vector<color> mis_colores = estado.getPlayerColors(jugador_evaluando_para);
    for (color c : mis_colores) {
        for (int i = 0; i < NUM_PIECES_PER_COLOR; ++i) {
            if (estado.isSafePiece(c, i)) puntuacion_jugador += 10;
            Box piece_box = estado.getBoard().getPiece(c, i).get_box();
            if (piece_box.type == goal) puntuacion_jugador += 100;
            else if (piece_box.type == home) puntuacion_jugador -= 50;
            puntuacion_jugador -= estado.distanceToGoal(c, i) * 0.1f;
        }
    }

    std::vector<color> colores_oponente = estado.getPlayerColors(oponente);
    for (color c : colores_oponente) {
        for (int i = 0; i < NUM_PIECES_PER_COLOR; ++i) {
            if (estado.isSafePiece(c, i)) puntuacion_oponente += 10;
            Box piece_box = estado.getBoard().getPiece(c, i).get_box();
            if (piece_box.type == goal) puntuacion_oponente += 100;
            else if (piece_box.type == home) puntuacion_oponente -= 50;
            puntuacion_oponente -= estado.distanceToGoal(c, i) * 0.1f;
        }
    }
    return puntuacion_jugador - puntuacion_oponente;
}

// --- HEURÍSTICA AVANZADA ---
float ValoracionAvanzada::getHeuristic(const Parchis &estado, int jugador_evaluando_para) const {
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;

    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    float score_jugador = 0;
    float score_oponente = 0;

    const float IN_GOAL_BONUS = 2500.0f;
    const float AT_HOME_PENALTY = -600.0f;
    const float DISTANCE_POWER_EXP = 1.35f;
    const float MAX_DIST_APPROX = 74.0f;
    const float SAFE_SQUARE_BONUS = 70.0f;
    const float NEAR_GOAL_THRESHOLD = 8.0f;
    const float NEAR_GOAL_BONUS = 120.0f;
    const float JUST_OUT_DIST_THRESHOLD = 65.0f; // If dist_to_goal is >= this, piece is "far"/just out
    const float JUST_OUT_BONUS = 50.0f;
    const float TWO_AT_HOME_PENALTY_COLOR = -700.0f;
    const float BARRIER_BONUS_DEFENSIVE = 25.0f; // Increased slightly


    // Evaluar mis piezas (jugador_evaluando_para)
    std::vector<color> mis_colores = estado.getPlayerColors(jugador_evaluando_para);
    for (color c : mis_colores) {
        int en_casa_color = 0;
        Box box_piece0 = estado.getBoard().getPiece(c, 0).get_box();
        Box box_piece1 = estado.getBoard().getPiece(c, 1).get_box();

        for (int j = 0; j < NUM_PIECES_PER_COLOR; ++j) {
            Box box = estado.getBoard().getPiece(c, j).get_box(); // Use the specific piece's box
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) {
                score_jugador += IN_GOAL_BONUS;
            } else if (box.type == home) {
                score_jugador += AT_HOME_PENALTY;
                en_casa_color++;
            } else {
                score_jugador += std::pow(std::max(0.0f, MAX_DIST_APPROX - dist), DISTANCE_POWER_EXP);
                if (estado.isSafePiece(c, j)) score_jugador += SAFE_SQUARE_BONUS;
                if (dist <= NEAR_GOAL_THRESHOLD && dist > 0) score_jugador += NEAR_GOAL_BONUS;

                // REPLACED: distanceFromHome with approximation using distanceToGoal
                if (dist >= JUST_OUT_DIST_THRESHOLD) {
                     score_jugador += JUST_OUT_BONUS;
                }
            }
        }
        if (en_casa_color == NUM_PIECES_PER_COLOR) score_jugador += TWO_AT_HOME_PENALTY_COLOR;

        // REPLACED: arePiecesBarrier with manual check
        if (box_piece0.type != home && box_piece0.type != goal &&
            box_piece0.type == box_piece1.type &&
            box_piece0.num == box_piece1.num &&
            box_piece0.color_sq == box_piece1.color_sq) {
            score_jugador += BARRIER_BONUS_DEFENSIVE;
        }
    }

    // Evaluar piezas del oponente
    std::vector<color> colores_oponente = estado.getPlayerColors(oponente);
    for (color c : colores_oponente) {
        int en_casa_color = 0;
        // Not checking opponent barriers for now, can be added if needed
        for (int j = 0; j < NUM_PIECES_PER_COLOR; ++j) {
            Box box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) {
                score_oponente += IN_GOAL_BONUS;
            } else if (box.type == home) {
                score_oponente += AT_HOME_PENALTY;
                en_casa_color++;
            } else {
                score_oponente += std::pow(std::max(0.0f, MAX_DIST_APPROX - dist), DISTANCE_POWER_EXP);
                if (estado.isSafePiece(c, j)) score_oponente += SAFE_SQUARE_BONUS;
                if (dist <= NEAR_GOAL_THRESHOLD && dist > 0) score_oponente += NEAR_GOAL_BONUS;
                // REPLACED: distanceFromHome
                 if (dist >= JUST_OUT_DIST_THRESHOLD) {
                     score_oponente += JUST_OUT_BONUS;
                }
            }
        }
        if (en_casa_color == NUM_PIECES_PER_COLOR) score_oponente += TWO_AT_HOME_PENALTY_COLOR;
    }

    return score_jugador - score_oponente;
}


// --- PODA ALFA-BETA ---
float AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
                               color &c_piece_out, int &id_piece_out, int &dice_out,
                               float alpha, float beta, Heuristic *heuristic, bool quiescence_enabled) const {

    if (NodeCounter::isLimitReached()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    if (profundidad == profundidad_max || estado.gameOver()) {
        if (quiescence_enabled && !estado.gameOver()) {
             return QuiescenceSearch(estado, jugador_maximizer, 0, PROFUNDIDAD_QUIESCENCE_DEFAULT, alpha, beta, heuristic);
        }
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    std::vector<NodoOrdenado> hijos_ordenados;
    ParchisBros hijos_iterable = estado.getChildren(); // ParchisBros from Parchis.h
    for (ParchisBros::Iterator it = hijos_iterable.begin(); it != hijos_iterable.end(); ++it) {
        if (NodeCounter::isLimitReached()) break;
        Parchis hijo_actual = *it;
        hijos_ordenados.push_back({hijo_actual, heuristic->evaluate(hijo_actual, jugador_maximizer), it.getMovedColor(), it.getMovedPieceId(), it.getMovedDiceValue()});
    }

    if (hijos_ordenados.empty()) {
        if (profundidad == 0) {
            c_piece_out = estado.getCurrentColor();
            id_piece_out = SKIP_TURN;
            dice_out = 0;
        }
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    if (estado.getCurrentPlayerId() == jugador_maximizer) {
        std::sort(hijos_ordenados.begin(), hijos_ordenados.end(), comparaNodosParaMax);
    } else {
        std::sort(hijos_ordenados.begin(), hijos_ordenados.end(), comparaNodosParaMin);
    }

    // Initialize best move for root from the first available child, in case all children are bad or limit is hit early.
    // This ensures c_piece_out, id_piece_out, dice_out are set to *something* if moves exist.
    if (profundidad == 0 && !hijos_ordenados.empty()) {
        c_piece_out = hijos_ordenados.front().c;
        id_piece_out = hijos_ordenados.front().id_ficha;
        dice_out = hijos_ordenados.front().dado_usado;
    }


    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX Node
        float mejor_valor_nodo = menosinf;
        // Best move variables are c_piece_out, id_piece_out, dice_out (passed by ref)

        for (const auto& nodo_hijo_ordenado : hijos_ordenados) {
            if (NodeCounter::isLimitReached()) {
                 // c_piece_out etc. would have been set by the best move found so far at root,
                 // or by the initialization from the first child above.
                return mejor_valor_nodo;
            }

            color c_dummy; int id_dummy; int dice_dummy;
            float valor_hijo = Poda_AlfaBeta(nodo_hijo_ordenado.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                             c_dummy, id_dummy, dice_dummy, alpha, beta, heuristic, quiescence_enabled);

            if (valor_hijo > mejor_valor_nodo) {
                mejor_valor_nodo = valor_hijo;
                if (profundidad == 0) {
                    c_piece_out = nodo_hijo_ordenado.c;
                    id_piece_out = nodo_hijo_ordenado.id_ficha;
                    dice_out = nodo_hijo_ordenado.dado_usado;
                }
            }
            alpha = std::max(alpha, mejor_valor_nodo);
            if (beta <= alpha) {
                break;
            }
        }
        return mejor_valor_nodo;
    }
    else { // MIN Node
        float peor_valor_nodo = masinf;
        for (const auto& nodo_hijo_ordenado : hijos_ordenados) {
            if (NodeCounter::isLimitReached()) {
                return heuristic->evaluate(estado, jugador_maximizer);
            }

            color c_dummy; int id_dummy; int dice_dummy;
            float valor_hijo = Poda_AlfaBeta(nodo_hijo_ordenado.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                             c_dummy, id_dummy, dice_dummy, alpha, beta, heuristic, quiescence_enabled);

            peor_valor_nodo = std::min(peor_valor_nodo, valor_hijo); // No c_piece_out update here.
            beta = std::min(beta, peor_valor_nodo);
            if (beta <= alpha) {
                break;
            }
        }
        return peor_valor_nodo;
    }
}

float AIPlayer::QuiescenceSearch(const Parchis &estado, int jugador_maximizer, int profundidad_quiescence, int profundidad_max_quiescence,
                                  float alpha, float beta, Heuristic* heuristic) const {

    if (NodeCounter::isLimitReached() || profundidad_quiescence >= profundidad_max_quiescence || estado.gameOver()) { // Use >= for depth check
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    float stand_pat_score = heuristic->evaluate(estado, jugador_maximizer);

    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX Node
        alpha = std::max(alpha, stand_pat_score);
        if (stand_pat_score >= beta) { // Beta cut-off
            return stand_pat_score;
        }
    }
    else { // MIN Node
        beta = std::min(beta, stand_pat_score);
        if (stand_pat_score <= alpha) { // Alpha cut-off
            return stand_pat_score;
        }
    }

    std::vector<NodoOrdenado> tactical_moves;
    ParchisBros todos_hijos = estado.getChildren(); // ParchisBros from Parchis.h
    for (ParchisBros::Iterator it = todos_hijos.begin(); it != todos_hijos.end(); ++it) {
        if (NodeCounter::isLimitReached()) break;
        Parchis hijo = *it;
        bool es_tactico = false;
        if (hijo.isEatingMove()) es_tactico = true;
        if (hijo.isGoalMove()) es_tactico = true;
        // Add more tactical conditions if needed (e.g., escaping capture, critical barrier moves)

        if (es_tactico) {
            tactical_moves.push_back({hijo, heuristic->evaluate(hijo, jugador_maximizer), it.getMovedColor(), it.getMovedPieceId(), it.getMovedDiceValue()});
        }
    }

    // If no tactical moves, return the stand_pat_score, unless it's the first call (depth 0)
    // to ensure at least one level of normal search happens before quiescence decides it's quiet.
    // The condition `profundidad_quiescence > 0` was in prev. version, let's see.
    // If tactical_moves is empty here, the loops below won't run, and stand_pat_score (updated by alpha/beta) will be returned.

    if (estado.getCurrentPlayerId() == jugador_maximizer) {
        std::sort(tactical_moves.begin(), tactical_moves.end(), comparaNodosParaMax);
    } else {
        std::sort(tactical_moves.begin(), tactical_moves.end(), comparaNodosParaMin);
    }

    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX Node
        float current_max_score = stand_pat_score;
        for (const auto& tactical_move_node : tactical_moves) {
            if (NodeCounter::isLimitReached()) break;

            float valor_hijo = QuiescenceSearch(tactical_move_node.hijo, jugador_maximizer, profundidad_quiescence + 1, profundidad_max_quiescence, alpha, beta, heuristic);
            current_max_score = std::max(current_max_score, valor_hijo);
            alpha = std::max(alpha, current_max_score);
            if (beta <= alpha) {
                break;
            }
        }
        return current_max_score;
    } else { // MIN Node
        float current_min_score = stand_pat_score;
        for (const auto& tactical_move_node : tactical_moves) {
            if (NodeCounter::isLimitReached()) break;

            float valor_hijo = QuiescenceSearch(tactical_move_node.hijo, jugador_maximizer, profundidad_quiescence + 1, profundidad_max_quiescence, alpha, beta, heuristic);
            current_min_score = std::min(current_min_score, valor_hijo);
            beta = std::min(beta, current_min_score);
            if (beta <= alpha) {
                break;
            }
        }
        return current_min_score;
    }
}