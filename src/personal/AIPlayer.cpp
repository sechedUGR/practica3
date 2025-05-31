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
    bool me_quedo_con_esta_accion = false;

    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end() && !me_quedo_con_esta_accion; ++it) {
        Parchis siguiente_hijo = *it;
        if (siguiente_hijo.isEatingMove() || siguiente_hijo.isGoalMove() ||
            (siguiente_hijo.gameOver() && siguiente_hijo.getWinner() == this->jugador)) {
            me_quedo_con_esta_accion = true;
            c_piece = it.getMovedColor();
            id_piece = it.getMovedPieceId();
            dice = it.getMovedDiceValue();
        }
    }
    if (!me_quedo_con_esta_accion) {
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

    float valor = 0;
    ParchisBros hijos = estado.getChildren();

    if (estado.getCurrentPlayerId() == jugador) { // Max
        for (auto it = hijos.begin(); it != hijos.end(); ++it) {
            color c_dummy; int id_dummy; int dice_dummy;
            float child_val = Poda_AlfaBeta(*it, jugador, profundidad+1, profundidad_max, c_dummy, id_dummy, dice_dummy, alpha, beta, heuristic);
            if (child_val > alpha) {
                alpha = child_val;
                if (profundidad == 0) {
                    c_piece = it.getMovedColor();
                    id_piece = it.getMovedPieceId();
                    dice = it.getMovedDiceValue();
                }
            }
            if (beta <= alpha) break;
        }
        return alpha;
    } else { // Min
        for (auto it = hijos.begin(); it != hijos.end(); ++it) {
            color c_dummy; int id_dummy; int dice_dummy;
            float child_val = Poda_AlfaBeta(*it, jugador, profundidad+1, profundidad_max, c_dummy, id_dummy, dice_dummy, alpha, beta, heuristic);
            if (child_val < beta) {
                beta = child_val;
                if (profundidad == 0) {
                    c_piece = it.getMovedColor();
                    id_piece = it.getMovedPieceId();
                    dice = it.getMovedDiceValue();
                }
            }
            if (beta <= alpha) break;
        }
        return beta;
    }
}
