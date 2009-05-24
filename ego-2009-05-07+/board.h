#ifndef _BOARD_H_
#define _BOARD_H_

#include "utils.h"
#include "hash.h"
#include "color.h"

// TODO move these to Board
const uint  board_area         = board_size * board_size;
const uint  max_empty_v_cnt    = board_area;
const uint  max_game_length    = board_area * 4;

class Board {
public:                         // board interface

  Board ();

  /* slow game functions */

  void clear ();


  float komi () const;
  void set_komi (float fkomi);

  Hash hash () const;

  Vertex ko_v () const;

  bool is_strict_legal (Player pl, Vertex v);

  bool try_play (Player player, Vertex v);

  bool undo ();

  bool is_hash_repeated ();

  /* playout functions */ 

  // can be use to initialize playout board
  void load (const Board* save_board);

  // v has to point to empty vertex
  // can't recognize play_suicide, will recognize ko
  bool is_pseudo_legal (Player player, Vertex v);

  bool is_eyelike (Player player, Vertex v);

  // accept pass
  // will ignore simple-ko ban
  // will play single stone suicide
  void play_legal (Player player, Vertex v);

  Player act_player () const;
  Player last_player () const;

  bool both_player_pass ();

  /* scoring functions */

  // scoring uses integers, so to get a true result you need to
  // substract 0.5 (white wins when score == 0)

  // returns 1 (-1) if v is occupied by or is an eye of Black(White)
  // returns 0 otherwise
  int vertex_score (Vertex v);

  // Tromp-Taylor score.
  int tt_score() const;
  int tt_winner_score() const;

  // Difference in (number of stones + number of eyes) of each player + komi.
  int score () const;
  Player winner () const;

  // Difference in (number of stones) of each player + komi.
  int approx_score () const;
  Player approx_winner () const;

  /* auxiliary functions */

  bool load_from_ascii (istream& ifs);
  string to_string (Vertex mark_v = Vertex::any ()) const;

  // debugging helper
  void print_cerr (Vertex v = Vertex::pass ()) const;

  void pseudo_atari (FastMap<Player, Vertex>* atari_v);
	void find_all_atari (
			Vertex *blacks, Vertex *whites, 
			uint &blackc, uint &whitec, uint len); 
	void find_recent_atari (
			Vertex *blacks, Vertex *whites, 
			uint &blackc, uint &whitec, uint len);
	
	// Sprawdzamy czy atari prowadzi do drabinki i 
	// kto ja wygrywa... Zawierzamy, ze atari to 
	// istotnie jest atari !!!
	Player is_ladder(Vertex atari, Player p);
	Player is_ladder_norec(Vertex atari, Player p);

	// Sprawdzenie czy grupa kamieni jest w atari
	// = Vertex::any() jesli nie.
	Vertex in_atari(Vertex group);
	Vertex in_atari_fast(Vertex group);// Szybkie i obarczone wiekszym bledem

  uint last_capture_size ();

private: 
  Hash recalc_hash () const;

  bool eye_is_ko (Player player, Vertex v);
  bool eye_is_suicide (Vertex v);

  void basic_play (Player player, Vertex v);
  void play_not_eye (Player player, Vertex v);
  void play_eye_legal (Player player, Vertex v);

  void merge_chains (Vertex v_base, Vertex v_new);
  void remove_chain (Vertex v);
  void place_stone (Player pl, Vertex v);
  void remove_stone (Vertex v);


  // TODO: move these consistency checks to some some kind of unit testing
  void check_empty_v () const;
  void check_hash () const;
  void check_color_at () const;
  void check_nbr_cnt () const;
  void check_chain_at () const;
  void check_chain_next_v () const;
  void check () const;
  void check_no_more_legal (Player player);

  class NbrCounter {
  public:
    static NbrCounter Empty();

    void player_inc (Player player);
    void player_dec (Player player);
    void off_board_inc ();
    uint empty_cnt  () const;
    uint player_cnt (Player pl) const;
    uint player_cnt_is_max (Player pl) const;
    void check ();
    void check (const FastMap<Color, uint>& nbr_color_cnt) const;

    static const uint max;    // maximal number of neighbours

  private:
    uint bitfield;

    static NbrCounter OfCounts(uint black_cnt, uint white_cnt, uint empty_cnt);

    static const uint f_size; // size in bits of each of 3 counters in nbr_cnt::t
    static const uint player_inc_tab [Player::cnt];
    static const uint f_shift [3];
    static const uint player_cnt_is_max_mask [Player::cnt];
  };

  struct Chain {
    uint lib_cnt;
    uint lib_sum;
  };

	// ---
public:
	Chain& chain_at (Vertex v); // +++
  
	// TODO make iterators / accessors
  FastMap<Vertex, Color>   color_at;

  Vertex                   empty_v [board_area]; // TODO use FastSet (empty_pos)
  uint                     empty_v_cnt;

  Move                     move_history [max_game_length]; // TODO FastStack
  uint                     move_no;

  enum {
    play_ok,
    play_suicide,
    play_ss_suicide,
    play_ko
  } last_move_status;

private:
  int komi_;

  static const Zobrist zobrist[1];

  FastMap<Vertex, NbrCounter>  nbr_cnt; // incremental, for fast eye checking
  FastMap<Vertex, uint>        empty_pos; // liberty position in empty_v
  FastMap<Vertex, Vertex>      chain_next_v;

  FastMap<Vertex, Vertex>      chain_id_;
  FastMap<Vertex, Chain>       chain_; // indexed by chain_id[v]

  uint                         last_empty_v_cnt;
  FastMap<Player, uint>        player_v_cnt;
  FastMap<Player, Vertex>      player_last_v;
  Hash                         hash_;
  Vertex                       ko_v_;             // vertex forbidden by ko
  Player                       last_player_;      // player who made the last play
};

#define empty_v_for_each(board, vv, i) {                                \
    Vertex vv = Vertex::any ();                                         \
    rep (ev_i, (board)->empty_v_cnt) {                                  \
      vv = (board)->empty_v [ev_i];                                     \
      i;                                                                \
    }                                                                   \
  }

#define empty_v_for_each_and_pass(board, vv, i) {                       \
    Vertex vv = Vertex::pass ();                                        \
    i;                                                                  \
    rep (ev_i, (board)->empty_v_cnt) {                                  \
      vv = (board)->empty_v [ev_i];                                     \
      i;                                                                \
    }                                                                   \
  }

#endif
