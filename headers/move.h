#ifndef MOVE_H
#define MOVE_H

#include <iostream>
#include <string>

#include "defs.h"

// Class responsible for storing the states of the game before a move for
// undoing it
class MoveInfo{
  int move;
  int fifty_move_counter;
  U64 position_key;
};

class Move{
private:
  /*
    Stores all info about the move in the 32 bits available
    7 bits -> "From square" -> 2^7 = 64 = Number of squares in the board
    0000 0000 0000 0000 0000 0000 0111 1111 -> [0x7F]

    7 bits -> "To square"   -> 2^7 = 64 = Number of squares in the board
    0000 0000 0000 0000 0011 1111 1000 0000 -> [>> 7, 0x7F]

    4 bits -> Piece the move captured -> 2^4 = 16 > Number of pieces (12, 6 for each color)
    0000 0000 0000 0011 1100 0000 1000 0000 -> [>> 14, 0xF]
  */
  int move;

  // Used for move ordering in the search
  int score;

public:
  Move();
  Move(int move);
  Move(int move, int score);
  Move(int from, int to, int captured);
  Move(int from, int to, int captured, int score);

  int get_from();
  int get_to();
  int get_captured();
  int get_score();

  std::string get_repr();
  void print();
};

#endif
