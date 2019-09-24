#include "board.h"

Board::Board(){
  this->initialize_piece_keys();
  this->initialize_side_key();

  char starting_fen[] = "k1r5/ppp5/8/8/8/8/5PPP/5R1K w - 0 1";
  this->set_fen(starting_fen);

  this->update_lists_material();
}

MoveList Board::generate_all_moves(){
  ASSERT(this->check_board());

  MoveList list;
  int side = this->side_to_move;
  int opp_side = (side == WHITE ? BLACK : WHITE);

  int forward_dir = (side == WHITE ? 1 : -1);
  int pawn_code = (side == WHITE ? wP : bP);

  // Pawn move list
  for(int pawn_i = 0; pawn_i < this->piece_count[pawn_code]; pawn_i++){
    int sq = this->piece_list[pawn_code][pawn_i];
    ASSERT(IS_SQUARE_ON_BOARD(sq));

    int forward_sq = sq + 10 * forward_dir;
    if(this->pieces[forward_sq] == EMPTY){ // If there's no piece in front of the pawn
      list.add_quiet_move(sq, forward_sq);
    }

    int diag_sq[2] = {sq + 9 * forward_dir, sq + 11 * forward_dir};
    for(int diag : diag_sq){
      if(IS_SQUARE_ON_BOARD(diag) && PIECE_COLOR[this->pieces[diag]] == opp_side){ // If can eat diag
        list.add_capture_move(sq, diag, this->pieces[diag]);
      }
    }
  }

  const int side_pieces[2][6] = {
    {wP, wN, wB, wR, wQ, wK},
    {bP, bN, bB, bR, bQ, bK}
  };

  // Other pieces
  for(int piece : side_pieces[side]){
    ASSERT(IS_PIECE_VALID(piece));

    for(int piece_i = 0; piece_i < this->piece_count[piece]; piece_i++){
      int sq = this->piece_list[piece][piece_i];
      ASSERT(IS_SQUARE_ON_BOARD(sq));

      for(int dir_i = 0; dir_i < PIECE_DIR_COUNT[piece]; dir_i++){
        int dir = PIECES_DIR[piece][dir_i];
        int t_sq = sq + dir;

        while(IS_SQUARE_ON_BOARD(t_sq)){
          if(this->pieces[t_sq] != EMPTY){
            if(PIECE_COLOR[this->pieces[t_sq]] == opp_side){
               list.add_capture_move(sq, t_sq, this->pieces[t_sq]);
            }
          }else{
            list.add_quiet_move(sq, t_sq);
          }

          if(IS_SLIDER_PIECE[piece]){
            if(this->pieces[t_sq] != EMPTY) break;
            t_sq += dir;
          }else{
            break;
          }
        }
      }
    }
  }

  return list;
}

bool Board::is_square_attacked(int sq, int side){
  ASSERT(IS_SQUARE_ON_BOARD(sq));
  ASSERT(IS_SIDE_VALID(side));
  ASSERT(this->check_board());

  // Pawns
  if(side == WHITE){
    if(this->pieces[sq - 11] == wP || this->pieces[sq - 9] == wP) return true;
  }else if(side == BLACK){
    if(this->pieces[sq + 11] == bP || this->pieces[sq + 9] == bP) return true;
  }

  // Knights
  for(int dir_i = 0; dir_i < 8; dir_i++){
    int piece = this->pieces[sq + KNIGHT_DIR[dir_i]];
    if(IS_PIECE_KNIGHT[piece] && PIECE_COLOR[piece] == side) return true;
  }

  // King
  for(int dir_i = 0; dir_i < 8; dir_i++){
    int piece = this->pieces[sq + KING_DIR[dir_i]];
    if(IS_PIECE_KING[piece] && PIECE_COLOR[piece] == side) return true;
  }

  // Rook and Queen
  for(int dir_i = 0; dir_i < 4; dir_i++){
    int dir = ROOK_DIR[dir_i];
    int t_sq = sq + dir;
    int piece = this->pieces[t_sq];

    while(piece != OFFBOARD){
      if(piece != EMPTY){
        if(IS_PIECE_ROOK_QUEEN[piece] && PIECE_COLOR[piece] == side) return true;
        break;
      }
      t_sq += dir;
      piece = this->pieces[t_sq];
    }
  }

  // Bishop and Queen
  for(int dir_i = 0; dir_i < 4; dir_i++){
    int dir = BISHOP_DIR[dir_i];
    int t_sq = sq + dir;
    int piece = this->pieces[t_sq];

    while(piece != OFFBOARD){
      if(piece != EMPTY){
        if(IS_PIECE_BISHOP_QUEEN[piece] && PIECE_COLOR[piece] == side) return true;
        break;
      }
      t_sq += dir;
      piece = this->pieces[t_sq];
    }
  }

  return false;
}

void Board::update_lists_material(){
  for(int i = 0; i < BOARD_SQ_NUM; i++){
    int sq = i;
    int piece = this->pieces[i];
    if(piece != OFFBOARD && piece != EMPTY){
      int color = PIECE_COLOR[piece];

      if(NON_PAWN_PIECES[piece]) this->not_pawn_piece_count[color]++;
      if(MINOR_PIECES[piece]) this->minor_piece_count[color]++;
      if(MAJOR_PIECES[piece]) this->major_piece_count[color]++;

      this->material[color] += PIECE_VAL[piece];

      this->piece_list[piece][this->piece_count[piece]] = sq;
      this->piece_count[piece]++;

      if(piece == wK) this->king_square[WHITE] = sq;
      if(piece == bK) this->king_square[BLACK] = sq;

      if(piece == wP){
        this->pawns[WHITE].set_bit(square_from_120_board_to_64_board[sq]);
        this->pawns[BOTH].set_bit(square_from_120_board_to_64_board[sq]);
      }else if(piece == bP){
        this->pawns[BLACK].set_bit(square_from_120_board_to_64_board[sq]);
        this->pawns[BOTH].set_bit(square_from_120_board_to_64_board[sq]);
      }
    }
  }
}

bool Board::check_board(){
  int t_piece_count[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int t_non_pawn_piece[2] = {0, 0};
  int t_major_piece[2] = {0, 0};
  int t_minor_piece[2] = {0, 0};
  int t_material[2] = {0, 0};

  Bitboard t_pawns[3];
  t_pawns[WHITE] = this->pawns[WHITE];
  t_pawns[BLACK] = this->pawns[BLACK];
  t_pawns[BOTH] = this->pawns[BOTH];

  for(int t_piece = wP; t_piece <= bK; t_piece++){
    for(int t_count = 0; t_count < this->piece_count[t_piece]; t_count++){
      int sq120 = this->piece_list[t_piece][t_count];
      ASSERT(this->pieces[sq120] == t_piece);
    }
  }

  for(int sq64 = 0; sq64 < 64; sq64++){
    int sq120 = square_from_64_board_to_120_board[sq64];
    int t_piece = this->pieces[sq120];
    t_piece_count[t_piece]++;
    int color = PIECE_COLOR[t_piece];

    if(NON_PAWN_PIECES[t_piece]) t_non_pawn_piece[color]++;
    if(MINOR_PIECES[t_piece]) t_minor_piece[color]++;
    if(MAJOR_PIECES[t_piece]) t_major_piece[color]++;

    t_material[color] += PIECE_VAL[t_piece];
  }

  // Checks if the number of pieces is ok
  for(int t_piece = wP; t_piece <= bK; t_piece++){
    ASSERT(t_piece_count[t_piece] == this->piece_count[t_piece]);
  }

  // Checks if the pawn bitboard is ok
  ASSERT(t_pawns[WHITE].count() == this->pawns[WHITE].count());
  ASSERT(t_pawns[BLACK].count() == this->pawns[BLACK].count());
  ASSERT(t_pawns[BOTH].count() == this->pawns[BOTH].count());

  while(t_pawns[WHITE].bitboard){
    int sq64 = t_pawns[WHITE].pop();
    ASSERT(this->pieces[square_from_64_board_to_120_board[sq64]] == wP);
  }

  while(t_pawns[BLACK].bitboard){
    int sq64 = t_pawns[BLACK].pop();
    ASSERT(this->pieces[square_from_64_board_to_120_board[sq64]] == bP);
  }

  while(t_pawns[BOTH].bitboard){
    int sq64 = t_pawns[BOTH].pop();
    ASSERT(this->pieces[square_from_64_board_to_120_board[sq64]] == wP ||
           this->pieces[square_from_64_board_to_120_board[sq64]] == bP);
  }

  // Assert material, major pieces...
  ASSERT(t_material[WHITE] == this->material[WHITE]);
  ASSERT(t_material[BLACK] == this->material[BLACK]);
  ASSERT(t_non_pawn_piece[WHITE] == this->not_pawn_piece_count[WHITE]);
  ASSERT(t_non_pawn_piece[BLACK] == this->not_pawn_piece_count[BLACK]);
  ASSERT(t_major_piece[WHITE] == this->major_piece_count[WHITE]);
  ASSERT(t_major_piece[BLACK] == this->major_piece_count[BLACK]);
  ASSERT(t_minor_piece[WHITE] == this->minor_piece_count[WHITE]);
  ASSERT(t_minor_piece[BLACK] == this->minor_piece_count[BLACK]);

  // Assert key and side
  ASSERT(this->side_to_move == WHITE || this->side_to_move == BLACK);
  ASSERT(this->generate_position_key() == this->position_key);

  // Assert king pos
  ASSERT(this->pieces[this->king_square[WHITE]] == wK);
  ASSERT(this->pieces[this->king_square[BLACK]] == bK);

  return true;
}

void Board::print(){
  std::ostream boardcout(std::cout.rdbuf());

  for(int r = RANK_8; r >= RANK_1; r--){
    boardcout << r + 1 << " ";
    for(int f = FILE_A; f <= FILE_H; f++){
      int sq = FILE_RANK_TO_SQUARE(f, r);
      int piece = this->pieces[sq];
      boardcout << std::setw(2) << PIECE_CHARS[piece];
    }

    boardcout << std::endl;
  }

  boardcout << std::endl << "  ";
  for(int f = FILE_A; f <= FILE_H; f++){
    boardcout << std::setw(2) << (char)('a' + f);
  }
  boardcout << std::endl << std::endl;

  boardcout << "Side: " << COLOR_CHARS[this->side_to_move] << std::endl;
  boardcout << "Position Key: " << std::hex << std::uppercase << this->position_key << std::endl;
}

void Board::set_fen(char *fen){
  ASSERT(fen != nullptr);

  this->reset();

  int piece = 0;
  int rank = RANK_8;
  int file = FILE_A;
  int count = 0;

  while(rank >= RANK_1 && *fen){
    count = 1;
    switch(*fen){
      case 'p': piece = bP; break;
      case 'n': piece = bN; break;
      case 'b': piece = bB; break;
      case 'r': piece = bR; break;
      case 'q': piece = bQ; break;
      case 'k': piece = bK; break;
      case 'P': piece = wP; break;
      case 'N': piece = wN; break;
      case 'B': piece = wB; break;
      case 'R': piece = wR; break;
      case 'Q': piece = wQ; break;
      case 'K': piece = wK; break;

      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
        piece = EMPTY;
        count = *fen - '0';
        break;

      case '/':
      case ' ':
        rank--;
        file = FILE_A;
        fen++;
        continue;

      default:
        std::cout << "FEN error" << std::endl;
        exit(1);
    }

    for(int i = 0; i < count; i++){
      int sq64 = rank * 8 + file;
      int sq120 = square_from_64_board_to_120_board[sq64];
      if(piece != EMPTY) this->pieces[sq120] = piece;
      file++;
    }

    fen++;
  }

  ASSERT(*fen == 'w' || *fen == 'b');

  this->side_to_move = (*fen == 'w') ? WHITE : BLACK;
  this->position_key = this->generate_position_key();
}

void Board::reset(){
  for(int i = 0; i < BOARD_SQ_NUM; i++){
    this->pieces[i] = OFFBOARD;
  }

  for(int i = 0; i < 64; i++){
    this->pieces[square_from_64_board_to_120_board[i]] = EMPTY;
  }

  for(int i = 0; i < COLOR_COUNT; i++){
    this->not_pawn_piece_count[i] = 0;
    this->major_piece_count[i] = 0;
    this->minor_piece_count[i] = 0;
    this->material[i] = 0;
  }

  for(int i = 0; i < COLOR_COUNT + 1; i++){
    this->pawns[i].reset();
  }

  for(int i = 0; i < PIECE_TYPES; i++){
    this->piece_count[i] = 0;
  }

  this->king_square[WHITE] = NO_SQUARE;
  this->king_square[BLACK] = NO_SQUARE;

  this->side_to_move = BOTH;
  this->fifty_move_counter = 0;

  this->ply = 0;
  this->history_ply = 0;

  this->position_key = 0ULL;
}

U64 Board::generate_position_key(){
  U64 final_key = 0;

  for(int sq = 0; sq < BOARD_SQ_NUM; sq++){
    int piece = this->pieces[sq];

    if(piece != NO_SQUARE && piece != EMPTY && piece != OFFBOARD){
      ASSERT(piece >= wP && piece <= bK);
      final_key ^= this->piece_keys[piece][sq];
    }
  }

  if(this->side_to_move == WHITE){
    final_key ^= this->side_key;
  }

  return final_key;
}

void Board::initialize_side_key(){
  U64 r = (U64)rand() | ((U64)rand() << 15) | ((U64)rand() << 30) | ((U64)rand() << 45) | (((U64)rand() & 0xf) << 60);
  this->side_key = r;
}

void Board::initialize_piece_keys(){
  for(int i = 0; i < PIECE_TYPES; i++){
    for(int j = 0; j < BOARD_SQ_NUM; j++){
      U64 r = (U64)rand() | ((U64)rand() << 15) | ((U64)rand() << 30) | ((U64)rand() << 45) | (((U64)rand() & 0xf) << 60);
      this->piece_keys[i][j] = r;
    }
  }
}
