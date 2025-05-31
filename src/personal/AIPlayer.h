#ifndef __AI_PLAYER_H__
#define __AI_PLAYER_H__

#include "../../include/model/Attributes.h"
#include "../../include/model/Player.h"
#include "../../include/model/Heuristic.h"

// --- Heurística básica ---
class ValoracionTest: public Heuristic{
protected:
   virtual float getHeuristic(const Parchis &estado, int jugador) const;
};

// --- Heurística avanzada para competir con los ninjas ---
class ValoracionAvanzada: public Heuristic{
protected:
   virtual float getHeuristic(const Parchis &estado, int jugador) const;
};

class AIPlayer : public Player {
protected:
   const int id;

public:
   inline AIPlayer(const std::string& name): Player(name), id(1) {}
   inline AIPlayer(const std::string& name, const int id): Player(name), id(id) {}
   inline virtual void perceive(Parchis& p){ Player::perceive(p); }
   virtual bool move();
   inline virtual bool canThink() const{ return true; }
   virtual void think(color & c_piece, int & id_piece, int & dice) const;

   // Comportamientos simples
   void thinkAleatorio(color &c_piece, int &id_piece, int &dice) const;
   void thinkFichaMasAdelantada(color &c_piece, int &id_piece, int &dice) const;
   void thinkMejorOpcion(color &c_piece, int &id_piece, int &dice) const;

   // Poda Alfa-Beta básica y ordenada
   float Poda_AlfaBeta(const Parchis &actual, int jugador, int profundidad, int profundidad_max,
       color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic) const;

   float Poda_AlfaBeta_Ordenada(const Parchis &actual, int jugador, int profundidad, int profundidad_max,
       color &c_piece, int &id_piece, int &dice, float alpha, float beta, Heuristic *heuristic) const;

   // Puedes añadir más aquí si implementas poda probabilística o quietud
};

#endif
