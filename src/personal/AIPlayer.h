#ifndef __AI_PLAYER_H__
#define __AI_PLAYER_H__

#include "../../include/model/Attributes.h"
#include "../../include/model/Player.h"
#include "../../include/model/Heuristic.h"
#include <string> // For std::string in constructor

// Forward declaration of Parchis is expected to be in Player.h
// class Parchis;

class ValoracionTest: public Heuristic {
protected:
   virtual float getHeuristic(const Parchis &estado, int jugador) const;
};

class ValoracionAvanzada: public Heuristic {
protected:
   virtual float getHeuristic(const Parchis &estado, int jugador) const;
};

class AIPlayer : public Player {
protected:
   const int id;

public:
   inline AIPlayer(const std::string& name): Player(name), id(1) {}
   // Corrected constructor to avoid shadowing id class member
   inline AIPlayer(const std::string& name, const int id_param): Player(name), id(id_param) {}

   inline virtual void perceive(Parchis& p){ Player::perceive(p); }
   virtual bool move();
   inline virtual bool canThink() const{ return true; }
   virtual void think(color & c_piece, int & id_piece, int & dice) const;

   // Comportamientos simples
   void thinkAleatorio(color &c_piece, int &id_piece, int &dice) const;
   void thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice) const;
   void thinkMejorOpcion(color &c_piece, int &id_piece, int &dice) const;

   // Poda Alfa-Beta principal y ordenada (matching your current C++ file)
   float Poda_AlfaBeta(const Parchis &estado, int jugador_maximizer, int profundidad, int profundidad_max,
       color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic) const;
};

#endif