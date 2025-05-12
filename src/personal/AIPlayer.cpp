# include "AIPlayer.h"
# include "../../include/model/Parchis.h"

const float masinf = 9999999999.0, menosinf = -9999999999.0;
const float gana = masinf / 10.f, pierde = menosinf / 10.f;
const int num_pieces = 2;
const int PROFUNDIDAD_MINIMAX = 4; // Umbral maximo de profundidad para el metodo MiniMax
const int PROFUNDIDAD_ALFABETA = 8; // Umbral maximo de profundidad para la poda Alfa_Beta

bool AIPlayer::move(){
   cout << COUT_ORANGE_BOLD << "Realizo un movimiento automatico" << COUT_NOCOLOR << endl;

   color c_piece;
   int id_piece;
   int dice;
   think(c_piece, id_piece, dice);

   cout << COUT_ORANGE_BOLD << "Movimiento elegido: " << str(c_piece) << " " << id_piece << " " << dice << COUT_NOCOLOR << endl;

   actual->movePiece(c_piece, id_piece, dice);
   return true;
}

void AIPlayer::think(color& c_piece, int& id_piece, int& dice) const{
   // IMPLEMENTACIÓN INICIAL DEL AGENTE
   // Esta implementación realiza un movimiento aleatorio.
   // Se proporciona como ejemplo, pero se debe cambiar por una que realice un movimiento inteligente
   //como lo que se muestran al final de la función.

   // OBJETIVO: Asignar a las variables c_piece, id_piece, dice (pasadas por referencia) los valores,
   //respectivamente, de:
   // - color de ficha a mover
   // - identificador de la ficha que se va a mover
   // - valor del dado con el que se va a mover la ficha.

   // El id de mi jugador actual.
   int player = actual->getCurrentPlayerId();

   // Vector que almacenará los dados que se pueden usar para el movimiento
   vector<int> current_dices;
   // Vector que almacenará los ids de las fichas que se pueden mover para el dado elegido.
   vector<tuple<color, int>> current_pieces;

   // Se obtiene el vector de dados que se pueden usar para el movimiento
   current_dices = actual->getAvailableNormalDices(player);
   // Elijo un dado de forma aleatoria.
   dice = current_dices[rand() % current_dices.size()];

   // Se obtiene el vector de fichas que se pueden mover para el dado elegido
   current_pieces = actual->getAvailablePieces(player, dice);

   // Si tengo fichas para el dado elegido muevo una al azar.
   if (current_pieces.size() > 0){
      int random_id = rand() % current_pieces.size();
      id_piece = get<1>(current_pieces[random_id]); // get<i>(tuple<...>) me devuelve el i-ésimo
      c_piece = get<0>(current_pieces[random_id]); // elemento de la tupla
   }
   else{
      // Si no tengo fichas para el dado elegido, pasa turno (la macro SKIP_TURN me permite no mover).
      id_piece = SKIP_TURN;
      c_piece = actual->getCurrentColor(); // Le tengo que indicar mi color actual al pasar turno.
   }

   
   // El siguiente código se proporciona como sugerencia para iniciar la implementación del agente.

   float valor; // Almacena el valor con el que se etiqueta el estado tras el proceso de busqueda.
   float alpha = menosinf, beta = masinf; // Cotas iniciales de la poda AlfaBeta
   // Llamada a la función para la poda (los parámetros son solo una sugerencia, se pueden modificar).
   ValoracionTest valoracionTest;

   // ----------------------------------------------------------------- //

   // Si quiero poder manejar varios comportamientos, puedo usar la variable id del agente para usar una u otra.
   // switch (id)
   // {
   // case 0:
   //    // Mi implementación base de la poda con ValoracionTest
   //    valor = Poda_AlfaBeta(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &valoracionTest);
   //    break;
   // case 1:
   //    // Mi implementación definitiva con la que gano a todos los ninjas.
   //    valor = Poda_Final2DefinitivaAhoraSi(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &miValoracion3);
   //    break;
   // case 2:
   //    // Las distintas pruebas que he realizado (primera prueba)
   //    valor = Poda_AlfaBeta_Mejorada(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &miValoracion1);
   //    break;
   // case 3:
   //    // Las distintas pruebas que he realizado (segunda prueba)
   //    valor = Poda_AlfaBeta_SegundaMejora(*actual, jugador, 0, PROFUNDIDAD_ALFABETA, c_piece, id_piece, dice, alpha, beta, &miValoracion1);
   //    break;
   //  // ...
   // }

}


float ValoracionTest::getHeuristic(const Parchis& estado, int jugador) const{
   // Heurística de prueba proporcionada para validar el funcionamiento del algoritmo de búsqueda.


   int ganador = estado.getWinner();
   int oponente = (jugador + 1) % 2;

   // Si hay un ganador, devuelvo más/menos infinito, según si he ganado yo o el oponente.
   if (ganador == jugador){
      return gana;
   }
   else if (ganador == oponente){
      return pierde;
   }
   else{
      // Colores que juega mi jugador y colores del oponente
      vector<color> my_colors = estado.getPlayerColors(jugador);
      vector<color> op_colors = estado.getPlayerColors(oponente);

      // Recorro todas las fichas de mi jugador
      int puntuacion_jugador = 0;
      // Recorro colores de mi jugador.
      for (int i = 0; i < my_colors.size(); i++){
         color c = my_colors[i];
         // Recorro las fichas de ese color.
         for (int j = 0; j < num_pieces; j++){
            // Valoro positivamente que la ficha esté en casilla segura o meta.
            if (estado.isSafePiece(c, j)){
               puntuacion_jugador++;
            }
            else if (estado.getBoard().getPiece(c, j).get_box().type == goal){
               puntuacion_jugador += 5;
            }
         }
      }

      // Recorro todas las fichas del oponente
      int puntuacion_oponente = 0;
      // Recorro colores del oponente.
      for (int i = 0; i < op_colors.size(); i++){
         color c = op_colors[i];
         // Recorro las fichas de ese color.
         for (int j = 0; j < num_pieces; j++){
            if (estado.isSafePiece(c, j)){
               // Valoro negativamente que la ficha esté en casilla segura o meta.
               puntuacion_oponente++;
            }
            else if (estado.getBoard().getPiece(c, j).get_box().type == goal){
               puntuacion_oponente += 5;
            }
         }
      }

      // Devuelvo la puntuación de mi jugador menos la puntuación del oponente.
      return puntuacion_jugador - puntuacion_oponente;
   }
}
