#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
#include <algorithm> // For std::sort, std::max, std::min
#include <vector>
#include <cmath>     // For std::pow
#include <tuple>     // For std::tuple in simple agents
#include <iostream>  // For debugging std::cout (optional)

// --- Constantes Globales ---
const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int num_pieces = 2;  // Using your variable name
const int PROFUNDIDAD_ALFABETA = 7; // You can try increasing this to 8 if time/nodes permit

// --- Implementación de AIPlayer ---
bool AIPlayer::move() {
    color c_piece;
    int id_piece;
    int dice_val; // Renamed to avoid conflict with Poda_AlfaBeta param
    think(c_piece, id_piece, dice_val);
    actual->movePiece(c_piece, id_piece, dice_val);
    return true;
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice to dice_val
    float valor_heuristic = 0; // Renamed from 'valor' to avoid confusion
    float alpha = menosinf, beta = masinf;
    int jugador_maximizer_root = actual->getCurrentPlayerId();

    ValoracionTest valoracionTest;
    ValoracionAvanzada valoracionAvanzada;

    // Initialize output parameters to a default "skip turn" state
    id_piece = SKIP_TURN;
    c_piece = actual->getCurrentColor();
    dice_val = 0; // Or a default dice if SKIP_TURN needs one, typically 0 is fine.

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
    // (void)valor_heuristic; // To suppress unused variable warning if its value isn't used later.
}

// --- AGENTES SIMPLES (Corrected getAvailableSpecialDices) ---
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice_val) const {
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    // REMOVED: getAvailableSpecialDices call
    // if (current_dices.empty()) {
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
    int player = actual->getCurrentPlayerId(); // Get current player
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    // REMOVED: getAvailableSpecialDices call

    if (current_dices.empty()) { // No dice available
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
        return;
    }
    // For this simple agent, let's still pick a random die
    dice_val = current_dices[rand() % current_dices.size()];

    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice_val);

    int id_ficha_mas_adelantada = -1;
    color col_ficha_mas_adelantada = none;
    int min_distancia_meta = 99999; // Use a larger initial value

    for (const auto& piece_tuple : current_pieces) { // Use range-based for
        color col = std::get<0>(piece_tuple);
        int id_curr = std::get<1>(piece_tuple); // Renamed to avoid conflict with member 'id'
        int distancia_meta = actual->distanceToGoal(col, id_curr);
        if (distancia_meta < min_distancia_meta) {
            min_distancia_meta = distancia_meta;
            id_ficha_mas_adelantada = id_curr;
            col_ficha_mas_adelantada = col;
        }
    }

    if (id_ficha_mas_adelantada != -1) { // Check if a piece was found
        id_piece = id_ficha_mas_adelantada;
        c_piece = col_ficha_mas_adelantada;
    } else { // No movable piece found for the chosen die
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    }
}

void AIPlayer::thinkMejorOpcion(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice to dice_val
    ParchisBros hijos = actual->getChildren();

    float mejor_valor = menosinf; // Use menosinf
    color mejor_c = none;
    int mejor_id = -1;
    int mejor_dado = -1;

    // Heuristic weights (consider making them const float)
    const int PESO_META_VAL = 2000; // Renamed to avoid conflict
    const int PESO_CASA_VAL = -500; // Renamed
    const int PESO_DISTANCIA_VAL = -25; // Renamed
    const int PESO_COMER_VAL = 900; // Renamed
    const int BONUS_POR_SEGURO_VAL = 10; // Renamed

    int jugador_perspectiva = actual->getCurrentPlayerId(); // The player whose turn it is currently

    bool move_selected = false;

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        const Parchis& hijo = *it; // Use const& for efficiency
        float valor = 0;

        std::vector<color> my_colors = hijo.getPlayerColors(jugador_perspectiva);
        for (color c : my_colors) {
            for (int j = 0; j < num_pieces; j++) {
                const Box& box = hijo.getBoard().getPiece(c, j).get_box(); // Use const&
                if (box.type == goal) valor += PESO_META_VAL;
                else if (box.type == home) valor += PESO_CASA_VAL;
                else valor += PESO_DISTANCIA_VAL * hijo.distanceToGoal(c, j); // distance is good if low, so negative weight

                if (hijo.isSafePiece(c, j)) valor += BONUS_POR_SEGURO_VAL;
            }
        }
        // Consider opponent's score too for a more complete 1-ply eval
        int oponente = (jugador_perspectiva + 1) % 2;
        std::vector<color> op_colors = hijo.getPlayerColors(oponente);
        for (color c : op_colors) {
            for (int j = 0; j < num_pieces; j++) {
                const Box& box = hijo.getBoard().getPiece(c, j).get_box();
                 if (box.type == goal) valor -= PESO_META_VAL; // Bad for me if opponent goals
                 else if (box.type == home) valor -= PESO_CASA_VAL; // Good for me if opponent home
                 else valor -= PESO_DISTANCIA_VAL * hijo.distanceToGoal(c, j); // Good for me if opp far from goal
                 if (hijo.isSafePiece(c, j)) valor -= BONUS_POR_SEGURO_VAL;
            }
        }


        if (hijo.isEatingMove()) { // If this move eats
            // Check if it's eating an opponent piece (good) or own piece (bad)
            // This requires knowing which piece was eaten. For simplicity, assume eating is good.
             valor += PESO_COMER_VAL;
        }
        if (hijo.isGoalMove()) valor += PESO_META_VAL / 2; // Reaching goal is good

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
    } else if (hijos.begin() != hijos.end()){ // Fallback to first move if no better option but moves exist
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


// --- HEURÍSTICA BÁSICA (ValoracionTest) ---
float ValoracionTest::getHeuristic(const Parchis& estado, int jugador_evaluando_para) const { // Renamed jugador to jugador_evaluando_para
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;
    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    std::vector<color> my_colors = estado.getPlayerColors(jugador_evaluando_para);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);
    int puntuacion_jugador = 0, puntuacion_oponente = 0;

    for (color c : my_colors) { // Use range-based for
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            if (estado.isSafePiece(c, j)) puntuacion_jugador++;
            else if (box.type == goal) puntuacion_jugador += 5;
            // Consider adding distance for ValoracionTest too, e.g.
            // puntuacion_jugador -= estado.distanceToGoal(c, j) * 0.1;
        }
    }
    for (color c : op_colors) { // Use range-based for
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            if (estado.isSafePiece(c, j)) puntuacion_oponente++;
            else if (box.type == goal) puntuacion_oponente += 5;
            // puntuacion_oponente -= estado.distanceToGoal(c, j) * 0.1;
        }
    }
    return puntuacion_jugador - puntuacion_oponente;
}

// --- HEURÍSTICA AVANZADA (ValoracionAvanzada - Enhanced) ---
float ValoracionAvanzada::getHeuristic(const Parchis &estado, int jugador_evaluando_para) const { // Renamed jugador
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;

    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    float score_jugador_total = 0;
    float score_oponente_total = 0;

    // Heuristic parameters (can be tuned)
    const float IN_GOAL_BONUS = 3000.0f;
    const float AT_HOME_PENALTY = -700.0f;
    const float DISTANCE_BASE = 74.0f; // Max distance approx.
    const float DISTANCE_EXP = 1.3f;
    const float SAFE_SQUARE_BONUS = 80.0f;
    const float NEAR_GOAL_DIST = 7.0f;   // If distance <= 7
    const float NEAR_GOAL_BONUS = 150.0f;
    const float JUST_OUT_DIST_MIN = 60.0f; // If distanceToGoal >= 60 (far from goal)
    const float JUST_OUT_BONUS = 60.0f;
    const float TWO_AT_HOME_COLOR_PENALTY = -800.0f;
    const float BARRIER_BONUS = 40.0f; // Bonus for own barrier

    // Evaluate pieces for 'jugador_evaluando_para'
    std::vector<color> my_colors = estado.getPlayerColors(jugador_evaluando_para);
    for (color c : my_colors) {
        int en_casa_count = 0;
        const Box& box0 = estado.getBoard().getPiece(c, 0).get_box();
        const Box& box1 = estado.getBoard().getPiece(c, 1).get_box();

        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) {
                score_jugador_total += IN_GOAL_BONUS;
            } else if (box.type == home) {
                score_jugador_total += AT_HOME_PENALTY;
                en_casa_count++;
            } else {
                score_jugador_total += std::pow(std::max(0.0f, DISTANCE_BASE - dist), DISTANCE_EXP);
                if (estado.isSafePiece(c, j)) score_jugador_total += SAFE_SQUARE_BONUS;
                if (dist <= NEAR_GOAL_DIST && dist > 0) score_jugador_total += NEAR_GOAL_BONUS;
                if (dist >= JUST_OUT_DIST_MIN) score_jugador_total += JUST_OUT_BONUS;
            }
        }
        if (en_casa_count == num_pieces) score_jugador_total += TWO_AT_HOME_COLOR_PENALTY;
        // Barrier check (simplified: two pieces of same color on same non-home/non-goal square)
        if (box0.type != home && box0.type != goal && box0.type == box1.type && box0.num == box1.num) {
            score_jugador_total += BARRIER_BONUS;
        }
    }

    // Evaluate pieces for 'oponente'
    std::vector<color> op_colors = estado.getPlayerColors(oponente);
    for (color c : op_colors) {
        int en_casa_count = 0;
        // Not adding opponent's barrier bonus to my score, but could penalize if it blocks me.
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box();
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) {
                score_oponente_total += IN_GOAL_BONUS;
            } else if (box.type == home) {
                score_oponente_total += AT_HOME_PENALTY;
                en_casa_count++;
            } else {
                score_oponente_total += std::pow(std::max(0.0f, DISTANCE_BASE - dist), DISTANCE_EXP);
                if (estado.isSafePiece(c, j)) score_oponente_total += SAFE_SQUARE_BONUS;
                if (dist <= NEAR_GOAL_DIST && dist > 0) score_oponente_total += NEAR_GOAL_BONUS;
                if (dist >= JUST_OUT_DIST_MIN) score_oponente_total += JUST_OUT_BONUS;
            }
        }
        if (en_casa_count == num_pieces) score_oponente_total += TWO_AT_HOME_COLOR_PENALTY;
    }

    // Bonuses for game events (isEatingMove implies the move *into* this state was eating)
    // Apply from the perspective of 'jugador_evaluando_para'
    // This is tricky. A common way: if an action leads to a state where I just ate, that child state is good.
    // The heuristic evaluates a *state*. If this state was reached by 'jugador_evaluando_para' eating,
    // this flag would be true.
    float game_event_bonus = 0;
    if (estado.isEatingMove()) {
        // Who ate whom? If player 'jugador_evaluando_para' just made the eating move that led to 'estado'.
        // This usually needs to be handled by evaluating the *child* of a move.
        // For a static evaluator, a simpler approach: if eating happened, it's generally dynamic.
        // The user's code added to score_jugador. Let's assume it's a general bonus for player 'jugador_evaluando_para'
        // if this state is a result of their favorable action.
        game_event_bonus += 400; // Value from user's code
    }
    if (estado.isGoalMove()) {
        game_event_bonus += 500; // Value from user's code
    }

    return (score_jugador_total + game_event_bonus) - score_oponente_total;
}


// --- PODA ALFA-BETA (Corrected and Enhanced Node Limit Handling) ---
float AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
    color &c_piece_out, int &id_piece_out, int &dice_out, float alpha, float beta, Heuristic *heuristic) const
{
    // Check 1: Node limit at the start of the function
    if (NodeCounter::isLimitReached()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    // Check 2: Terminal conditions (depth or game over)
    if (profundidad == profundidad_max || estado.gameOver()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    // Define NodoOrdenado locally (as in user's provided code)
    struct NodoOrdenadoLocal { // Renamed to avoid conflict with anonymous namespace one if ever merged
        Parchis hijo;
        float valor_heuristico_estimado; // Used for sorting
        color c_move;
        int id_move;
        int dado_move;
    };

    std::vector<NodoOrdenadoLocal> hijos_ordenados;
    ParchisBros hijos_iterable = estado.getChildren();
    for (auto it = hijos_iterable.begin(); it != hijos_iterable.end(); ++it) {
        if (NodeCounter::isLimitReached()) { // Check while generating children
             // If limit hit while generating, we might not have a full list to sort/evaluate
             // Break and proceed with what we have, or return current heuristic immediately.
             // For simplicity here, break and sort what's available.
            break;
        }
        hijos_ordenados.push_back({*it, heuristic->evaluate(*it, jugador_maximizer), it.getMovedColor(), it.getMovedPieceId(), it.getMovedDiceValue()});
    }

    if (hijos_ordenados.empty()) { // No valid moves
        if (profundidad == 0) {
            c_piece_out = estado.getCurrentColor();
            id_piece_out = SKIP_TURN;
            dice_out = 0;
        }
        return heuristic->evaluate(estado, jugador_maximizer); // Evaluate current state if no moves
    }

    // Initialize root move with the first (potentially best after sorting) child's move
    // This ensures a move is selected if the loop is exited early due to limits/pruning.
    if (profundidad == 0) {
        // Sort first to pick the heuristically best as initial guess
        if (estado.getCurrentPlayerId() == jugador_maximizer) {
            std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                      [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado > b.valor_heuristico_estimado; });
        } else {
            std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                      [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado < b.valor_heuristico_estimado; });
        }
        c_piece_out = hijos_ordenados.front().c_move;
        id_piece_out = hijos_ordenados.front().id_move;
        dice_out = hijos_ordenados.front().dado_move;
    } else { // For deeper nodes, sort normally
        if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX node
            std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                      [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado > b.valor_heuristico_estimado; });
        } else { // MIN node
            std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                      [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado < b.valor_heuristico_estimado; });
        }
    }


    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX node
        float mejor_valor = menosinf;
        for (const auto& nodo : hijos_ordenados) {
            // Check node limit before each recursive call
            if (NodeCounter::isLimitReached()) {
                return mejor_valor; // Return best value found so far for this MAX node
            }

            color dummy_c; int dummy_id; int dummy_dice; // Dummies for recursive calls deeper than root
            float child_val = Poda_AlfaBeta(nodo.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);

            if (child_val > mejor_valor) {
                mejor_valor = child_val;
                if (profundidad == 0) { // Update root move only if a better one is found
                    c_piece_out = nodo.c_move;
                    id_piece_out = nodo.id_move;
                    dice_out = nodo.dado_move;
                }
            }
            alpha = std::max(alpha, mejor_valor);
            if (beta <= alpha) {
                break; // Beta pruning
            }
        }
        return mejor_valor;

    } else { // MIN node
        float peor_valor = masinf;
        for (const auto& nodo : hijos_ordenados) {
            // Check node limit before each recursive call
            if (NodeCounter::isLimitReached()) {
                return peor_valor; // Return best value found so far for this MIN node
            }

            color dummy_c; int dummy_id; int dummy_dice;
            float child_val = Poda_AlfaBeta(nodo.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);

            // MIN node does NOT update c_piece_out, id_piece_out, dice_out for the root move
            peor_valor = std::min(peor_valor, child_val);
            beta = std::min(beta, peor_valor);
            if (beta <= alpha) {
                break; // Alpha pruning
            }
        }
        return peor_valor;
    }
}