#ifndef _PLAYOUT_H_
#define _PLAYOUT_H_

#include "utils.h"
#include "board.h"

#include <cmath>

// mercy rule

const bool use_mercy_rule      = false;
const uint mercy_threshold     = 25;
const uint max_playout_length  = board_area * 2;

enum playout_status_t { pass_pass, mercy, too_long };

template <typename Policy> class Playout {
public:
  Policy*  policy;
  Board*   board;
  Move*    move_history;
  uint     move_history_length;

  Playout (Policy* policy_, Board*  board_) : policy (policy_), board (board_) {}

  all_inline
  playout_status_t run () {
    uint begin_move_no = board->move_no;
    move_history = board->move_history + board->move_no;
    
    while (true) {
      if (board->both_player_pass ()) {
        move_history_length = board->move_no - begin_move_no;
        return pass_pass;
      }
      
      if (board->move_no >= max_playout_length) {
        move_history_length = board->move_no - begin_move_no;
        return too_long;
      }
      
      if (use_mercy_rule &&
          uint (abs (float(board->approx_score ()))) > mercy_threshold) {
        move_history_length = board->move_no - begin_move_no;
        return mercy;
      }

      policy->play_move (board);
    }
  }
  
};

// -----------------------------------------------------------------------------

class ExtPolicy {
public:
  ExtPolicy(FastRandom& random_) : random(random_) { 
  }

  flatten all_inline
  void play_move (Board* board) {
#ifdef USE_BEGINING_IN_PLAYOUT
		if (play_begin(board)) return; // gramy rozpoczecie !!!
#endif
#ifdef USE_ATARI_IN_PLAYOUT
		if (play_atari(board)) return; // gramy atari !!!
#endif 
#ifdef USE_LOCALITY_IN_PLAYOUT
		if (play_local(board)) return; // gramy lokalnie !!!
#endif

		// Jakis inny losowy ruch - byc moze jeden z wczesniej 
		// odrzuconych przez play_atari() lub play_local().
    
		uint ii_start = random.rand_int (board->empty_v_cnt); 
    uint ii = ii_start;
    Player act_player = board->act_player ();

    Vertex v;
    while (true) { 
      v = board->empty_v [ii];
      if (!board->is_eyelike (act_player, v) &&
          board->is_pseudo_legal (act_player, v)) { 
        board->play_legal(act_player, v);
        return;
      }
      ii += 1;
      ii &= ~(-(ii == board->empty_v_cnt)); // if (ii==board->empty_v_cnt) ii=0;
      if (ii == ii_start) {
        board->play_legal(act_player, Vertex::pass());
        return;
      }
    }
  }

private:
  FastRandom& random;
	static const Vertex begin1[25]; // Dla planszy 9x9 !!!
	static const Vertex begin2[24]; // Dla planszy 9x9 !!!

	flatten all_inline 
	bool play_begin (Board *board) {
		if (board->move_no > 7) return false;
		Player act_player = board->act_player();
		const Vertex *vs;
		uint vc = 0;

		if (random.rand_int(10)) {
			vs = begin1;
			vc = 25;
		} else {
			vs = begin2;
			vc = 24;
		}

		uint start = random.rand_int(vc), i = start;

		do {
      if (board->color_at[vs[i]] == Color::empty() &&
          !board->is_eyelike(act_player, vs[i]) &&
          board->is_pseudo_legal(act_player, vs[i])) {
        board->play_legal(act_player, vs[i]);
        return true;
      }

			i = (i+1) % vc;
		} while (start != i);

		assert(false); // MUSI byc jakies dobrze pole !!!
		return false;
	}

	flatten all_inline
  bool try_play (Board* board, Player pl, Vertex v) {
    if (v != Vertex::any() &&
        !board->is_eyelike (pl, v) &&
        board->is_pseudo_legal (pl, v)) { 
      board->play_legal(pl, v);
      return true;
    }
    return false;
  }

  flatten all_inline
  bool play_atari (Board* board) {
    Player act_player  = board->act_player();
		Vertex blacks[10], whites[10], *vs;
		uint blackc, whitec, vc;

		// Wyszukujemy ostatnie zagrania na atari
		board->find_recent_atari(blacks, whites, blackc, whitec, 10);

		vs = act_player == Player::black() ? blacks:whites;
		vc = act_player == Player::black() ? blackc:whitec;

		// Uciekamy z atari
		if (vc > 0 && random.rand_int(4) == 0) {
			uint i = vc > 1 ? random.rand_int(vc) : 0, start = i;
			
			do {
				// Sprawdzamy czy nie wpadamy w drabinke
				if (
#ifdef USE_LADDER_DETECTION
						board->is_ladder_norec(vs[i], act_player) == act_player &&
#endif
						try_play(board, act_player, vs[i])) 
					return true;

				i = (i+1) % vc;
			} while (start != i);
		}

		vs = act_player == Player::white() ? blacks:whites;
		vc = act_player == Player::white() ? blackc:whitec;

		// Dajemy atari
		if (vc > 0 && random.rand_int(4) != 0) {
			uint i = vc > 1 ? random.rand_int(vc) : 0, start = i;
			
			do {
				if (try_play(board, act_player, vs[i])) return true;
				i = (i+1) % vc;
			} while (start != i);
		}

		return false;
  }

  flatten all_inline
  bool play_local(Board *board) {
    if (board->move_no < 1) return false;
	
		/**** local-p7 ****/
		uint r = random.rand_int(7);
    Vertex center1 = board->move_history[board->move_no-1].get_vertex();
		Vertex local[4];
		 
		switch (r) { 
			case 0: // 1/7
				local[0] = center1.NN();
				local[1] = center1.SS();
				local[2] = center1.WW();
				local[3] = center1.EE();
				break;
			case 1: // 1/7
				local[0] = center1.NW();
				local[1] = center1.NE();
				local[2] = center1.SW();
				local[3] = center1.SE();
				break;
			case 2: // 2/7
			case 3: 
				local[0] = center1.N();
				local[1] = center1.E();
				local[2] = center1.W();
				local[3] = center1.S();
				break;
			default: // 3/7
				return false;
		}

    Player act_player = board->act_player ();

    uint i_start = random.rand_int(4), i = i_start;
    do {
      if (local[i].is_on_board() &&
          board->color_at[local[i]] == Color::empty() &&
          !board->is_eyelike(act_player, local[i]) &&
          board->is_pseudo_legal(act_player, local[i])) {
        board->play_legal(act_player, local[i]);
        return true;
      }
      i = (i + 1) & 3;
    } while (i != i_start);

    // No possibility of a local move
    return false;
  }

};


#endif

