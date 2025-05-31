#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
#include <algorithm>
#include <vector>
#include <cmath>

const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int num_pieces = 2;  // ¡CRITERIO DE TU AÑO!
const int PROFUNDIDAD_ALFABETA = 7; // Puedes subir a 8 si no te pasas de nodos

bool AIPlayer::move() {
    color c_piece; int id_piece; int dice;
    think(c_piece, id_piece, dice);
    actual->movePiece(c_piece, id_piece, dice);
    return true;
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice) const {
    float valor = 0;
    float alpha = menosinf, beta = masinf;
    int jugador = actual->getCurrentPlayerId(); // ¡¡IMPORTANTE, usar el jugador real!!

    ValoracionTest valoracionTest;
    ValoracionAvanzada valoracionAvanzada;

    switch(id){
        case 0: // Poda Alfa-Beta básica con heurística test
            valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &valoracionTest);
            break;
        case 1: // Poda Alfa-Beta con heurística avanzada
            valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &valoracionAvanzada);
            break;
        case 2:
            thinkMejorOpcion(c_piece, id_piece, dice);
            break;
        case 3:
            thinkFichaMasAdelantada(c_piece, id_piece, dice);
            break;
        default:
            thinkAleatorio(c_piece, id_piece, dice);
            break;
    }
}

// --- AGENTES SIMPLES PARA TESTEAR ---
void AIPlayer::thinkAleatorio(color &c_piece, int &id_piece, int &dice) const {
    int player = actual->getCurrentPlayerId();
    std::vector<int> current_dices = actual->getAvailableNormalDices(player);
    dice = current_dices[rand() % current_dices.size()];
    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice);

    if (!current_pieces.empty()) {
        int random_id = rand() % current_pieces.size();
        id_piece = std::get<1>(current_pieces[random_id]);
        c_piece = std::get<0>(current_pieces[random_id]);
    } else {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    }
}

void AIPlayer::thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice) const {
    thinkAleatorio(c_piece, id_piece, dice);
    int player = actual->getCurrentPlayerId();
    std::vector<std::tuple<color, int>> current_pieces = actual->getAvailablePieces(player, dice);

    int id_ficha_mas_adelantada = -1;
    color col_ficha_mas_adelantada = none;
    int min_distancia_meta = 9999;

    for (int i = 0; i < current_pieces.size(); i++) {
        color col = std::get<0>(current_pieces[i]);
        int id = std::get<1>(current_pieces[i]);
        int distancia_meta = actual->distanceToGoal(col, id);
        if (distancia_meta < min_distancia_meta) {
            min_distancia_meta = distancia_meta;
            id_ficha_mas_adelantada = id;
            col_ficha_mas_adelantada = col;
        }
    }

    if (id_ficha_mas_adelantada == -1) {
        id_piece = SKIP_TURN;
        c_piece = actual->getCurrentColor();
    } else {
        id_piece = id_ficha_mas_adelantada;
        c_piece = col_ficha_mas_adelantada;
    }
}

void AIPlayer::thinkMejorOpcion(color &c_piece, int &id_piece, int &dice) const {
    ParchisBros hijos = actual->getChildren();

    float mejor_valor = -1e9;
    color mejor_c = none;
    int mejor_id = -1;
    int mejor_dado = -1;

    const int PESO_META = 2000;
    const int PESO_CASA = -500;
    const int PESO_DISTANCIA = -25;
    const int PESO_COMER = 900;
    const int BONUS_POR_SEGURO = 10;

    int jugador = actual->getCurrentPlayerId();

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        Parchis hijo = *it;
        float valor = 0;

        // Para cada color y ficha mía, suma valores
        std::vector<color> my_colors = hijo.getPlayerColors(jugador);
        for (color c : my_colors) {
            for (int j = 0; j < num_pieces; j++) {
                auto box = hijo.getBoard().getPiece(c, j).get_box();
                if (box.type == goal) valor += PESO_META;
                else if (box.type == home) valor += PESO_CASA;
                valor += PESO_DISTANCIA * hijo.distanceToGoal(c, j);
                if (hijo.isSafePiece(c, j)) valor += BONUS_POR_SEGURO;
            }
        }

        // Bonus si comes
        if (hijo.isEatingMove()) valor += PESO_COMER;
        // Bonus si haces meta
        if (hijo.isGoalMove()) valor += PESO_META / 2;

        if (valor > mejor_valor) {
            mejor_valor = valor;
            mejor_c = it.getMovedColor();
            mejor_id = it.getMovedPieceId();
            mejor_dado = it.getMovedDiceValue();
        }
    }

    if (mejor_id != -1) {
        c_piece = mejor_c;
        id_piece = mejor_id;
        dice = mejor_dado;
    } else {
        thinkFichaMasAdelantada(c_piece, id_piece, dice);
    }
}


// --- HEURÍSTICA BÁSICA (la del tutorial) ---
float ValoracionTest::getHeuristic(const Parchis& estado, int jugador) const {
    int ganador = estado.getWinner();
    int oponente = (jugador + 1) % 2;
    if (ganador == jugador) return gana;
    if (ganador == oponente) return pierde;

    std::vector<color> my_colors = estado.getPlayerColors(jugador);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);
    int puntuacion_jugador = 0, puntuacion_oponente = 0;

    for (int i = 0; i < my_colors.size(); i++) {
        color c = my_colors[i];
        for (int j = 0; j < num_pieces; j++) {
            if (estado.isSafePiece(c, j)) puntuacion_jugador++;
            else if (estado.getBoard().getPiece(c, j).get_box().type == goal) puntuacion_jugador += 5;
        }
    }
    for (int i = 0; i < op_colors.size(); i++) {
        color c = op_colors[i];
        for (int j = 0; j < num_pieces; j++) {
            if (estado.isSafePiece(c, j)) puntuacion_oponente++;
            else if (estado.getBoard().getPiece(c, j).get_box().type == goal) puntuacion_oponente += 5;
        }
    }
    return puntuacion_jugador - puntuacion_oponente;
}

// --- HEURÍSTICA AVANZADA ADAPTADA ---
#include <cstdlib> // arriba, para rand()

#include <cstdlib> // arriba, para rand()

float ValoracionAvanzada::getHeuristic(const Parchis &estado, int jugador) const {
    int ganador = estado.getWinner();
    int oponente = (jugador+1) % 2;
    if (ganador == jugador)
        return gana;
    else if (ganador == oponente)
        return pierde;

    std::vector<color> my_colors = estado.getPlayerColors(jugador);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);

    int puntuacion_jugador = 0;
    color ganador_yo = none;
    int maximo = 0;

    // Jugador
    for (int i = 0; i < my_colors.size(); i++) {
        color c = my_colors[i];
        if (estado.piecesAtGoal(c) > maximo) {
            ganador_yo = c;
            maximo = estado.piecesAtGoal(c);
        }
        for (int j = 0; j < num_pieces; j++) {
            if (estado.getBoard().getPiece(c, j).get_box().type == home) {
                if (ganador_yo == c)
                    puntuacion_jugador -= 80;
                else
                    puntuacion_jugador -= 50;
            }
            if (estado.distanceToGoal(c, j) <= 8) {
                puntuacion_jugador += 50;
                if (estado.distanceToGoal(c, j) == 0)
                    puntuacion_jugador += 50;
            }
            if (ganador_yo == c)
                puntuacion_jugador += pow(74 - estado.distanceToGoal(c, j), 1.5);
            else
                puntuacion_jugador += pow(74 - estado.distanceToGoal(c, j), 1.2);
        }
    }

    // Oponente
    int puntuacion_oponente = 0;
    color ganador_op = none;
    int maximo_op = 0;
    for (int i = 0; i < op_colors.size(); i++) {
        color c = op_colors[i];
        if (estado.piecesAtGoal(c) > maximo_op) {
            ganador_op = c;
            maximo_op = estado.piecesAtGoal(c);
        }
        for (int j = 0; j < num_pieces; j++) {
            if (estado.getBoard().getPiece(c, j).get_box().type == home) {
                if (ganador_op == c)
                    puntuacion_oponente -= 80;
                else
                    puntuacion_oponente -= 50;
            }
            if (estado.distanceToGoal(c, j) <= 8) {
                puntuacion_oponente += 50;
                if (estado.distanceToGoal(c, j) == 0)
                    puntuacion_oponente += 50;
            }
            if (ganador_op == c)
                puntuacion_oponente += pow(74 - estado.distanceToGoal(c, j), 1.5);
            else
                puntuacion_oponente += pow(74 - estado.distanceToGoal(c, j), 1.2);
        }
    }

    return puntuacion_jugador - puntuacion_oponente;
}

// --- PODA ALFA-BETA ADAPTADA ---
float AIPlayer::Poda_AlfaBeta(const Parchis &estado, int jugador, int profundidad, int profundidad_max,
    color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic) const
{
    if (profundidad == profundidad_max || estado.gameOver() || NodeCounter::isLimitReached()) {
        return heuristic->evaluate(estado, jugador);
    }

    ParchisBros hijos = estado.getChildren();

    // Si no hay hijos, pasa turno (movimiento legal)
    if (hijos.begin() == hijos.end()) {
        if (profundidad == 0) {
            c_piece = estado.getCurrentColor();
            id_piece = SKIP_TURN;
            dice = 0;
        }
        return heuristic->evaluate(estado, jugador);
    }

    if (estado.getCurrentPlayerId() == jugador) { // Max
        float mejor_valor = menosinf;
        for (auto it = hijos.begin(); it != hijos.end(); ++it) {
            color dummy_c; int dummy_id; int dummy_dice;
            float child_val = Poda_AlfaBeta(*it, jugador, profundidad+1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);
            if (child_val > mejor_valor) {
                mejor_valor = child_val;
                if (profundidad == 0) {
                    c_piece = it.getMovedColor();
                    id_piece = it.getMovedPieceId();
                    dice = it.getMovedDiceValue();
                }
            }
            alpha = std::max(alpha, mejor_valor);
            if (beta <= alpha) break;
        }
        return mejor_valor;
    } else {
        float peor_valor = masinf;
        for (auto it = hijos.begin(); it != hijos.end(); ++it) {
            color dummy_c; int dummy_id; int dummy_dice;
            float child_val = Poda_AlfaBeta(*it, jugador, profundidad+1, profundidad_max,
                                            dummy_c, dummy_id, dummy_dice, alpha, beta, heuristic);
            if (child_val < peor_valor) {
                peor_valor = child_val;
                if (profundidad == 0) {
                    c_piece = it.getMovedColor();
                    id_piece = it.getMovedPieceId();
                    dice = it.getMovedDiceValue();
                }
            }
            beta = std::min(beta, peor_valor);
            if (beta <= alpha) break;
        }
        return peor_valor;
    }
}

