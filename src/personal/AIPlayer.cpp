#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
#include <algorithm> // For std::sort, std::max, std::min
#include <vector>
#include <cmath>     // For std::pow
#include <tuple>     // For std::tuple in simple agents
#include <cstdlib>   // For rand() - ensure only one include
// #include <iostream> // Uncomment for debugging std::cout

// --- Constantes Globales ---
const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int num_pieces = 2;
const int PROFUNDIDAD_ALFABETA = 7; // You can experiment with increasing this to 8

bool AIPlayer::move() {
    color c_piece;
    int id_piece;
    int dice_val; // Renamed from dice
    think(c_piece, id_piece, dice_val);
    actual->movePiece(c_piece, id_piece, dice_val);
    return true;
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice to dice_val
    float valor_heuristic = 0; // Renamed from valor
    float alpha = menosinf, beta = masinf;
    // 'jugador' here is the player for whom we want to maximize the score (the AI itself)
    int jugador_maximizer_root = actual->getCurrentPlayerId();

    ValoracionTest valoracionTest;
    ValoracionAvanzada valoracionAvanzada;

    // Initialize output parameters to a default "skip turn" state
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
    if (id_piece == SKIP_TURN) { // Ensure color is correct for SKIP_TURN
        c_piece = actual->getCurrentColor();
    }
    // (void)valor_heuristic; // Suppress unused variable warning if needed
}

// --- AGENTES SIMPLES PARA TESTEAR (Corrected) ---
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice to dice_val
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    // REMOVED: actual->getAvailableSpecialDices(player);

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

void AIPlayer::thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    // REMOVED: actual->getAvailableSpecialDices(player);

    if (current_dices.empty()) {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
        dice_val = 0;
        return;
    }
    // This agent should still pick a die to then pick the most advanced piece for THAT die
    dice_val = current_dices[rand() % current_dices.size()]; // Or could iterate all dice for best option

    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice_val);

    int id_ficha_mas_adelantada = -1;
    color col_ficha_mas_adelantada = none; // Default to 'none' color
    int min_distancia_meta = 99999; // Increased initial large value

    for (const auto& piece_tuple : current_pieces) { // Use range-based for
        color col = std::get<0>(piece_tuple);
        int id_curr = std::get<1>(piece_tuple); // Renamed id to id_curr
        int distancia_meta = actual->distanceToGoal(col, id_curr);
        if (distancia_meta < min_distancia_meta) {
            min_distancia_meta = distancia_meta;
            id_ficha_mas_adelantada = id_curr;
            col_ficha_mas_adelantada = col;
        }
    }

    if (id_ficha_mas_adelantada != -1) { // Check if a piece was actually found
        id_piece = id_ficha_mas_adelantada;
        c_piece = col_ficha_mas_adelantada;
    } else { // No movable piece found for the chosen die
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    }
}

void AIPlayer::thinkMejorOpcion(color &c_piece, int &id_piece, int &dice_val) const { // Renamed dice
    ParchisBros hijos = actual->getChildren();

    float mejor_valor = menosinf; // Initialize with menosinf
    color mejor_c = none; // Default to 'none' color
    int mejor_id = -1;    // Default to invalid id
    int mejor_dado = -1;  // Default to invalid dado

    // Heuristic weights
    const float PESO_META_VAL = 2000.0f;
    const float PESO_CASA_VAL = -500.0f;
    const float PESO_DISTANCIA_VAL = -25.0f; // Negative because lower distance is better
    const float PESO_COMER_VAL = 900.0f;
    const float BONUS_POR_SEGURO_VAL = 10.0f;

    int jugador_perspectiva = actual->getCurrentPlayerId();
    int oponente = (jugador_perspectiva + 1) % 2;
    bool move_selected = false;

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        const Parchis& hijo = *it; // Use const&
        float valor = 0;

        // Evaluate for my pieces
        std::vector<color> my_colors = hijo.getPlayerColors(jugador_perspectiva);
        for (color c : my_colors) {
            for (int j = 0; j < num_pieces; j++) {
                const Box& box = hijo.getBoard().getPiece(c, j).get_box(); // Use const&
                if (box.type == goal) valor += PESO_META_VAL;
                else if (box.type == home) valor += PESO_CASA_VAL;
                else valor += PESO_DISTANCIA_VAL * hijo.distanceToGoal(c, j);

                if (hijo.isSafePiece(c, j)) valor += BONUS_POR_SEGURO_VAL;
            }
        }
        // Evaluate for opponent's pieces (subtract their score)
        std::vector<color> op_colors = hijo.getPlayerColors(oponente);
         for (color c : op_colors) {
            for (int j = 0; j < num_pieces; j++) {
                const Box& box = hijo.getBoard().getPiece(c, j).get_box();
                if (box.type == goal) valor -= PESO_META_VAL; // Subtract opponent's gain
                else if (box.type == home) valor -= PESO_CASA_VAL; // Add to my score if opp is home
                else valor -= PESO_DISTANCIA_VAL * hijo.distanceToGoal(c, j); // Add if opp is far
                if (hijo.isSafePiece(c, j)) valor -= BONUS_POR_SEGURO_VAL;
            }
        }

        // Bonus if this move leads to an eating state for me
        if (hijo.isEatingMove()) {
            // Ideally check if eaten piece is opponent's. For this simple agent, assume eating is good.
            valor += PESO_COMER_VAL;
        }
        // Bonus if this move leads to a goal state for me
        if (hijo.isGoalMove()) valor += PESO_META_VAL / 2.0f; // Goal move is very good

        if (valor > mejor_valor) {
            mejor_valor = valor;
            mejor_c = it.getMovedColor();
            mejor_id = it.getMovedPieceId();
            mejor_dado = it.getMovedDiceValue();
            move_selected = true;
        }
    }

    if (move_selected) { // If a move was selected based on heuristic
        c_piece = mejor_c;
        id_piece = mejor_id;
        dice_val = mejor_dado;
    } else if (hijos.begin() != hijos.end()){ // Fallback to the first available move if no heuristic improvement
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


// --- HEURÍSTICA BÁSICA (la del tutorial, ValoracionTest) ---
float ValoracionTest::getHeuristic(const Parchis& estado, int jugador_evaluando_para) const { // Renamed jugador
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;
    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    std::vector<color> my_colors = estado.getPlayerColors(jugador_evaluando_para);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);
    int puntuacion_jugador = 0, puntuacion_oponente = 0;

    for (color c : my_colors) { // Range-based for
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            if (estado.isSafePiece(c, j)) puntuacion_jugador++;
            else if (box.type == goal) puntuacion_jugador += 5;
            // Consider adding distance to make it slightly better
            puntuacion_jugador -= estado.distanceToGoal(c, j) * 0.05f; // Small factor
        }
    }
    for (color c : op_colors) { // Range-based for
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            if (estado.isSafePiece(c, j)) puntuacion_oponente++;
            else if (box.type == goal) puntuacion_oponente += 5;
            puntuacion_oponente -= estado.distanceToGoal(c, j) * 0.05f;
        }
    }
    return puntuacion_jugador - puntuacion_oponente;
}

// --- HEURÍSTICA AVANZADA (ValoracionAvanzada - From your 2/6 version, with barrier addition) ---
float ValoracionAvanzada::getHeuristic(const Parchis &estado, int jugador_evaluando_para) const { // Renamed jugador
    int ganador = estado.getWinner();
    int oponente = (jugador_evaluando_para + 1) % 2;

    if (ganador == jugador_evaluando_para) return gana;
    if (ganador == oponente) return pierde;

    std::vector<color> my_colors = estado.getPlayerColors(jugador_evaluando_para);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);

    float score_jugador = 0, score_oponente = 0; // Use float for scores

    // Heuristic parameters (tune these!)
    const float IN_GOAL_BONUS_ADV = 3000.0f;
    const float AT_HOME_PENALTY_ADV = -750.0f;
    const float DISTANCE_BASE_ADV = 74.0f;
    const float DISTANCE_EXP_ADV = 1.3f; //Exponent can make distance more impactful
    const float SAFE_SQUARE_BONUS_ADV = 75.0f;
    const float NEAR_GOAL_DIST_ADV = 7.0f;
    const float NEAR_GOAL_BONUS_ADV = 150.0f;
    const float JUST_OUT_DIST_MIN_ADV = 65.0f; // If distanceToGoal >= 65
    const float JUST_OUT_BONUS_ADV = 60.0f;
    const float TWO_AT_HOME_COLOR_PENALTY_ADV = -800.0f;
    const float BARRIER_BONUS_ADV = 50.0f; // Added bonus for own barrier
    const float EATING_MOVE_BONUS_ADV = 450.0f; // Slightly increased
    const float GOAL_MOVE_BONUS_ADV = 550.0f;   // Slightly increased

    for (color c : my_colors) {
        int en_casa = 0;
        const Box& box0 = estado.getBoard().getPiece(c, 0).get_box();
        const Box& box1 = estado.getBoard().getPiece(c, 1).get_box();

        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) score_jugador += IN_GOAL_BONUS_ADV;
            else if (box.type == home) {
                score_jugador += AT_HOME_PENALTY_ADV;
                en_casa++;
            } else {
                score_jugador += std::pow(std::max(0.0f, DISTANCE_BASE_ADV - dist), DISTANCE_EXP_ADV);
                if (estado.isSafePiece(c, j)) score_jugador += SAFE_SQUARE_BONUS_ADV;
                if (dist <= NEAR_GOAL_DIST_ADV && dist > 0) score_jugador += NEAR_GOAL_BONUS_ADV;
                if (dist >= JUST_OUT_DIST_MIN_ADV) score_jugador += JUST_OUT_BONUS_ADV;
            }
        }
        if (en_casa == num_pieces) score_jugador += TWO_AT_HOME_COLOR_PENALTY_ADV;
        // Barrier check
        if (box0.type != home && box0.type != goal && box0.type == box1.type && box0.num == box1.num) {
            score_jugador += BARRIER_BONUS_ADV;
        }
    }

    for (color c : op_colors) {
        int en_casa = 0;
        for (int j = 0; j < num_pieces; j++) {
            const Box& box = estado.getBoard().getPiece(c, j).get_box(); // Use const&
            int dist = estado.distanceToGoal(c, j);

            if (box.type == goal) score_oponente += IN_GOAL_BONUS_ADV;
            else if (box.type == home) {
                score_oponente += AT_HOME_PENALTY_ADV;
                en_casa++;
            } else {
                score_oponente += std::pow(std::max(0.0f, DISTANCE_BASE_ADV - dist), DISTANCE_EXP_ADV);
                if (estado.isSafePiece(c, j)) score_oponente += SAFE_SQUARE_BONUS_ADV;
                if (dist <= NEAR_GOAL_DIST_ADV && dist > 0) score_oponente += NEAR_GOAL_BONUS_ADV;
                if (dist >= JUST_OUT_DIST_MIN_ADV) score_oponente += JUST_OUT_BONUS_ADV;
            }
        }
        if (en_casa == num_pieces) score_oponente += TWO_AT_HOME_COLOR_PENALTY_ADV;
    }

    float event_score_boost = 0;
    // isEatingMove and isGoalMove refer to the transition INTO the current 'estado'.
    // If jugador_evaluando_para is the one who made that move, these are positive.
    // This is difficult to assert directly without knowing who moved last.
    // The current simple addition implies these are good if they happened, favoring the current player's eval.
    if (estado.isEatingMove()) event_score_boost += EATING_MOVE_BONUS_ADV;
    if (estado.isGoalMove()) event_score_boost += GOAL_MOVE_BONUS_ADV;

    // The interpretation of event_score_boost needs care.
    // If 'estado' is a child node generated by 'jugador_evaluando_para's move, then these bonuses are for 'jugador_evaluando_para'.
    // If 'estado' is a child node generated by 'oponente's move, these bonuses should be for 'oponente' (i.e., subtracted from 'jugador_evaluando_para's score).
    // Since heuristic evaluates H(state, root_player), and isEatingMove is a property of 'state':
    // If this 'state' was reached by root_player making an eating move, it's good.
    // If this 'state' was reached by opponent making an eating move, it's bad for root_player.
    // Without knowing *who* made the move to get to 'estado', this is ambiguous.
    // For now, keeping your original structure: bonuses are added to 'score_jugador' before subtracting 'score_oponente'.
    // This implicitly assumes these events are favorable to 'jugador_evaluando_para' if they occurred.

    return (score_jugador + event_score_boost) - score_oponente;
}


// --- PODA ALFA-BETA (Corrected and Enhanced from your 2/6 version) ---
float AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
    color &c_piece_out, int &id_piece_out, int &dice_out, float alpha, float beta, Heuristic *heuristic) const
{
    // Check 1: Node limit at the start of the function
    if (NodeCounter::isLimitReached()) {
        // If limit hit, return heuristic of current state.
        // c_piece_out etc. will retain their values from the parent call's initialization (or previous best).
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    // Check 2: Terminal conditions (depth or game over)
    if (profundidad == profundidad_max || estado.gameOver()) {
        return heuristic->evaluate(estado, jugador_maximizer);
    }

    // Local struct for sorting children
    struct NodoOrdenadoLocal {
        Parchis hijo;
        float valor_heuristico_estimado;
        color c_move;
        int id_move;
        int dado_move;
    };

    std::vector<NodoOrdenadoLocal> hijos_ordenados;
    ParchisBros hijos_iterable = estado.getChildren(); // This comes from Parchis.h
    for (auto it = hijos_iterable.begin(); it != hijos_iterable.end(); ++it) {
        if (NodeCounter::isLimitReached()) { // Check while generating children
            break; // Stop generating more children if limit is hit
        }
        // Evaluate child for sorting purposes
        hijos_ordenados.push_back({*it, heuristic->evaluate(*it, jugador_maximizer), it.getMovedColor(), it.getMovedPieceId(), it.getMovedDiceValue()});
    }

    if (hijos_ordenados.empty()) { // No valid moves from this state
        if (profundidad == 0) { // At the root, if no moves, must explicitly skip.
            c_piece_out = estado.getCurrentColor();
            id_piece_out = SKIP_TURN;
            dice_out = 0; // Or a default die.
        }
        return heuristic->evaluate(estado, jugador_maximizer); // Evaluate current state
    }

    // Sort children for move ordering
    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX node: sort descending by heuristic value
        std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                  [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado > b.valor_heuristico_estimado; });
    } else { // MIN node: sort ascending by heuristic value (best for MIN is worst for MAX)
        std::sort(hijos_ordenados.begin(), hijos_ordenados.end(),
                  [](const NodoOrdenadoLocal& a, const NodoOrdenadoLocal& b) { return a.valor_heuristico_estimado < b.valor_heuristico_estimado; });
    }

    // Initialize root move with the first (heuristically best after sorting) child's move.
    // This ensures a move is always selected if children exist, even if search is cut short.
    if (profundidad == 0) {
        c_piece_out = hijos_ordenados.front().c_move;
        id_piece_out = hijos_ordenados.front().id_move;
        dice_out = hijos_ordenados.front().dado_move;
    }


    if (estado.getCurrentPlayerId() == jugador_maximizer) { // MAX node
        float mejor_valor = menosinf;
        for (const auto& nodo : hijos_ordenados) {
            // Check node limit before each recursive call
            if (NodeCounter::isLimitReached()) {
                // If limit hit, mejor_valor holds the best found so far.
                // c_piece_out etc. at root would have been updated if this mejor_valor
                // came from a fully explored child that set it.
                return mejor_valor;
            }

            color dummy_c; int dummy_id; int dummy_dice; // Dummies for recursive calls
            float child_val = Poda_AlfaBeta(nodo.hijo, jugador_maximizer, profundidad + 1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);

            if (child_val > mejor_valor) {
                mejor_valor = child_val;
                if (profundidad == 0) { // Update root move if this child is better
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

            // CRITICAL FIX: MIN node does NOT update c_piece_out, id_piece_out, dice_out
            // Those are for the root's (MAX player's) decision.
            // if (profundidad == 0) { /* THIS BLOCK WAS REMOVED */ }

            peor_valor = std::min(peor_valor, child_val);
            beta = std::min(beta, peor_valor);
            if (beta <= alpha) {
                break; // Alpha pruning
            }
        }
        return peor_valor;
    }
}