#ifndef __AI_PLAYER_H__
#define __AI_PLAYER_H__

#include "../../include/model/Attributes.h"
#include "../../include/model/Player.h"
#include "../../include/model/Heuristic.h"

//A
// Forward declare Parchis if the above include is problematic,
// though for const Parchis& parameters, the full include is usually better if available.
// class Parchis; // Already forward-declared in Player.h

class ValoracionTest : public Heuristic {
protected:
   virtual float getHeuristic(const Parchis &estado, int jugador_evaluando_para) const;
};

class ValoracionAvanzada : public Heuristic {
protected:
   virtual float getHeuristic(const Parchis &estado, int jugador_evaluando_para) const;
};

class AIPlayer : public Player {
protected:
   const int id;
   // NodoOrdenado struct definition is REMOVED from here.
   // It will be defined in AIPlayer.cpp

public:
   inline AIPlayer(const std::string& name) : Player(name), id(1) {}
   inline AIPlayer(const std::string& name, const int id_param) : Player(name), id(id_param) {}
   inline virtual void perceive(Parchis& p) { Player::perceive(p); }
   virtual bool move();
   inline virtual bool canThink() const { return true; }
   virtual void think(color & c_piece, int & id_piece, int & dice) const;

   void thinkAleatorio(color &c_piece, int &id_piece, int &dice) const;
   void thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice) const;
   void thinkMejorOpcion(color &c_piece, int &id_piece, int &dice) const;

   float Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
                       color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic, bool quiescence_enabled = false) const;

   float QuiescenceSearch(const Parchis &estado, int jugador_maximizer, int profundidad_quiescence, int profundidad_max_quiescence,
                          float alpha, float beta, Heuristic* heuristic) const;
};

#endif
