/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
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
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "stat.h"

// uct parameters


// ----------------------------------------------------------------------
class Node {
public:
  Stat stat;
#ifdef USE_PLAYOUT_VIEW
	PlayoutView *view; // Trzymamy wskaznik, zeby nie alokowac zbyt wiele pamieci
#endif

  Player player;
  Vertex v;

  // TODO this should be replaced by Stat
  FastMap<Vertex, Node*> children;
  bool have_child;

public:
	~Node(void) { 
#ifdef USE_PLAYOUT_VIEW
		if (view) delete view; 
#endif
	}

  #define node_for_each_child(node, act_node, i) do {       \
    assertc (tree_ac, node!= NULL);                         \
    Node* act_node;                                         \
    vertex_for_each_all (v) {                               \
      act_node = node->children[v];                         \
      if (act_node == NULL) continue;                       \
      i;                                                    \
    }                                                       \
  } while (false)
  
  void init (Player pl, Vertex v) {
    this->player = pl;
    this->v = v;
    vertex_for_each_all (v) {
      children[v] = NULL;
		}
    have_child = false;

#ifdef USE_PLAYOUT_VIEW
		view = NULL; // TODO : Czy mi tu jakas pamiec nie cieknie ??
#endif
  }

  void add_child (Node* new_child) { // TODO sorting?
    have_child = true;
    // TODO assert
    children[new_child->v] = new_child;
  }

  void remove_child (Node* del_child) { // TODO inefficient
    assertc (tree_ac, del_child != NULL);
    children[del_child->v] = NULL;
  }

  bool no_children () {
    return !have_child;
  }

  Node* find_uct_child (Board *b, float explore_rate) {
    Node* best_child = NULL;
    float best_urgency = -large_float;
    float explore_coeff = log (stat.update_count()) * explore_rate;
    
#ifdef USE_UCT_LOCALITY 
		float factor_factor1 = 1.1, 
					l11 = 1.1*factor_factor1, 
					l12 = 1.1*factor_factor1, 
					l13 = 2.0*factor_factor1;
		float factor_factor2 = 1.1, 
					l21 = 1.1*factor_factor2, 
					l22 = 1.1*factor_factor2, 
					l23 = 2.0*factor_factor2;
		FastMap<Vertex, float> factor;
		vertex_for_each_all(v) { factor[v] = 1.; }

		if (b->move_no > 1) {
			Vertex center1 = b->move_history[b->move_no-1].get_vertex();
			Vertex center2 = b->move_history[b->move_no-2].get_vertex();
			Vertex x;
			
			if ((x = center1.NN()).is_on_board()) factor[x] = l11;
			if ((x = center1.SS()).is_on_board()) factor[x] = l11;
			if ((x = center1.WW()).is_on_board()) factor[x] = l11;
			if ((x = center1.EE()).is_on_board()) factor[x] = l11;
			
			if ((x = center1.NW()).is_on_board()) factor[x] = l12;
			if ((x = center1.NE()).is_on_board()) factor[x] = l12;
			if ((x = center1.SW()).is_on_board()) factor[x] = l12;
			if ((x = center1.SE()).is_on_board()) factor[x] = l12;
			
			if ((x = center1.N ()).is_on_board()) factor[x] = l13;
			if ((x = center1.E ()).is_on_board()) factor[x] = l13;
			if ((x = center1.W ()).is_on_board()) factor[x] = l13;
			if ((x = center1.S ()).is_on_board()) factor[x] = l13;

			if ((x = center1.NN()).is_on_board() && factor[x] < l21) factor[x] = l21;
			if ((x = center1.SS()).is_on_board() && factor[x] < l21) factor[x] = l21;
			if ((x = center1.WW()).is_on_board() && factor[x] < l21) factor[x] = l21;
			if ((x = center1.EE()).is_on_board() && factor[x] < l21) factor[x] = l21;
			
			if ((x = center1.NW()).is_on_board() && factor[x] < l22) factor[x] = l22;
			if ((x = center1.NE()).is_on_board() && factor[x] < l22) factor[x] = l22;
			if ((x = center1.SW()).is_on_board() && factor[x] < l22) factor[x] = l22;
			if ((x = center1.SE()).is_on_board() && factor[x] < l22) factor[x] = l22;
			
			if ((x = center1.N ()).is_on_board() && factor[x] < l23) factor[x] = l23;
			if ((x = center1.E ()).is_on_board() && factor[x] < l23) factor[x] = l23;
			if ((x = center1.W ()).is_on_board() && factor[x] < l23) factor[x] = l23;
			if ((x = center1.S ()).is_on_board() && factor[x] < l23) factor[x] = l23;
		}
#else
		(void)b;
#endif

    vertex_for_each_all(x) {
      Node* child = children[x];
      if (child == NULL) continue;

      float child_urgency = 
#ifdef USE_PLAYOUT_VIEW
				( (view) ? (*view)[x] : 0 ) + 
#endif
				child->stat.ucb (
						child->player, 
#ifdef USE_UCT_LOCALITY
						factor[x] * 
#endif	
						explore_coeff);
      
			if (child_urgency > best_urgency) {
        best_urgency  = child_urgency;
        best_child    = child;
      }
    }

    assertc (tree_ac, best_child != NULL); // at least pass
    return best_child;
  }

  Node* find_most_explored_child () {
    Node* best_child = NULL;
    float best_update_count = -1;

    vertex_for_each_all(v) {
      Node* child = children[v];
      if (child == NULL) continue;
      if (child->stat.update_count() > best_update_count) {
        best_update_count = child->stat.update_count();
        best_child       = child;
      }
    }

    assertc (tree_ac, best_child != NULL);
    return best_child;
  }

  void rec_print (ostream& out, uint depth,
                  float min_visit, float min_visit_parent) {
    rep (d, depth) out << "  ";
    out 
      << player.to_string () << " " 
      << v.to_string () << " " 
      << stat.to_string() << " "
      << endl;

    rec_print_children (out, depth, min_visit, min_visit_parent);
  }

  void rec_print_children (ostream& out, uint depth, 
                           float min_visit, float min_visit_parent) {
    Node*  child_tab [Vertex::cnt]; // rough upper bound for the number of legal move
    uint     child_tab_size;
    uint     best_child_idx;
    float    min_visit_cnt;
    
    child_tab_size  = 0;
    best_child_idx  = 0;
    min_visit_cnt   =
      min_visit + 
      stat.update_count() * min_visit_parent; 
    // we want to be visited at least some percentage of parent's visit_cnt

    // prepare for selection sort
    node_for_each_child (this, child, child_tab [child_tab_size++] = child);

    #define best_child child_tab [best_child_idx]

    while (child_tab_size > 0) {
      // find best child
      rep(ii, child_tab_size) {
        if ((player == Player::black ()) == 
            (best_child->stat.mean() > child_tab [ii]->stat.mean()))
          best_child_idx = ii;
      }
      // rec call
      if (best_child->stat.update_count() >= min_visit_cnt)
        child_tab [best_child_idx]->rec_print (out, depth + 1,
                                               min_visit, min_visit_parent);
      else break;

      // remove best
      best_child = child_tab [--child_tab_size];
    }

    #undef best_child
    
  }
};



// class Tree

class Tree {

  static const uint uct_max_depth = 1000;
  static const uint uct_max_nodes = 1000000;

public:

  FastPool <Node> node_pool;
  Node*           history [uct_max_depth];
  uint            history_top;

public:

  Tree () : node_pool(uct_max_nodes) {
  }

  void init (Player pl) {
    node_pool.reset();
    history [0] = node_pool.malloc ();
    history [0]->init (pl.other(), Vertex::any ());
    history_top = 0;
  }

  void history_reset () {
    history_top = 0;
  }
  
  Node* act_node () {
    return history [history_top];
  }
  
  void uct_descend (Board *board, float explore_rate) {
    history [history_top + 1] = act_node ()->find_uct_child (board, explore_rate);
    history_top++;
    assertc (tree_ac, act_node () != NULL);
  }
  
  void alloc_child (Vertex v) {
    Node* new_node;
    new_node = node_pool.malloc ();
    new_node->init (act_node()->player.other(), v);
    act_node ()->add_child (new_node);
  }
  
  void delete_act_node () {
    assertc (tree_ac, act_node ()->no_children ());
    assertc (tree_ac, history_top > 0);
    history [history_top-1]->remove_child (act_node ());
    node_pool.free (act_node ());
  }
  
  void free_subtree (Node* parent) {
    node_for_each_child (parent, child, {
      free_subtree (child);
      node_pool.free (child);
    });
  }

  // TODO free history (for sync with base board)
  
#ifdef USE_PLAYOUT_VIEW
  void update_history (float sample, Board *board) {
    rep (hi, history_top+1) {
       history [hi]->stat.update (sample);
			 if (!history [hi]->view) history [hi]->view = new PlayoutView();
			 history [hi]->view->update (board, board->winner());
		}
  }
#else
	void update_history (float sample) {
    rep (hi, history_top+1) {
       history [hi]->stat.update (sample);
		}
  }
#endif

  string to_string (float min_visit, float min_visit_parent) { 
    ostringstream out_str;
    history [0]->rec_print (out_str, 0, min_visit, min_visit_parent); 
    return out_str.str ();
  }
};


 // class Uct


class Uct {
public:

  float explore_rate;
  uint  uct_genmove_playout_cnt;
  float mature_update_count_threshold;

  float min_visit;
  float min_visit_parent;

  float resign_mean;

  Board&        base_board;
  Tree          tree;      // TODO sync tree->root with base_board
  ExtPolicy  policy;

  Board play_board;
  
public:
  
  Uct (Board& base_board_) : base_board (base_board_), policy(global_random) { 
    explore_rate                   = 1.0;
    uct_genmove_playout_cnt        = 100000;
    mature_update_count_threshold  = 100.0;

    min_visit         = 500.0;
    min_visit_parent  = 0.02;

    resign_mean = 0.95;
  }

  void root_ensure_children_legality (Player pl) {
    // cares about superko in root (only)
    tree.init(pl);

    assertc (uct_ac, tree.history_top == 0);
    assertc (uct_ac, tree.act_node ()->no_children());

    empty_v_for_each_and_pass (&base_board, v, {
      if (base_board.is_strict_legal (pl, v))
        tree.alloc_child (v);
    });
  }

  flatten 
  void do_playout (Player first_player){
    Player act_player = first_player;
    Vertex v;
    
    play_board.load (&base_board);
    tree.history_reset ();
    
    do {
      if (tree.act_node ()->no_children ()) { // we're finishing it
        
        // If the leaf is ready expand the tree -- add children - 
        // all potential legal v (i.e.empty)
        if (tree.act_node()->stat.update_count() >
            mature_update_count_threshold) 
        {
          empty_v_for_each_and_pass (&play_board, v, {
            tree.alloc_child (v); // TODO simple ko should be handled here
            // (suicides and ko recaptures, needs to be dealt with later)
          });
          continue;            // try again
        }
        
        Playout<ExtPolicy> (&policy, &play_board).run ();

        int score = play_board.winner().get_idx (); // black -> 0, white -> 1
			
				// black -> 1, white -> -1
#ifdef USE_PLAYOUT_VIEW
				if (!tree.act_node()->view) 
					tree.act_node()->view = new PlayoutView();
	
				tree.update_history (1 - score - score, &play_board); 
#else
				tree.update_history (1 - score - score); 
#endif
        return;
      }
      
      tree.uct_descend (&play_board, explore_rate);
      v = tree.act_node ()->v;
      
      if (play_board.is_pseudo_legal (act_player, v) == false) {
        tree.delete_act_node ();
        return;
      }
      
      play_board.play_legal (act_player, v);

      if (play_board.last_move_status != Board::play_ok) {
        tree.delete_act_node ();
        return;
      }

      act_player = act_player.other();

      if (play_board.both_player_pass()) {
#ifdef USE_PLAYOUT_VIEW
        tree.update_history (play_board.tt_winner_score(), &play_board);
#else
        tree.update_history (play_board.tt_winner_score());
#endif
        return;
      }

    } while (true);
    
  }
  

  Vertex genmove (Player player) {

    root_ensure_children_legality (player);

    rep (ii, uct_genmove_playout_cnt) do_playout (player);
    
		Node* best = tree.history [0]->find_most_explored_child ();
    assertc (uct_ac, best != NULL);

    cerr << tree.to_string (min_visit, min_visit_parent) << endl;
    if ((player == Player::black () && best->stat.mean() < -resign_mean) ||
        (player == Player::white () && best->stat.mean() >  resign_mean)) {
      return Vertex::resign ();
    }
    return best->v;
  }
  
};
