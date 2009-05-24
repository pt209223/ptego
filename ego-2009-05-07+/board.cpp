/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  This file is part of Library of Effective GO routines - EGO library      *
 *                                                                           *
 *  Copyright 2006 and onwards, Lukasz Lew                                   *
 *                                                                           *
 *  EGO library is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  EGO library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with EGO library; if not, write to the Free Software               *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor,                           *
 *  Boston, MA  02110-1301  USA                                              *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <cmath>
#include <cstring>
#include <iostream>

#include "board.h"
#include "testing.h"

all_inline
Board::NbrCounter Board::NbrCounter::OfCounts (uint black_cnt,
                                               uint white_cnt,
                                               uint empty_cnt) {
  assertc (nbr_cnt_ac, black_cnt <= max);
  assertc (nbr_cnt_ac, white_cnt <= max);
  assertc (nbr_cnt_ac, empty_cnt <= max);
  Board::NbrCounter nc;
  nc.bitfield =
    (black_cnt << f_shift[0]) +
    (white_cnt << f_shift[1]) +
    (empty_cnt << f_shift[2]);
  return nc;
}

all_inline
Board::NbrCounter Board::NbrCounter::Empty () {
  return OfCounts(0, 0, max); 
}

all_inline
void Board::NbrCounter::player_inc (Player player) {
  bitfield += player_inc_tab [player.get_idx ()]; 
}

all_inline
void Board::NbrCounter::player_dec (Player player) {
  bitfield -= player_inc_tab [player.get_idx ()]; 
}

all_inline
void Board::NbrCounter::off_board_inc () { 
  static const uint off_board_inc_val = 
    (1 << f_shift[0]) + (1 << f_shift[1]) - (1 << f_shift[2]);
  bitfield += off_board_inc_val; 
}

all_inline
uint Board::NbrCounter::empty_cnt () const {
  return bitfield >> f_shift[2]; 
}

all_inline
uint Board::NbrCounter::player_cnt (Player pl) const { 
  static const uint f_mask = (1 << f_size) - 1;
  return (bitfield >> f_shift [pl.get_idx ()]) & f_mask; 
}

all_inline
uint Board::NbrCounter::player_cnt_is_max (Player pl) const {
  return
    (player_cnt_is_max_mask [pl.get_idx ()] & bitfield) ==
    player_cnt_is_max_mask [pl.get_idx ()];
}

all_inline
void Board::NbrCounter::check () {
  if (!nbr_cnt_ac) return;
  assert (empty_cnt () <= max);
  assert (player_cnt (Player::black ()) <= max);
  assert (player_cnt (Player::white ()) <= max);
}

all_inline
void Board::NbrCounter::check(const FastMap<Color, uint>& nbr_color_cnt) const {
  if (!nbr_cnt_ac) return;

  uint expected_nbr_cnt =        // definition of nbr_cnt[v]
    + ((nbr_color_cnt [Color::black ()] + nbr_color_cnt [Color::off_board ()])
       << f_shift[0])
    + ((nbr_color_cnt [Color::white ()] + nbr_color_cnt [Color::off_board ()])
       << f_shift[1])
    + ((nbr_color_cnt [Color::empty ()])
       << f_shift[2]);
  assert (bitfield == expected_nbr_cnt);
}

const uint Board::NbrCounter::max = 4;    // maximal number of neighbours
const uint Board::NbrCounter::f_size = 4; // size in bits of each of 3 counters
const uint Board::NbrCounter::f_shift [3] = {
  0 * f_size,
  1 * f_size,
  2 * f_size,
};

const uint Board::NbrCounter::player_cnt_is_max_mask [Player::cnt] = {  // TODO player_Map
  (max << f_shift[0]),
  (max << f_shift[1])
};

const uint Board::NbrCounter::player_inc_tab [Player::cnt] = {
  (1 << f_shift[0]) - (1 << f_shift[2]),
  (1 << f_shift[1]) - (1 << f_shift[2]),
};


// -----------------------------------------------------------------------------


all_inline
bool Board::undo () {
  Move replay [max_game_length];

  uint   game_length  = move_no;
  float  old_komi     = komi ();

  if (game_length == 0)
    return false;

  rep (mn, game_length-1)
    replay [mn] = move_history [mn];

  clear ();
  set_komi (old_komi); // TODO maybe last_player should be preserverd as well

  rep (mn, game_length-1)
    play_legal (replay [mn].get_player (), replay [mn].get_vertex ());

  return true;
}

all_inline
bool Board::is_strict_legal (Player pl, Vertex v) {
  if (try_play (pl, v) == false) return false;
  bool ok = undo ();
  assert(ok);
  return true;
}

all_inline
bool Board::is_hash_repeated () {
  Board tmp_board;
  rep (mn, move_no-1) {
    tmp_board.play_legal (move_history [mn].get_player (),
                          move_history [mn].get_vertex ());
    if (hash_ == tmp_board.hash_)
      return true;
  }
  return false;
}

all_inline
bool Board::try_play (Player player, Vertex v) {
  if (v == Vertex::pass ()) {
    play_legal (player, v);
    return true;
  }

  v.check_is_on_board ();

  if (color_at [v] != Color::empty ())
    return false;

  if (is_pseudo_legal (player,v) == false)
    return false;

  play_legal (player, v);

  if (last_move_status != play_ok) {
    bool ok = undo ();
    assert(ok);
    return false;
  }

  if (is_hash_repeated ()) {
    bool ok = undo ();
    assert(ok);
    return false;
  }

  return true;
}

string Board::to_string (Vertex mark_v) const {
  ostringstream out;

#define os(n)      out << " " << n
#define o_left(n)  out << "(" << n
#define o_right(n) out << ")" << n

  out << " ";
  if (board_size < 10) out << " "; else out << "  ";
  coord_for_each (col) os (col.col_to_string ());
  out << endl;

  coord_for_each (row) {
    if (board_size >= 10 && board_size - row.idx < 10) out << " ";
    os (row.row_to_string ());
    coord_for_each (col) {
      Vertex v = Vertex (row, col);
      char ch = color_at [v].to_char ();
      if      (v == mark_v)        o_left  (ch);
      else if (v == mark_v.E ())   o_right (ch);
      else                         os (ch);
    }
    if (board_size >= 10 && board_size - row.idx < 10) out << " ";
    os (row.row_to_string ());
    out << endl;
  }

  if (board_size < 10) out << "  "; else out << "   ";
  coord_for_each (col) os (col.col_to_string ());
  out << endl;

#undef os
#undef o_left
#undef o_right

  return out.str ();
}

void Board::print_cerr (Vertex v) const {
  cerr << to_string (v);
}

bool Board::load_from_ascii (istream& ifs) {
  uint     bs;
  char     c;

  Player    play_player[board_area];
  Vertex  play_v[board_area];
  uint      play_cnt;

  clear ();

  play_cnt = 0;

  if (!ifs) return false;

  ifs >> bs;
  if (bs != board_size) return false;

  if (getc_non_space (ifs) != '\n') return false;

  coord_for_each (row) {
    coord_for_each (col) {
      Color color;

      c      = getc_non_space (ifs);
      color  = Color (c); // TODO 0 is to choose the constructor

      if (color == Color::wrong_char ()) return false;

      if (color.is_player ()) {
        play_player [play_cnt]  = color.to_player ();
        play_v [play_cnt]       = Vertex (row, col);
        play_cnt += 1;
        assertc (board_ac, play_cnt < board_area);
      }
    }

    if (getc_non_space (ifs) != '\n') return false;
  }

  Board  tmp_board [1];
  rep (pi, play_cnt) {
    if (tmp_board->is_strict_legal (play_player[pi], play_v[pi]) == false)
      return false;
    bool ret = tmp_board->try_play (play_player[pi], play_v[pi]);
    assertc (board_ac, ret == true);
  }

  this->load (tmp_board);

  return true;
}


void Board::clear () {
  set_komi (-0.5); // white wins the draws on default komi
  empty_v_cnt = 0;
  player_for_each (pl) {
    player_v_cnt  [pl] = 0;
    player_last_v [pl] = Vertex::any ();
  }
  move_no      = 0;
  last_player_ = Player::white (); // act player is other
  last_move_status = play_ok;
  ko_v_        = Vertex::any ();
  vertex_for_each_all (v) {
    color_at      [v] = Color::off_board ();
    nbr_cnt       [v] = NbrCounter::Empty();
    chain_next_v  [v] = v;
    chain_id_     [v] = v;      // TODO is it needed, is it used?
    chain_[v].lib_cnt = NbrCounter::max; // TODO off_boards?

    if (v.is_on_board ()) {
      color_at   [v]              = Color::empty ();
      empty_pos  [v]              = empty_v_cnt;
      empty_v    [empty_v_cnt++]  = v;

      vertex_for_each_4_nbr (v, nbr_v, {
        if (!nbr_v.is_on_board ()) {
          nbr_cnt [v].off_board_inc ();
        }
      });
    }
  }

  hash_ = recalc_hash ();

  check ();
}


Hash Board::recalc_hash () const {
  Hash new_hash;

  new_hash.set_zero ();

  vertex_for_each_all (v) {
    if (color_at [v].is_player ()) {
      new_hash ^= zobrist->of_pl_v (color_at [v].to_player (), v);
    }
  }

  return new_hash;
}

Board::Board () {
  clear ();
}

all_inline
void Board::load (const Board* save_board) {
  memcpy(this, save_board, sizeof(Board));
  check ();
}

all_inline
void Board::set_komi (float fkomi) {
  komi_ = int (ceil (fkomi));
}


all_inline
float Board::komi () const {
  return float(komi_) - 0.5;
}

all_inline
Vertex Board::ko_v () const {
  return ko_v_;
}

all_inline
Hash Board::hash () const {
  return hash_;
}

all_inline
bool Board::is_pseudo_legal (Player player, Vertex v) {
  check ();
  return
    v == Vertex::pass () ||
    !nbr_cnt[v].player_cnt_is_max (player.other ()) ||
    (!eye_is_ko (player, v) && !eye_is_suicide (v));
}


all_inline
bool Board::is_eyelike (Player player, Vertex v) {
  assertc (board_ac, color_at [v] == Color::empty ());
  if (! nbr_cnt[v].player_cnt_is_max (player)) return false;

  FastMap<Color, int> diag_color_cnt; // TODO
  color_for_each (col)
    diag_color_cnt [col] = 0;

  vertex_for_each_diag_nbr (v, diag_v, {
      diag_color_cnt [color_at [diag_v]]++;
    });

  return
    diag_color_cnt [Color (player.other ())] +
    (diag_color_cnt [Color::off_board ()] > 0) < 2;
}

flatten all_inline
void Board::play_legal (Player player, Vertex v) { // TODO test with move
  check ();

  if (v == Vertex::pass ()) {
    basic_play (player, Vertex::pass ());
    return;
  }

  v.check_is_on_board ();
  assertc (board_ac, color_at[v] == Color::empty ());

  if (nbr_cnt[v].player_cnt_is_max (player.other ())) {
    play_eye_legal (player, v);
  } else {
    play_not_eye (player, v);
    assertc (board_ac, last_move_status == play_ok ||
             last_move_status == play_suicide);
    // TODO invent complete suicide testing
  }
}


all_inline
bool Board::eye_is_ko (Player player, Vertex v) {
  return (v == ko_v_) & (player == last_player_.other ());
}


all_inline
bool Board::eye_is_suicide (Vertex v) {
  uint all_nbr_live = true;
  vertex_for_each_4_nbr (v, nbr_v, all_nbr_live &= (--chain_at(nbr_v).lib_cnt != 0));
  vertex_for_each_4_nbr (v, nbr_v, chain_at(nbr_v).lib_cnt += 1);
  return all_nbr_live;
}


all_inline
void Board::play_not_eye (Player player, Vertex v) {
  check ();
  v.check_is_on_board ();
  assertc (board_ac, color_at[v] == Color::empty ());

  basic_play (player, v);

  place_stone (player, v);

  vertex_for_each_4_nbr (v, nbr_v, {

      nbr_cnt [nbr_v].player_inc (player);

      if (color_at [nbr_v].is_player ()) {
        chain_at(nbr_v).lib_cnt -= 1;
        chain_at(nbr_v).lib_sum -= v.get_idx(); // 

        if (color_at [nbr_v] != Color (player)) { // same color of groups
          if (chain_at(nbr_v).lib_cnt == 0)
            remove_chain (nbr_v);
        } else {
          if (chain_id_ [nbr_v] != chain_id_ [v]) {
            if (chain_at(v).lib_cnt > chain_at(nbr_v).lib_cnt) {
              merge_chains (v, nbr_v);
            } else {
              merge_chains (nbr_v, v);
            }
          }
        }
      }
    });

  if (chain_at(v).lib_cnt == 0) {
    assertc (board_ac, last_empty_v_cnt - empty_v_cnt == 1);
    remove_chain(v);
    assertc (board_ac, last_empty_v_cnt - empty_v_cnt > 0);
    last_move_status = play_suicide;
  } else {
    last_move_status = play_ok;
  }
}


no_inline
void Board::play_eye_legal (Player player, Vertex v) {
  vertex_for_each_4_nbr (v, nbr_v, { // +++
    chain_at(nbr_v).lib_cnt -= 1; // +++
    chain_at(nbr_v).lib_sum -= v.get_idx(); // +++
  }); // +++
  // vertex_for_each_4_nbr (v, nbr_v, chain_at(nbr_v).lib_cnt -= 1); // ---

  basic_play (player, v);
  place_stone (player, v);

  vertex_for_each_4_nbr (v, nbr_v, {
      nbr_cnt [nbr_v].player_inc (player);
    });

  vertex_for_each_4_nbr (v, nbr_v, {
      if ((chain_at(nbr_v).lib_cnt == 0))
        remove_chain (nbr_v);
    });

  assertc (board_ac, chain_at(v).lib_cnt != 0);

  if (last_empty_v_cnt == empty_v_cnt) {
    // captured exactly one stone, end this was eye
    ko_v_ = empty_v [empty_v_cnt - 1]; // then ko formed
  } else {
    ko_v_ = Vertex::any ();
  }
}

// Warning: has to be called before place_stone, because of hash storing
void Board::basic_play (Player player, Vertex v) {
  assertc (board_ac, move_no <= max_game_length);
  ko_v_                   = Vertex::any ();
  last_empty_v_cnt        = empty_v_cnt;
  last_player_            = player;
  player_last_v [player]  = v;
  move_history [move_no]  = Move (player, v);
  move_no                += 1;
}


all_inline
void Board::merge_chains (Vertex v_base, Vertex v_new) {
  Vertex act_v;

  chain_at(v_base).lib_cnt += chain_at(v_new).lib_cnt;
  chain_at(v_base).lib_sum += chain_at(v_new).lib_sum; // +++

  act_v = v_new;
  do {
    chain_id_ [act_v] = chain_id_ [v_base];
    act_v = chain_next_v [act_v];
  } while (act_v != v_new);

  swap (chain_next_v[v_base], chain_next_v[v_new]);
}

no_inline
void Board::remove_chain (Vertex v) {
  Vertex act_v;
  Vertex tmp_v;
  Color old_color;

  old_color = color_at[v];
  act_v = v;

  assertc (board_ac, old_color.is_player ());

  do {
    remove_stone (act_v);
    act_v = chain_next_v[act_v];
  } while (act_v != v);

  assertc (board_ac, act_v == v);

  do {
    vertex_for_each_4_nbr (act_v, nbr_v, {
        nbr_cnt [nbr_v].player_dec (old_color.to_player());
        chain_at(nbr_v).lib_cnt += 1;
        chain_at(nbr_v).lib_sum += act_v.get_idx(); // +++
      });

    tmp_v = act_v;
    act_v = chain_next_v[act_v];
    chain_next_v [tmp_v] = tmp_v;

  } while (act_v != v);
}


all_inline
void Board::place_stone (Player pl, Vertex v) {
  hash_ ^= zobrist->of_pl_v (pl, v);
  player_v_cnt[pl]++;
  color_at[v] = Color (pl);

  empty_v_cnt--;
  empty_pos [empty_v [empty_v_cnt]] = empty_pos [v];
  empty_v [empty_pos [v]] = empty_v [empty_v_cnt];

  assertc (chain_next_v_ac, chain_next_v[v] == v);

  chain_id_ [v] = v;
  chain_at(v).lib_cnt = nbr_cnt[v].empty_cnt ();
  chain_[v].lib_sum = 0; // +++
  vertex_for_each_4_nbr(v, nbr, { // +++
    chain_[v].lib_sum += nbr.get_idx() & -(color_at[nbr] == Color::empty()); // +++
  });
}


all_inline
void Board::remove_stone (Vertex v) {
  hash_ ^= zobrist->of_pl_v (color_at [v].to_player (), v);
  player_v_cnt [color_at[v].to_player ()]--;
  color_at [v] = Color::empty ();

  empty_pos [v] = empty_v_cnt;
  empty_v [empty_v_cnt++] = v;
  chain_id_ [v] = v;

  assertc (board_ac, empty_v_cnt < Vertex::cnt);
}


all_inline
// TODO/FIXME last_player should be preserverd in undo function
Player Board::act_player () const {
  return last_player_.other ();
}

all_inline
Player Board::last_player () const {
  return last_player_;
}


all_inline
bool Board::both_player_pass () {
  return
    (player_last_v [Player::black ()] == Vertex::pass ()) &
    (player_last_v [Player::white ()] == Vertex::pass ());
}

all_inline
int Board::tt_score() const {
  FastMap<Player, int> score;
  player_for_each(pl) score[pl] = 0;

  player_for_each(pl) {
    FastStack<Vertex, board_area> queue;
    FastMap<Vertex, bool> visited;

    vertex_for_each_all(v) {
      visited[v] = false;
      if (color_at[v] == Color(pl)) {
        queue.push_back(v);
        visited[v] = true;
      }
    }

    while (!queue.empty()) {
      Vertex v = queue.pop_top();
      assertc (board_ac, visited[v]);
      score[pl] += 1;
      vertex_for_each_4_nbr(v, nbr, {
        if (!visited[nbr] && color_at[nbr] == Color::empty()) {
          queue.push_back(nbr);
          visited[nbr] = true;
        }
      });
    }
  }
  return komi_ + score[Player::black ()] - score[Player::white ()];
}

all_inline
int Board::tt_winner_score() const {
  int tmp = tt_score () > 0;
  return  tmp + tmp - 1;
}

all_inline
int Board::approx_score () const {
  return komi_ + player_v_cnt[Player::black ()] -  player_v_cnt[Player::white ()];
}

all_inline
int Board::score () const {
  int eye_score = 0;

  empty_v_for_each (this, v, {
      eye_score += nbr_cnt[v].player_cnt_is_max (Player::black ());
      eye_score -= nbr_cnt[v].player_cnt_is_max (Player::white ());
    });

  return approx_score () + eye_score;
}

all_inline
Player Board::approx_winner () const { 
  return Player (approx_score () <= 0); 
}


all_inline
Player Board::winner () const {
  return Player (score () <= 0);
}


all_inline
int Board::vertex_score (Vertex v) {
  //     FastMap<Color, int> Coloro_score;
  //     Coloro_score [Color::black ()] = 1;
  //     Coloro_score [Color::white ()] = -1;
  //     Coloro_score [Color::empty ()] =
  //       (nbr_cnt[v].player_cnt_is_max (Player::black ())) -
  //       (nbr_cnt[v].player_cnt_is_max (Player::white ()));
  //     Coloro_score [Color::off_board ()] = 0;
  //     return Coloro_score [color_at [v]];
  switch (color_at [v].get_idx ()) {
  case Color::black_idx: return 1;
  case Color::white_idx: return -1;
  case Color::empty_idx:
    return
      (nbr_cnt[v].player_cnt_is_max (Player::black ())) -
      (nbr_cnt[v].player_cnt_is_max (Player::white ()));
  case Color::off_board_idx: return 0;
  default: assert (false);
  }
}

all_inline
void Board::check_empty_v () const {
  if (!board_empty_v_ac) return;

  FastMap<Vertex, bool> noticed;
  FastMap<Player, uint> exp_player_v_cnt;

  vertex_for_each_all (v) noticed[v] = false;

  assert (empty_v_cnt <= board_area);

  empty_v_for_each (this, v, {
      assert (noticed [v] == false);
      noticed [v] = true;
    });

  player_for_each (pl) exp_player_v_cnt [pl] = 0;

  vertex_for_each_all (v) {
    assert ((color_at[v] == Color::empty ()) == noticed[v]);
    if (color_at[v] == Color::empty ()) {
      assert (empty_pos[v] < empty_v_cnt);
      assert (empty_v [empty_pos[v]] == v);
    }
    if (color_at [v].is_player ()) exp_player_v_cnt [color_at[v].to_player ()]++;
  }

  player_for_each (pl)
    assert (exp_player_v_cnt [pl] == player_v_cnt [pl]);
}

all_inline
Board::Chain& Board::chain_at (Vertex v) {
  return chain_[chain_id_[v]];
}


/* +++ */
all_inline
void Board::pseudo_atari (FastMap<Player, Vertex>* atari_v) {
  player_for_each(pl) (*atari_v)[pl] = Vertex::any();

  if (move_no == 0) return;

  Vertex last_v = move_history[move_no-1].get_vertex();
  if (last_v == Vertex::pass()) return;

  vertex_for_each_5_nbr(last_v, vv, {
    if (color_at[vv].is_player() && chain_at(vv).lib_cnt == 1)
      (*atari_v)[color_at[vv].to_player()] = Vertex(chain_at(vv).lib_sum);
  });
}

all_inline
void Board::find_recent_atari (
		Vertex *blacks, Vertex *whites, uint &blackc, uint &whitec, uint len)
{	
	blackc = whitec = 0;

	for (uint i = 1; i <= 3; ++i) {
		if (i >= move_no) break;
		Vertex center(move_history[move_no-i].get_vertex());
		if (center == Vertex::pass()) continue;

		vertex_for_each_5_nbr(center, v, {
			if (v.is_correct() && color_at[v].is_player()) {
				Vertex cid = chain_id_[v];

#ifdef CHECK_ATARI_DEEPLY
				{ // ~2x wolniejsze !! (wg benchmarkow)
					Vertex atari_v = in_atari(cid);
					if (color_at[v] == Color::black()) {
						if (blackc < len) blacks[blackc++] = atari_v;
					} else {
						if (whitec < len) whites[whitec++] = atari_v;
					}
				}
#else 
				if (chain_[cid].lib_cnt == 1) { // 100% atari
					if (color_at[v] == Color::black()) {
						if (blackc < len) blacks[blackc++] = Vertex(chain_[cid].lib_sum);
					} else {
						if (whitec < len) whites[whitec++] = Vertex(chain_[cid].lib_sum);
					}
				}
#endif
			}
		});
	}
}

all_inline
void Board::find_all_atari (
		Vertex *blacks, Vertex *whites, uint &blackc, uint &whitec, uint len)
{
	// FastMap-a , zeby markowac, jakie lancuchy juz przeanalizowalismy.
	// - powinno przyspieszyc sprawdzanie, wtedy wykonujemy jedno 
	// sprawdzenie dla calej grupy kamieni. 
	FastMap<Vertex, int> chains;
	blackc = whitec = 0;

	vertex_for_each_all (v) { chains[v] = 0; }

	//- Szukamy wszystkich sytuacji atari

	vertex_for_each_all (v) {
		Vertex cid = chain_id_[v];

		if (!chains[cid] && color_at[cid].is_player()) {
			Vertex atari_v = in_atari(cid);
					
			if (atari_v != Vertex::any()) {
				if (color_at[cid] == Color::black()) {
					if (blackc < len) blacks[blackc++] = atari_v;
				} else {
					if (whitec < len) whites[whitec++] = atari_v;
				}
			}
		}

		chains[cid] = 1;
	}
}

all_inline
Vertex Board::in_atari(Vertex group) {
	// Czy to w ogole na planszy, czy tam w ogole jest jakis kamien?
	if (!group.is_correct() || !color_at[group].is_player()) return Vertex::any();

	Vertex cid = chain_id_[group]; // ID lancucha kamieni

	// jak == 1 to lib_sum bedzie od razu wskazywac na atari
	if (chain_[cid].lib_cnt == 1) return Vertex(chain_[cid].lib_sum);

	// wpp musimy poszperac...
	if (chain_[cid].lib_cnt > 0 && !(chain_[cid].lib_sum % chain_[cid].lib_cnt)) {
		// lib_sum jest wielokrotnoscia lib_cnt, jest szansa, ze lib_sum/lib_cnt jest atari
		Vertex atari_v (chain_[cid].lib_sum/chain_[cid].lib_cnt);
		
		// ... o ile jest na planszy i jest to puste pule 
		if (atari_v.is_on_board() && color_at[atari_v] == Color::empty()) {
			Vertex x;
			uint cnt = 0;
			
			// i o ile graniczy ono ze wskazana grupa.
			// ID lancucha jest unikalne, nie trzeba sprawdzac jaki gracz itp...
			if ((x = atari_v.N()).is_correct() && chain_id_[x] == cid) ++cnt;
			if ((x = atari_v.W()).is_correct() && chain_id_[x] == cid) ++cnt;
			if ((x = atari_v.S()).is_correct() && chain_id_[x] == cid) ++cnt;
			if ((x = atari_v.E()).is_correct() && chain_id_[x] == cid) ++cnt;

			// Liczniki sie zgadzaja, mamy atari!
			if (cnt == chain_[cid].lib_cnt) return atari_v;
		}
	}

	// wpp...
	return Vertex::any(); 
}

all_inline
Vertex Board::in_atari_fast(Vertex group) {
	if (!group.is_correct() || !color_at[group].is_player()) return Vertex::any();

	Vertex cid = chain_id_[group]; // ID lancucha kamieni

	// jak == 1 to lib_sum bedzie od razu wskazywac na atari
	if (chain_[cid].lib_cnt == 1) return Vertex(chain_[cid].lib_sum);

	return Vertex::any();
}

//all_inline
Player Board::is_ladder(Vertex atari, Player p) {
	//- Player p jest w atari, sprawdzimy, czy jesli bedzie
	//- probowal sie bronic to sie uchroni
	//- return p; - jesli p ucieknie
	//- return p.other(); - jesli p zostanie zbite
	
	Player winner = p.other();
	Board board[1];
	board->load(this);
	
	assert(atari.is_correct());
	assert(board->color_at[atari] == Color::empty());
	if (!board->chain_[board->chain_id_[atari]].lib_cnt) return p.other(); // TODO : Wcale niekoniecznie !!

	if (!board->is_pseudo_legal(p, atari)) return p;
	board->play_legal(p, atari);
	
	// ehh, czasami(czesto?) lib_cnt zlicza jedno pole wiele razy...

	Vertex cid = board->chain_id_[atari];
	if (board->chain_[cid].lib_cnt >= 3) return p;         // ucieczka
	if (board->chain_[cid].lib_cnt == 1) return p.other(); // zbicie

	assert(board->chain_[cid].lib_cnt == 2); // Nie 0, bo nie bylo samobojstwa :P

	// Zostaly dwie mozliwosci do sprawdzenia. I albo sa nieopodal 
	// ostatnio wykonanego ruchu, albo tylko jedno z nich, albo zadne.
	// W dwoch ostatnich przypadkach trzeba rozwazyc mozliwosc 
	// polaczenia sie z inna grupa kamieni.

	// Warto sprawdzic czy ucieczka z atari nie spowodowala atari 
	// dla drugiego gracza, jak tak to mozna uznac, ze gracz uciekl.

	Color pcolor = (p == Player::black() ? Color::white():Color::black());
	Vertex x;

	if (((x = atari.N()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()) ||
			((x = atari.W()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()) ||
			((x = atari.S()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()) ||
			((x = atari.E()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()))
		return p;

	// Teraz analiza pozostalych dwoch mozliwosci (1 oddech zabierzemy, 2 to nowe atari)

	Vertex natari[4];
	uint natari_cnt = 0;

	if ((x = atari.N()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;
	if ((x = atari.W()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;
	if ((x = atari.S()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;
	if ((x = atari.E()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;

	if (natari_cnt != 2) {
		if (natari_cnt == 0) {
			// Moze byc tak, ze ostatnim ruchem zabralismy ostatni oddech oraz 
			// polaczylismy sue z innym lancuchem, ktory ma jeszcze dwa inne oddechy. 
			// TODO : Sytuacja rzadka - uznaje, ze sie uda uciec...
			return p;
		} else { // natari == 1
			assert(natari_cnt == 1);
			// Moze byc tak, ze jedno pole jest liczone dwukrotnie.
			// Wtedy w jednym ruchu zbijamy!
			if (2*natari[0].get_idx() == board->chain_[cid].lib_sum) return p.other();

			// Moze byc tak gdy atakowany lancuch znienacka polaczy sie z innym a 
			// ten ma oddechy gdzies dalej, dalej... Jednak - o ile nie wykonalismy
			// samobojczego ruchu - powinnismy wykryc jeden wolny oddech, a poniewaz
			// grupa ma tylko dwa oddechy, drugi bedzie juz latwo ustalic.
		
			natari[1] = Vertex(board->chain_[cid].lib_sum - natari[0].get_idx());
			++natari_cnt;
		}
	}

	Board nboard[1];
	nboard->load(board);

	if (nboard->is_pseudo_legal(p.other(), natari[0])) {
		nboard->play_legal(p.other(), natari[0]);
		if (nboard->is_ladder(natari[1], p) != p) return p.other();
	}

	nboard->load(board);

	if (nboard->is_pseudo_legal(p.other(), natari[1])) {
		nboard->play_legal(p.other(), natari[1]);
		if (nboard->is_ladder(natari[0], p) != p) return p.other();
	}

	// ^^^ Wyglada wykladniczo ale chyba zawsze jedna z drog sie szybko zwija

	return p; // Z kazdej ewentualnosci uciekamy :)
}

all_inline
Player Board::is_ladder_norec(Vertex atari0, Player p)
{
	static Board boards[50];  // UWAGA na watki !!
	static Vertex ataris[50]; // UWAGA na watki !!
	uint stack_len = 1;
	boards[0].load(this);
	ataris[0] = atari0;
	
	//return p.other();

	// Na stos odkladamy kolejno mozliwe ucieczki gracza p.
	// jesli wszystkie sie powioda - ucieka. Wpp przeciwnik
	// wybierze akurat te mozliwosc, gdzie zbija uciekiniera.

	while (stack_len > 0) {
		--stack_len;
		Board *board = boards + stack_len;
		Vertex atari = ataris[stack_len];
		
		assert(atari.is_correct());
		assert(board->color_at[atari] == Color::empty());
		if (!board->chain_[board->chain_id_[atari]].lib_cnt) return p.other(); // TODO : Wcale niekoniecznie !!

		if (!board->is_pseudo_legal(p, atari)) continue; // tym razem ucieklo sie...
		board->play_legal(p, atari);
		
		// ehh, czasami(czesto?) lib_cnt zlicza jedno pole wiele razy...

		Vertex cid = board->chain_id_[atari];
		if (board->chain_[cid].lib_cnt >= 3) continue; // tym razem ucieklo sie...
		if (board->chain_[cid].lib_cnt == 1) return p.other(); // zbicie

		assert(board->chain_[cid].lib_cnt == 2); // Nie 0, bo nie bylo samobojstwa :P

		// Zostaly dwie mozliwosci do sprawdzenia. I albo sa nieopodal 
		// ostatnio wykonanego ruchu, albo tylko jedno z nich, albo zadne.
		// W dwoch ostatnich przypadkach trzeba rozwazyc mozliwosc 
		// polaczenia sie z inna grupa kamieni.

		// Warto sprawdzic czy ucieczka z atari nie spowodowala atari 
		// dla drugiego gracza, jak tak to mozna uznac, ze gracz uciekl.

		Color pcolor = (p == Player::black() ? Color::white():Color::black());
		Vertex x;

		if (((x = atari.N()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()) ||
				((x = atari.W()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()) ||
				((x = atari.S()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()) ||
				((x = atari.E()).is_correct() && board->color_at[x] == pcolor && board->in_atari(x) != Vertex::any()))
			continue; // tym razem ucieklo sie...

		// Teraz analiza pozostalych dwoch mozliwosci (1 oddech zabierzemy, 2 to nowe atari)

		Vertex natari[4];
		uint natari_cnt = 0;

		if ((x = atari.N()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;
		if ((x = atari.W()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;
		if ((x = atari.S()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;
		if ((x = atari.E()).is_correct() && board->color_at[x] == Color::empty()) natari[natari_cnt++] = x;

		if (natari_cnt != 2) {
			if (natari_cnt == 0) {
				// Moze byc tak, ze ostatnim ruchem zabralismy ostatni oddech oraz 
				// polaczylismy sue z innym lancuchem, ktory ma jeszcze dwa inne oddechy. 
				// TODO : Sytuacja rzadka - uznaje, ze sie uda uciec...
				continue; // tym razem ucieklo sie...
			} else { // natari == 1
				assert(natari_cnt == 1);
				// Moze byc tak, ze jedno pole jest liczone dwukrotnie.
				// Wtedy w jednym ruchu zbijamy!
				if (2*natari[0].get_idx() == board->chain_[cid].lib_sum) return p.other();

				// Moze byc tak gdy atakowany lancuch znienacka polaczy sie z innym a 
				// ten ma oddechy gdzies dalej, dalej... Jednak - o ile nie wykonalismy
				// samobojczego ruchu - powinnismy wykryc jeden wolny oddech, a poniewaz
				// grupa ma tylko dwa oddechy, drugi bedzie juz latwo ustalic.
			
				natari[1] = Vertex(board->chain_[cid].lib_sum - natari[0].get_idx());
				++natari_cnt;
			}
		}

		if (stack_len == 48) return p.other(); // :P

		//boards[stack_len] = board[0] // to jest to samo !!!
		boards[stack_len+1] = boards[stack_len];

		if (boards[stack_len].is_pseudo_legal(p.other(), natari[0])) {
			ataris[stack_len] = natari[1];
			boards[stack_len].play_legal(p.other(), natari[0]);
			++stack_len; ++board;
		}

		if (boards[stack_len].is_pseudo_legal(p.other(), natari[1])) {
			ataris[stack_len] = natari[0];
			boards[stack_len].play_legal(p.other(), natari[1]);
			++stack_len; // ++board;
		}

		// ^^^ Wyglada wykladniczo ale chyba zawsze jedna z drog sie szybko zwija
	}

	return p; // Ok, gracz p ucieknie z atari
}


/* +++ */

all_inline
uint Board::last_capture_size () {
  return empty_v_cnt + 1 - last_empty_v_cnt;
}

// -----------------------------------------------------------------------------

all_inline
void Board::check_hash () const {
  assertc (board_hash_ac, hash_ == recalc_hash ());
}


all_inline
void Board::check_color_at () const {
  if (!board_color_at_ac) return;

  vertex_for_each_all (v) {
    assert ((color_at[v] != Color::off_board()) == (v.is_on_board ()));
  }
}


all_inline
void Board::check_nbr_cnt () const {
  if (!board_nbr_cnt_ac) return;

  vertex_for_each_all (v) {
    FastMap<Color, uint> nbr_color_cnt;
    if (color_at[v] == Color::off_board()) continue; // TODO is that right?

    color_for_each (col) {
      nbr_color_cnt [col] = 0;
    }
    vertex_for_each_4_nbr (v, nbr_v, {
        nbr_color_cnt [color_at [nbr_v]]++;
      });

    nbr_cnt[v].check(nbr_color_cnt);
  }
}


all_inline
void Board::check_chain_at () const {
  if (!chain_at_ac) return;

  vertex_for_each_all (v) {
    // whether same color neighbours have same root and liberties
    // TODO what about off_board and empty?
    if (color_at [v].is_player ()) {

      assert (chain_[chain_id_[v]].lib_cnt != 0);
      assert (chain_[chain_id_[v]].lib_sum != 0); // +++

      if (chain_[chain_id_[v]].lib_cnt == 1) { // +++
        Vertex atari = Vertex(chain_[chain_id_[v]].lib_sum); // +++
        assert (color_at[atari] == Color::empty()); // +++
        uint my_nbr_cnt = 0; // +++
        vertex_for_each_4_nbr (atari, nbr, { // +++
            my_nbr_cnt += chain_id_ [v] == chain_id_[nbr]; // +++
        }); // +++
        assert (my_nbr_cnt == 1);
      }

      vertex_for_each_4_nbr (v, nbr_v, {
          if (color_at[v] == color_at[nbr_v])
            assert (chain_id_ [v] == chain_id_ [nbr_v]);
        });
    }
  }
}


all_inline
void Board::check_chain_next_v () const {
  if (!chain_next_v_ac) return;
  vertex_for_each_all (v) {
    chain_next_v[v].check ();
    if (!color_at [v].is_player ())
      assert (chain_next_v [v] == v);
  }
}


all_inline
void Board::check () const {
  if (!board_ac) return;

  check_empty_v       ();
  check_hash          ();
  check_color_at      ();
  check_nbr_cnt       ();
  check_chain_at      ();
  check_chain_next_v  ();
}


all_inline
void Board::check_no_more_legal (Player player) { // at the end of the playout
  unused (player);

  if (!board_ac) return;

  vertex_for_each_all (v)
    if (color_at[v] == Color::empty ())
      assert (is_eyelike (player, v) || is_pseudo_legal (player, v) == false);
}

const Zobrist Board::zobrist[1] = { Zobrist (global_random) };
