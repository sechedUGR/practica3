#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
#include <algorithm> // For std::sort, std::max, std::min
#include <vector>
#include <cmath>     // For std::pow
#include <tuple>     // For std::tuple in simple agents (implicitly included by other headers usually)
#include <cstdlib>   // For rand() - included once

// --- Constantes Globales ---
const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int num_pieces = 2;
const int PROFUNDIDAD_ALFABETA = 7; // You can experiment with increasing this to 8 or 9

bool AIPlayer::move() {
    color c_piece;
    int id_piece;
    int dice_val; // Renamed to avoid conflict with Poda_AlfaBeta parameter
    think(c_piece, id_piece, dice_val);
    actual->movePiece(c_piece, id_piece, dice_val);
    return true;
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice to dice_val
    float valor_heuristic = 0;
    float alpha = menosinf, beta = masinf;
    int jugador_maximizer_root = actual->getCurrentPlayerId();

    ValoracionTest valoracionTest;
    ValoracionAvanzada valoracionAvanzada;

    id_piece = SKIP_TURN;
    c_piece = actual->getCurrentColor();
    dice_val = 0;

    switch(id){
        case 0:
            valor_heuristic = Poda_AlfaBeta(*actual, jugador_maximizer_root, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice_val, alpha, beta, &valoracionTest);
            break;
        case 1:
            valor_heuristic = Poda_AlfaBeta(*actual, jugador_maximizer_root, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice_val, alpha, beta, &valoracionAvanzada);
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
    // (void)valor_heuristic; // Suppress unused variable warning if value isn't used
}

// --- AGENTES SIMPLES PARA TESTEAR (Minor corrections) ---
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    if (current_dices.empty()) { // If no normal dice, check special (as per your working code)
        current_dices = actual->getAvailableSpecialDices(player);
    }

    if (current_dices.empty()) { // If still no dice
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
        return;
    }
    dice_val = current_dices[rand() % current_dices.size()]; // Safe due to check above

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

void AIPlayer::thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice
    // In your 3/6 version, this called thinkAleatorio first, then overrode piece choice.
    // Let's keep that structure if it was working.
    // First, get a dice roll (randomly, as in your thinkAleatorio part)
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
    dice_val = current_dices[rand() % current_dices.size()]; // Dice chosen

    // Now, for this chosen dice, find the most advanced piece
    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice_val);

    int id_ficha_mas_adelantada = -1;
    color col_ficha_mas_adelantada = none;
    int min_distancia_meta = 99999; // Large number

    for (const auto& piece_tuple : current_pieces) { // Use range-based for
        color col = std::get<0>(piece_tuple);
        int id_curr = std::get<1>(piece_tuple); // Renamed to avoid conflict
        int distancia_meta = actual->distanceToGoal(col, id_curr);
        if (distancia_meta < min_distancia_meta) {
            min_distancia_meta = distancia_meta;
            id_ficha_mas_adelantada = id_curr;
            col_ficha_mas_adelantada = col;
        }
    }

    if (id_ficha_mas_adelantada != -1) {
        id_piece = id_ficha_mas_adelantada;
        c_piece = col_ficha_mas_adelantada;
        // dice_val is already set
    } else { // No movable piece found for the chosen die
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        // dice_val can remain as chosen, game will handle SKIP_TURN
    }
}

void AIPlayer::thinkMejorOpcion(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice
    ParchisBros hijos = actual->getChildren();

    float mejor_valor = menosinf; // Use menosinf
    color mejor_c = none;
    int mejor_id = -1;
    int mejor_dado = -1;

    const float PESO_META_MCP = 2000.0f; // Renamed for clarity
    const float PESO_CASA_MCP = -500.0f;
    const float PESO_DISTANCIA_MCP = -25.0f;
    const float PESO_COMER_MCP = 900.0f;
    const float BONUS_POR_SEGURO_MCP = 10.0f;

    int jugador_perspectiva = actual->getCurrentPlayerId();
    int oponente_perspectiva = (jugador_perspectiva + 1) % 2; // Define opponent
    bool move_selected = false;

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        const Parchis& hijo = *it; // Use const&
        float valor = 0;

        std::vector<color> my_colors = hijo.getPlayerColors(jugador_perspectiva);
        for (color c : my_colors) {
            for (int j = 0; j < num_pieces; j++) {
                const Box& box = hijo.getBoard().getPiece(c, j).get_box();
                if (box.type == goal) valor += PESO_META_MCP;
                else if (box.type == home) valor += PESO_CASA_MCP;
                else valor += PESO_DISTANCIA_MCP * hijo.distanceToGoal(c, j);
                if (hijo.isSafePiece(c, j)) valor += BONUS_POR_SEGURO_MCP;
            }
        }
        // Symmetric evaluation for opponent
        std::vector<color> op_colors = hijo.getPlayerColors(oponente_perspectiva);
        for (color c : op_colors) {
            for (int j = 0; j < num_pieces; j++) {
                const Box& box = hijo.getBoard().getPiece(c, j).get_box();
                if (box.type == goal) valor -= PESO_META_MCP; // Subtract opponent's score
                else if (box.type == home) valor -= PESO_CASA_MCP;
                else valor -= PESO_DISTANCIA_MCP * hijo.distanceToGoal(c, j);
                if (hijo.isSafePiece(c, j)) valor -= BONUS_POR_SEGURO_MCP;
            }
        }

        if (hijo.isEatingMove()) valor += PESO_COMER_MCP;
        if (hijo.isGoalMove()) valor += PESO_META_MCP / 2.0f; // Fractional bonus

        if (valor > mejor_valor) {
            mejor_valor = valor;
            mejor_c = it.getMovedColor();
            mejor_id = it.getMovedPieceId();
            mejor_dado = it.getMovedDiceValue();
            move_selected = true;
        }
    }

    if (move_selected) {
        c_piece = mejor_c;
        id_piece = mejor_id;
        dice_val = mejor_dado;
    } else if (hijos.begin() != hijos.end()){ // Fallback to first available move
        ParchisBros::Iterator it = hijos.begin();
        c_piece = it.getMovedColor();
        id_piece = it.getMovedPieceId();
        dice_val = it.getMovedDiceValue();
    }
    else { // No moves at all
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
    }
}

// --- HEURÍSTICA BÁSICA (ValoracionTest - from your 3/6 version) ---
float ValoracionTest::getHeuristic(const Parchis& estado, int jugador) const {
    int ganador = estado.getWinner();
    int oponente = (jugador + 1) % 2;
    if (ganador == jugador) return gana;
    if (ganador == oponente) return pierde;

    std::vector<color> my_colors = estado.getPlayerColors(jugador);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);
    int puntuacion_jugador = 0, puntuacion_oponente = 0;

    for (color c : my_colors) { // Use range-based for
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            if (estado.isSafePiece(c, j)) puntuacion_jugador++;
            else if (box.type == goal) puntuacion_jugador += 5;
            puntuacion_jugador -= estado.distanceToGoal(c, j) * 0.05f; // Added small distance factor
        }
    }
    for (color c : op_colors) { // Use range-based for
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            if (estado.isSafePiece(c, j)) puntuacion_oponente++;
            else if (box.type == goal) puntuacion_oponente += 5;
            puntuacion_oponente -= estado.distanceToGoal(c, j) * 0.05f;
        }
    }
    return puntuacion_jugador - puntuacion_oponente;
}

// --- HEURÍSTICA AVANZADA (ValoracionAvanzada - based on your 3/6 version, with barrier bonus) ---
float ValoracionAvanzada::getHeuristic(const Parchis &estado, int jugador_evaluando_para) const { // Renamed jugador
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;

    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    float score_jugador = 0, score_oponente = 0;

    // Coefficients from your 3/6 version, slightly tuned and named
    const float IN_GOAL_VAL_ADV = 2000.0f; // Your value
    const float AT_HOME_PENALTY_VAL_ADV = -250.0f; // Your value
    const float DISTANCE_BASE_VAL_ADV = 74.0f;
    const float DISTANCE_EXP_VAL_ADV = 1.2f; // Your exponent
    const float SAFE_SQUARE_BONUS_VAL_ADV = 50.0f; // Your value
    const float NEAR_GOAL_DIST_VAL_ADV = 6.0f; // Your value
    const float NEAR_GOAL_BONUS_VAL_ADV = 30.0f; // Your value
    const float JUST_OUT_DIST_MIN_VAL_ADV = 65.0f; // Your value
    const float JUST_OUT_BONUS_VAL_ADV = 20.0f; // Your value
    const float TWO_AT_HOME_COLOR_PENALTY_VAL_ADV = -400.0f; // Your value
    const float EATING_MOVE_BONUS_VAL_ADV = 400.0f; // Your value
    const float GOAL_MOVE_BONUS_VAL_ADV = 500.0f;   // Your value
    const float BARRIER_BONUS_HEUR_ADV = 60.0f; // Added barrier bonus, tune this!

    std::vector<color> my_colors = estado.getPlayerColors(jugador_evaluando_para);
    for (color c : my_colors) {
        int en_casa = 0;
        const Box& box0 = estado.getBoard().getPiece(c, 0).get_box();
        const Box& box1 = estado.getBoard().getPiece(c, 1).get_box();

        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) score_jugador += IN_GOAL_VAL_ADV;
            else if (box.type == home) {
                score_jugador += AT_HOME_PENALTY_VAL_ADV;
                en_casa++;
            } else {
                score_jugador += std::pow(std::max(0.0f, DISTANCE_BASE_VAL_ADV - dist), DISTANCE_EXP_VAL_ADV);
                if (estado.isSafePiece(c, j)) score_jugador += SAFE_SQUARE_BONUS_VAL_ADV;
                if (dist <= NEAR_GOAL_DIST_VAL_ADV && dist > 0) score_jugador += NEAR_GOAL_BONUS_VAL_ADV;
                if (dist >= JUST_OUT_DIST_MIN_VAL_ADV) score_jugador += JUST_OUT_BONUS_VAL_ADV;
            }
        }
        if (en_casa == num_pieces) score_jugador += TWO_AT_HOME_COLOR_PENALTY_VAL_ADV;
        // Barrier check
        if (box0.type != home && box0.type != goal && box0.type == box1.type && box0.num == box1.num) {
            score_jugador += BARRIER_BONUS_HEUR_ADV;
        }
    }

    std::vector<color> op_colors = estado.getPlayerColors(oponente);
    for (color c : op_colors) {
        int en_casa = 0;
        // Not adding opponent barrier check yet, to keep it simple
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) score_oponente += IN_GOAL_VAL_ADV;
            else if (box.type == home) {
                score_oponente += AT_HOME_PENALTY_VAL_ADV;
                en_casa++;
            } else {
                score_oponente += std::pow(std::max(0.0f, DISTANCE_BASE_VAL_ADV - dist), DISTANCE_EXP_VAL_ADV);
                if (estado.isSafePiece(c, j)) score_oponente += SAFE_SQUARE_BONUS_VAL_ADV;
                if (dist <= NEAR_GOAL_DIST_VAL_ADV && dist > 0) score_oponente += NEAR_GOAL_BONUS_VAL_ADV;
                if (dist >= JUST_OUT_DIST_MIN_VAL_ADV) score_oponente += JUST_OUT_BONUS_VAL_ADV;
            }
        }
        if (en_casa == num_pieces) score_oponente += TWO_AT_HOME_COLOR_PENALTY_VAL_ADV;
    }

    float event_score_boost = 0;
    if (estado.isEatingMove()) event_score_boost += EATING_MOVE_BONUS_VAL_ADV;
    if (estado.isGoalMove()) event_score_boost += GOAL_MOVE_BONUS_VAL_ADV;

    return (score_jugador + event_score_boost) - score_oponente;
}


// --- PODA ALFA-BETA (Corrected and Enhanced from your 3/6 version) ---
float AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
    color &c_piece_out, int &id_piece_out, int &dice_out, float alpha, float beta, Heuristic *heuristic) const
{
    if (NodeCounter::isLimitReached()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }
    if (profundidad == profundidad_max || estado.gameOver()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    struct NodoOrdenadoLocal { // Keep local struct as in your 3/6 version
        Parchis hijo;
        float valor_heuristico_estimado; // Renamed from 'valor' for clarity
        color c_move; // Renamed from 'c'
        int id_move;  // Renamed from 'id'
        int dado_move;// Renamed from 'dado'
    };

    std::vector<NodoOrdenadoLocal> hijos_ordenados;
    ParchisBros hijos_iterable = estado.getChildren(); // From Parchis.h
    for (auto it = hijos_iterable.begin(); it != hijos_iterable.end(); ++it) {
        if (NodeCounter::isLimitReached()) {
            break;
        }
        hijos_ordenados.push_back({*it, heuristic->evaluate(*it, jugador_maximizer), it.getMovedColor(), it.getMovedPieceId(), it.getMovedDiceValue()});
    }

    if (hijos_ordenados.empty()) {
        if (profundidad == 0) {
            c_piece_out = estado.getCurrentColor();
            id_piece_out = SKIP_TURN;
            dice_out = 0;
        }
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    // Sort children
    if (estado.getCurrentPlayerId() == jugador_maximizer) {
        std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                  [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado > b.valor_heuristico_estimado; });
    } else {
        std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                  [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado < b.valor_heuristico_estimado; });
    }

    // Initialize root move with the first (heuristically best) child's move
    if (profundidad == 0) {
        c_piece_out = hijos_ordenados.front().c_move;
        id_piece_out = hijos_ordenados.front().id_move;
        dice_out = hijos_ordenados.front().dado_move;
    }

    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX node
        float mejor_valor = menosinf;
        for (const auto& nodo : hijos_ordenados) {
            if (NodeCounter::isLimitReached()) { // Check before recursive call
                return mejor_valor; // Return best value found *for this node* so far
            }
            color dummy_c; int dummy_id; int dummy_dice;
            float child_val = Poda_AlfaBeta(nodo.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);

            if (child_val > mejor_valor) {
                mejor_valor = child_val;
                if (profundidad == 0) { // If at root, update the chosen move
                    c_piece_out = nodo.c_move;
                    id_piece_out = nodo.id_move;
                    dice_out = nodo.dado_move;
                }
            }
            alpha = std::max(alpha, mejor_valor);
            if (beta <= alpha) {
                break;
            }
        }
        return mejor_valor;

    } else { // MIN node
        float peor_valor = masinf;
        for (const auto& nodo : hijos_ordenados) {
            if (NodeCounter::isLimitReached()) { // Check before recursive call
                return peor_valor; // Return best value found *for this node* so far
            }
            color dummy_c; int dummy_id; int dummy_dice;
            float child_val = Poda_AlfaBeta(nodo.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);

            // CRITICAL FIX: MIN node does NOT update c_piece_out, id_piece_out, dice_out for the root.
            // The `if (profundidad == 0)` block that was here in your version is removed.

            peor_valor = std::min(peor_valor, child_val);
            beta = std::min(beta, peor_valor);
            if (beta <= alpha) {
                break;
            }
        }
        return peor_valor;
    }
}