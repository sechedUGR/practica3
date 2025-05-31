#include "AIPlayer.h"
#include "../../include/model/Parchis.h"
#include "../../include/model/NodeCounter.h"
#include <algorithm>
#include <vector>

const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int num_pieces = 2;
// PROFUNDIDAD máxima sin superar límite de nodos (ajusta a 8 si va justo)
const int PROFUNDIDAD_ALFABETA = 9;

bool AIPlayer::move() {
    color c_piece; int id_piece; int dice;
    think(c_piece, id_piece, dice);
    actual->movePiece(c_piece, id_piece, dice);
    return true;
}

void AIPlayer::think(color &c_piece, int &id_piece, int &dice) const {
    float valor = 0;
    float alpha = menosinf, beta = masinf;

    ValoracionTest valoracionTest;
    ValoracionAvanzada valoracionAvanzada;

    switch(id){
        case 0: // Poda Alfa-Beta básica con heurística simple (para testear)
            valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &valoracionTest);
            break;
        case 1: // Poda Alfa-Beta ordenada con heurística avanzada (para competir)
            valor = Poda_AlfaBeta_Ordenada(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &valoracionAvanzada);
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

// --- HEURÍSTICA BÁSICA ---
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

// --- HEURÍSTICA AVANZADA (AQUÍ ESTÁ LA CLAVE PARA GANAR NINJAS) ---
float ValoracionAvanzada::getHeuristic(const Parchis& estado, int jugador) const {
    int ganador = estado.getWinner();
    int oponente = (jugador + 1) % 2;
    if (ganador == jugador) return gana;
    if (ganador == oponente) return pierde;

    float val = 0;
    const int PESO_META = 1000;
    const int PESO_CASA = -500;
    const int PESO_SEGURO = 10;
    const int PESO_DISTANCIA = -5;
    const int PESO_COMER = 300;
    const int PESO_BARRERA = 30;

    std::vector<color> my_colors = estado.getPlayerColors(jugador);
    std::vector<color> op_colors = estado.getPlayerColors(oponente);

    // --- Mis fichas ---
    for (auto c : my_colors) {
        for (int j = 0; j < num_pieces; j++) {
            auto box = estado.getBoard().getPiece(c, j).get_box();
            if (box.type == goal) val += PESO_META;
            else if (box.type == home) val += PESO_CASA;
            val += PESO_DISTANCIA * estado.distanceToGoal(c, j);
            if (estado.isSafePiece(c, j)) val += PESO_SEGURO;

        }
    }

    // --- Fichas rivales ---
    for (auto c : op_colors) {
        for (int j = 0; j < num_pieces; j++) {
            auto box = estado.getBoard().getPiece(c, j).get_box();
            if (box.type == goal) val -= PESO_META;
            else if (box.type == home) val -= PESO_CASA;
            val -= PESO_DISTANCIA * estado.distanceToGoal(c, j);
            if (estado.isSafePiece(c, j)) val -= PESO_SEGURO;

        }
    }//a

    // --- BONUS: Si puedo comer o llegar a meta en la siguiente jugada (usando hijos) ---
    ParchisBros hijos = estado.getChildren();
    for (ParchisBros::Iterator it = hijos.begin(); it != hijos.end(); ++it) {
        if ((*it).isEatingMove()) {
            val += PESO_COMER;
        }
        if ((*it).isGoalMove()) {
            val += PESO_META/2;
        }
    }
    return val;
}

// --- PODA ALFA-BETA BÁSICA ---
float AIPlayer::Poda_AlfaBeta(const Parchis &actual, int jugador, int profundidad, int profundidad_max,
    color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic) const {

    if (profundidad == profundidad_max || actual.gameOver())
        return heuristic->evaluate(actual, jugador);
    if (NodeCounter::isLimitReached())
        return heuristic->evaluate(actual, jugador);

    if (actual.getCurrentPlayerId() == jugador) {
        float valor = menosinf;
        ParchisBros rama = actual.getChildren();
        for (ParchisBros::Iterator it = rama.begin(); it != rama.end(); ++it) {
            color cc; int idp; int d;
            Parchis hijo = *it;
            float new_val = Poda_AlfaBeta(hijo, jugador, profundidad + 1, profundidad_max, cc, idp, d, alpha, beta, heuristic);
            if (new_val > valor) {
                valor = new_val;
                if (profundidad == 0) {
                    c_piece = it.getMovedColor();
                    id_piece = it.getMovedPieceId();
                    dice = it.getMovedDiceValue();
                }
            }
            alpha = std::max(alpha, valor);
            if (beta <= alpha) break;
        }
        return valor;
    } else {
        float valor = masinf;
        ParchisBros rama = actual.getChildren();
        for (ParchisBros::Iterator it = rama.begin(); it != rama.end(); ++it) {
            color cc; int idp; int d;
            Parchis hijo = *it;
            float new_val = Poda_AlfaBeta(hijo, jugador, profundidad + 1, profundidad_max, cc, idp, d, alpha, beta, heuristic);
            if (new_val < valor) valor = new_val;
            beta = std::min(beta, valor);
            if (beta <= alpha) break;
        }
        return valor;
    }
}

// --- PODA ALFA-BETA ORDENADA (MEJORADA PARA NINJAS) ---
float AIPlayer::Poda_AlfaBeta_Ordenada(const Parchis &actual, int jugador, int profundidad, int profundidad_max,
    color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic) const {

    if (profundidad == profundidad_max || actual.gameOver())
        return heuristic->evaluate(actual, jugador);
    if (NodeCounter::isLimitReached())
        return heuristic->evaluate(actual, jugador);

    struct NodoAux {
        Parchis hijo;
        color c_piece; int id_piece; int dice;
        float valor_rapido;
    };

    std::vector<NodoAux> hijos;
    ParchisBros rama = actual.getChildren();
    for (ParchisBros::Iterator it = rama.begin(); it != rama.end(); ++it) {
        NodoAux n;
        n.hijo = *it;
        n.c_piece = it.getMovedColor();
        n.id_piece = it.getMovedPieceId();
        n.dice = it.getMovedDiceValue();
        n.valor_rapido = heuristic->evaluate(n.hijo, jugador);
        hijos.push_back(n);
    }
    if (actual.getCurrentPlayerId() == jugador)
        std::sort(hijos.begin(), hijos.end(), [](const NodoAux& a, const NodoAux& b){ return a.valor_rapido > b.valor_rapido; });
    else
        std::sort(hijos.begin(), hijos.end(), [](const NodoAux& a, const NodoAux& b){ return a.valor_rapido < b.valor_rapido; });

    if (actual.getCurrentPlayerId() == jugador) {
        float valor = menosinf;
        for (int i = 0; i < hijos.size(); ++i) {
            color cc; int idp; int d;
            float new_val = Poda_AlfaBeta_Ordenada(hijos[i].hijo, jugador, profundidad + 1, profundidad_max, cc, idp, d, alpha, beta, heuristic);
            if (new_val > valor) {
                valor = new_val;
                if (profundidad == 0) {
                    c_piece = hijos[i].c_piece;
                    id_piece = hijos[i].id_piece;
                    dice = hijos[i].dice;
                }
            }
            alpha = std::max(alpha, valor);
            if (beta <= alpha) break;
        }
        return valor;
    } else {
        float valor = masinf;
        for (int i = 0; i < hijos.size(); ++i) {
            color cc; int idp; int d;
            float new_val = Poda_AlfaBeta_Ordenada(hijos[i].hijo, jugador, profundidad + 1, profundidad_max, cc, idp, d, alpha, beta, heuristic);
            if (new_val < valor) valor = new_val;
            beta = std::min(beta, valor);
            if (beta <= alpha) break;
        }
        return valor;
    }
}

