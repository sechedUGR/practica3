#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
#include <algorithm>
#include <vector>
#include <cmath>


// --- Constantes Globales ---
const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int NUM_PIECES_PER_COLOR = 2;
const int PROFUNDIDAD_ALFABETA_DEFAULT = 7;
const int PROFUNDIDAD_QUIESCENCE_DEFAULT = 3; // Increased quiescence depth slightly

// --- Anonymous Namespace for Helper Structs ---
namespace {
    struct NodoOrdenado {
        Parchis hijo; // Parchis definition will be complete here from AIPlayer.cpp's includes
        float valor_heuristico_estimado;
        color c;
        int id_ficha;
        int dado_usado;
    };

    // Comparators for sorting NodoOrdenado if needed, or use lambdas directly
    bool comparaNodosParaMax(const NodoOrdenado& a, const NodoOrdenado& b) {
        return a.valor_heuristico_estimado > b.valor_heuristico_estimado;
    }
    bool comparaNodosParaMin(const NodoOrdenado& a, const NodoOrdenado& b) {
        return a.valor_heuristico_estimado < b.valor_heuristico_estimado;
    }
} // end anonymous namespace


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
    dice_val = 1;

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
    if (id_piece == SKIP_TURN) {
        c_piece = actual->getCurrentColor();
    }
}

// --- AGENTES SIMPLES (sin cambios significativos, ensure they compile) ---
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice_val) const {
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    if (current_dices.empty()) {
        current_dices = actual->getAvailableSpecialDices(player);
    }

    if (current_dices.empty()) {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
        return;
    }
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
     if (current_dices.empty()) {
        current_dices = actual->getAvailableSpecialDices(player);
    }

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
        int id_val = std::get<1>(piece_tuple); // Renamed id to id_val to avoid conflict with member id
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
    ParchisBros hijos = actual->getChildren();
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
                if(estado_hijo.getBoard().getPiece(c_eval, i).get_box().type == goal) valor += 50;
                 if(estado_hijo.getBoard().getPiece(c_eval, i).get_box().type == home) valor -= 100;
            }
        }
        return valor;
    };

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        Parchis hijo = *it;
        float valor_actual = evalua_simple(hijo, current_player_id);

        if (valor_actual > mejor_valor_heuristico) {
            mejor_valor_heuristico = valor_actual;
            c_piece = it.getMovedColor();
            id_piece = it.getMovedPieceId();
            dice_val = it.getMovedDiceValue();
            found_move = true;
        }
    }

    if (!found_move && hijos.begin() != hijos.end()) {
        // If no move strictly better than menosinf, but there are moves, pick the first one.
        // This handles cases where all moves have very negative heuristic values.
        ParchisBros::Iterator it = hijos.begin();
        c_piece = it.getMovedColor();
        id_piece = it.getMovedPieceId();
        dice_val = it.getMovedDiceValue();
        found_move = true;
    }


    if (!found_move) {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0; // Or an available die
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
            if (estado.getBoard().getPiece(c, i).get_box().type == goal) puntuacion_jugador += 100;
            else if (estado.getBoard().getPiece(c, i).get_box().type == home) puntuacion_jugador -= 50;
            puntuacion_jugador -= estado.distanceToGoal(c, i) * 0.1f;
        }
    }

    std::vector<color> colores_oponente = estado.getPlayerColors(oponente);
    for (color c : colores_oponente) {
        for (int i = 0; i < NUM_PIECES_PER_COLOR; ++i) {
            if (estado.isSafePiece(c, i)) puntuacion_oponente += 10;
            if (estado.getBoard().getPiece(c, i).get_box().type == goal) puntuacion_oponente += 100;
            else if (estado.getBoard().getPiece(c, i).get_box().type == home) puntuacion_oponente -= 50;
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

    const float IN_GOAL_BONUS = 2500.0f; // Increased
    const float AT_HOME_PENALTY = -600.0f; // Increased
    const float DISTANCE_POWER_EXP = 1.35f; // Slightly increased emphasis
    const float MAX_DIST_APPROX = 74.0f;
    const float SAFE_SQUARE_BONUS = 70.0f; // Increased
    const float NEAR_GOAL_THRESHOLD = 8.0f; // within 8 moves (more generous)
    const float NEAR_GOAL_BONUS = 120.0f; // Increased
    const float JUST_OUT_THRESHOLD_LOW = 5.0f; // How many steps from home start considered "just out"
    const float JUST_OUT_BONUS = 50.0f;
    const float TWO_AT_HOME_PENALTY_COLOR = -700.0f; // Penalty for a single color
    const float EATING_BONUS = 450.0f;
    const float GOAL_MOVE_BONUS = 300.0f; // Bonus for the state *after* a goal move
    const float BARRIER_BONUS_DEFENSIVE = 15.0f; // Small bonus for own barrier
    const float BARRIER_PENALTY_OFFENSIVE = -20.0f; // Small penalty for opponent barrier blocking me (harder to calculate generally)


    // Evaluar mis piezas (jugador_evaluando_para)
    std::vector<color> mis_colores = estado.getPlayerColors(jugador_evaluando_para);
    for (color c : mis_colores) {
        int en_casa_color = 0;
        int pieces_this_color_on_board = 0;
        for (int j = 0; j < NUM_PIECES_PER_COLOR; ++j) {
            Box box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) {
                score_jugador += IN_GOAL_BONUS;
            } else if (box.type == home) {
                score_jugador += AT_HOME_PENALTY;
                en_casa_color++;
            } else {
                pieces_this_color_on_board++;
                score_jugador += std::pow(std::max(0.0f, MAX_DIST_APPROX - dist), DISTANCE_POWER_EXP);
                if (estado.isSafePiece(c, j)) score_jugador += SAFE_SQUARE_BONUS;
                if (dist <= NEAR_GOAL_THRESHOLD && dist > 0) score_jugador += NEAR_GOAL_BONUS;

                // Check if piece is "just out" by checking distance from its starting square if that info is available
                // Or approximate: if distanceToGoal is very high (e.g. > MAX_DIST_APPROX - JUST_OUT_THRESHOLD_LOW)
                if (estado.distanceFromHome(c,j) <= JUST_OUT_THRESHOLD_LOW && estado.distanceFromHome(c,j) > 0) { // Assuming distanceFromHome exists
                     score_jugador += JUST_OUT_BONUS;
                }
            }
        }
        if (en_casa_color == NUM_PIECES_PER_COLOR) score_jugador += TWO_AT_HOME_PENALTY_COLOR;
        if (pieces_this_color_on_board == 2 && estado.arePiecesBarrier(c, 0, c, 1)) { // Crude check for barrier
            score_jugador += BARRIER_BONUS_DEFENSIVE;
        }
    }

    // Evaluar piezas del oponente
    std::vector<color> colores_oponente = estado.getPlayerColors(oponente);
    for (color c : colores_oponente) {
        int en_casa_color = 0;
        int pieces_this_color_on_board = 0;
        for (int j = 0; j < NUM_PIECES_PER_COLOR; ++j) {
            Box box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) {
                score_oponente += IN_GOAL_BONUS;
            } else if (box.type == home) {
                score_oponente += AT_HOME_PENALTY;
                en_casa_color++;
            } else {
                pieces_this_color_on_board++;
                score_oponente += std::pow(std::max(0.0f, MAX_DIST_APPROX - dist), DISTANCE_POWER_EXP);
                if (estado.isSafePiece(c, j)) score_oponente += SAFE_SQUARE_BONUS;
                if (dist <= NEAR_GOAL_THRESHOLD && dist > 0) score_oponente += NEAR_GOAL_BONUS;
                 if (estado.distanceFromHome(c,j) <= JUST_OUT_THRESHOLD_LOW && estado.distanceFromHome(c,j) > 0) {
                     score_oponente += JUST_OUT_BONUS;
                }
            }
        }
        if (en_casa_color == NUM_PIECES_PER_COLOR) score_oponente += TWO_AT_HOME_PENALTY_COLOR;
        // Not penalizing opponent barriers here directly, too complex without pathing info.
    }

    // Bonus for the *outcome* of the move that led to this state.
    // These flags are about the transition *into* the current 'estado'.
    // If 'jugador_evaluando_para' is the one whose turn it was *before* this state, these bonuses apply to them.
    // This is hard to use correctly in a general heuristic `H(s, player_to_max)` without knowing whose move led to 's'.
    // However, the problem states "isEatingMove" as a property of the state itself.
    // Let's assume if these are true, it's a generally "active" and potentially good state for the player who made the move.
    // This part is often better handled by evaluating children or in quiescence.
    // The tutorial (ValoracionTest) applies these to the overall score_jugador - score_oponente.
    // The game rules state the player *who eats* gets +20 (handled by game, not heuristic directly),
    // and *who reaches goal* gets +10.
    // The heuristic evaluates the static board.
    // For now, these global state flags will be omitted from this static heuristic to avoid misattribution,
    // relying on Quiescence to explore captures/goals.

    return score_jugador - score_oponente;
}


// --- PODA ALFA-BETA ---
float AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
                               color &c_piece_out, int &id_piece_out, int &dice_out,
                               float alpha, float beta, Heuristic *heuristic, bool quiescence_enabled) const {

    if (NodeCounter::isLimitReached()) { // Check at the very beginning of the call
        return heuristic->evaluate(estado, jugador_maximizer); // Return a basic evaluation if limit hit
    }

    if (profundidad == profundidad_max || estado.gameOver()) {
        if (quiescence_enabled && !estado.gameOver()) {
             return QuiescenceSearch(estado, jugador_maximizer, 0, PROFUNDIDAD_QUIESCENCE_DEFAULT, alpha, beta, heuristic);
        }
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    std::vector<NodoOrdenado> hijos_ordenados;
    ParchisBros hijos_iterable = estado.getChildren();
    for (ParchisBros::Iterator it = hijos_iterable.begin(); it != hijos_iterable.end(); ++it) {
        if (NodeCounter::isLimitReached()) break;
        Parchis hijo_actual = *it; // Creates a Parchis object
        // Pass hijo_actual by const reference if possible to heuristic, but evaluate might modify a counter or take by value.
        // Assuming heuristic->evaluate takes const Parchis&.
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

    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX Node
        float mejor_valor_nodo = menosinf;
        color current_best_c = c_piece_out;
        int current_best_id = id_piece_out;
        int current_best_d = dice_out;
        bool root_move_updated = false;

        if (profundidad == 0 && !hijos_ordenados.empty()) { // Initialize with first move at root if no skip
            current_best_c = hijos_ordenados.front().c;
            current_best_id = hijos_ordenados.front().id_ficha;
            current_best_d = hijos_ordenados.front().dado_usado;
        }


        for (const auto& nodo_hijo_ordenado : hijos_ordenados) {
            if (NodeCounter::isLimitReached()) {
                if (profundidad == 0 && root_move_updated) {
                     c_piece_out = current_best_c;
                     id_piece_out = current_best_id;
                     dice_out = current_best_d;
                } else if (profundidad == 0 && !hijos_ordenados.empty() && !root_move_updated) {
                    // If limit hit before any recursive call at root, use the first sorted move
                    c_piece_out = hijos_ordenados.front().c;
                    id_piece_out = hijos_ordenados.front().id_ficha;
                    dice_out = hijos_ordenados.front().dado_usado;
                }
                return mejor_valor_nodo;
            }

            color c_dummy; int id_dummy; int dice_dummy;
            float valor_hijo = Poda_AlfaBeta(nodo_hijo_ordenado.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                             c_dummy, id_dummy, dice_dummy, alpha, beta, heuristic, quiescence_enabled);

            if (valor_hijo > mejor_valor_nodo) {
                mejor_valor_nodo = valor_hijo;
                if (profundidad == 0) {
                    current_best_c = nodo_hijo_ordenado.c;
                    current_best_id = nodo_hijo_ordenado.id_ficha;
                    current_best_d = nodo_hijo_ordenado.dado_usado;
                    root_move_updated = true;
                }
            }
            alpha = std::max(alpha, mejor_valor_nodo);
            if (beta <= alpha) {
                break;
            }
        }
        if (profundidad == 0) {
             c_piece_out = current_best_c;
             id_piece_out = current_best_id;
             dice_out = current_best_d;
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

            peor_valor_nodo = std::min(peor_valor_nodo, valor_hijo);
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

    if (NodeCounter::isLimitReached() || profundidad_quiescence == profundidad_max_quiescence || estado.gameOver()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    float stand_pat_score = heuristic->evaluate(estado, jugador_maximizer);

    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX Node
        alpha = std::max(alpha, stand_pat_score);
        if (stand_pat_score >= beta) {
            return stand_pat_score;
        }
    }
    else { // MIN Node
        beta = std::min(beta, stand_pat_score);
        if (stand_pat_score <= alpha) {
            return stand_pat_score;
        }
    }

    std::vector<NodoOrdenado> tactical_moves;
    ParchisBros todos_hijos = estado.getChildren();
    for (ParchisBros::Iterator it = todos_hijos.begin(); it != todos_hijos.end(); ++it) {
        if (NodeCounter::isLimitReached()) break;
        Parchis hijo = *it;
        bool es_tactico = false;
        if (hijo.isEatingMove()) es_tactico = true;
        if (hijo.isGoalMove()) es_tactico = true;
        // Consider moves that change the 'stand_pat_score' significantly?
        // float hijo_eval = heuristic->evaluate(hijo, jugador_maximizer);
        // if (abs(hijo_eval - stand_pat_score) > SOME_THRESHOLD) es_tactico = true;


        if (es_tactico) {
            tactical_moves.push_back({hijo, heuristic->evaluate(hijo, jugador_maximizer), it.getMovedColor(), it.getMovedPieceId(), it.getMovedDiceValue()});
        }
    }

    if (tactical_moves.empty() && profundidad_quiescence > 0) { // No tactical moves to explore further from a non-root quiescence call
        return stand_pat_score;
    }


    if (estado.getCurrentPlayerId() == jugador_maximizer) {
        std::sort(tactical_moves.begin(), tactical_moves.end(), comparaNodosParaMax);
    } else {
        std::sort(tactical_moves.begin(), tactical_moves.end(), comparaNodosParaMin);
    }


    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX Node
        float mejor_valor_quiesc = stand_pat_score;
        for (const auto& tactical_move_node : tactical_moves) {
            if (NodeCounter::isLimitReached()) break;
//a
            float valor_hijo = QuiescenceSearch(tactical_move_node.hijo, jugador_maximizer, profundidad_quiescence + 1, profundidad_max_quiescence, alpha, beta, heuristic);
            mejor_valor_quiesc = std::max(mejor_valor_quiesc, valor_hijo);
            alpha = std::max(alpha, mejor_valor_quiesc);
            if (beta <= alpha) {
                break;
            }
        }
        return mejor_valor_quiesc;
    } else { // MIN Node
        float peor_valor_quiesc = stand_pat_score;
        for (const auto& tactical_move_node : tactical_moves) {
            if (NodeCounter::isLimitReached()) break;

            float valor_hijo = QuiescenceSearch(tactical_move_node.hijo, jugador_maximizer, profundidad_quiescence + 1, profundidad_max_quiescence, alpha, beta, heuristic);
            peor_valor_quiesc = std::min(peor_valor_quiesc, valor_hijo);
            beta = std::min(beta, peor_valor_quiesc);
            if (beta <= alpha) {
                break;
            }
        }
        return peor_valor_quiesc;
    }
}