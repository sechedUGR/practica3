#ifndef __AI_PLAYER_H__
#define __AI_PLAYER_H__

#include "../../include/model/Attributes.h" // Provides color, Box, etc.
#include "../../include/model/Player.h"   // Provides base Player, should forward-declare Parchis
#include "../../include/model/Heuristic.h" // Provides base Heuristic

// Standard library includes that might be needed by this header's declarations
#include <string> // For std::string in constructors
#include <vector> // Although not directly in signatures here, good practice if related types use it.

// Forward declaration of Parchis is expected to be in Player.h
// class Parchis;

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

public:
   inline AIPlayer(const std::string& name) : Player(name), id(1) {}
   // Corrected constructor to avoid shadowing 'id' class member
   inline AIPlayer(const std::string& name, const int id_param) : Player(name), id(id_param) {}

   inline virtual void perceive(Parchis& p) { Player::perceive(p); }
   virtual bool move();
   inline virtual bool canThink() const{ return true; }
   virtual void think(color & c_piece, int & id_piece, int & dice) const;

   void thinkAleatorio(color &c_piece, int &id_piece, int &dice) const;
   void thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice) const;
   void thinkMejorOpcion(color &c_piece, int &id_piece, int &dice) const;

   // Poda_AlfaBeta signature now includes quiescence_enabled
   float Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
                       color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic, bool quiescence_enabled = false) const;

   float QuiescenceSearch(const Parchis &estado, int jugador_maximizer, int profundidad_quiescence, int profundidad_max_quiescence,
                           float alpha, float beta, Heuristic* heuristic) const;
};

#endif