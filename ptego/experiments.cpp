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

// ----------------------------------------------------------------------

class AafStats {
public:
  Stat unconditional;
  FastMap<Move, Stat> given_move;

  void reset (float prior = 1.0) {
    unconditional.reset (prior);
    move_for_each_all (m)
      given_move [m].reset (prior);
  }

  void update (Move* move_history, uint move_count, float score) {
    unconditional.update (score);
    rep (ii, move_count)
      given_move [move_history [ii]].update (score);
  }

  float norm_mean_given_move (const Move& m) {
    return given_move[m].mean () - unconditional.mean ();   // ineffective in loop
  }
};

// ----------------------------------------------------------------------

class AllAsFirst : public GtpCommand {
public:
  Board*      board;
  AafStats    aaf_stats;
  uint        playout_no;
  float       aaf_fraction;
  float       influence_scale;
  float       prior;
  bool        progress_dots;
  ExtPolicy policy;

public:
  AllAsFirst (Gtp& gtp, Board& board_) : board (&board_), policy(global_random) { 
    playout_no       = 50000;
    aaf_fraction     = 0.5;
    influence_scale  = 6.0;
    prior            = 1.0;
    progress_dots    = false;

    gtp.add_gogui_command (this, "dboard", "AAF.move_value", "black");
    gtp.add_gogui_command (this, "dboard", "AAF.move_value", "white");
		gtp.add_gogui_command (this, "dboard", "AAF.predict", "");
		gtp.add_gogui_command (this, "dboard", "AAF.critical", "");
		gtp.add_gogui_command (this, "dboard", "AAF.black_in_atari_all", "");
		gtp.add_gogui_command (this, "dboard", "AAF.black_in_atari_recent", "");
		gtp.add_gogui_command (this, "dboard", "AAF.white_in_atari_all", "");
		gtp.add_gogui_command (this, "dboard", "AAF.white_in_atari_recent", "");
		gtp.add_gogui_command (this, "dboard", "AAF.breaths", "");
		gtp.add_gogui_command (this, "dboard", "AAF.score", "");
		gtp.add_gogui_command (this, "dboard", "AAF.detect_ladders", "");
		gtp.add_gogui_command (this, "dboard", "AAF.detect_ladders2", "");

    gtp.add_gogui_param_float ("AAF.params", "prior",            &prior);
    gtp.add_gogui_param_float ("AAF.params", "aaf_fraction",     &aaf_fraction);
    gtp.add_gogui_param_float ("AAF.params", "influence_scale",  &influence_scale);
    gtp.add_gogui_param_uint  ("AAF.params", "playout_number",   &playout_no);
    gtp.add_gogui_param_bool  ("AAF.params", "20_progress_dots", &progress_dots);
  }
    
  void do_playout (const Board* base_board) {
    Board mc_board [1];
    mc_board->load (base_board);

    Playout<ExtPolicy> playout(&policy, mc_board);
    playout.run ();

    uint aaf_move_count = uint (float(playout.move_history_length)*aaf_fraction);
    float score = mc_board->score ();

    aaf_stats.update (playout.move_history, aaf_move_count, score);
  }

  virtual GtpResult exec_command (const string& command, istream& params) {
    if (command == "AAF.move_value") {
      Player player;
      if (!(params >> player)) return GtpResult::syntax_error ();
      aaf_stats.reset (prior);
      rep (ii, playout_no) {
        if (progress_dots && (ii * 20) % playout_no == 0) cerr << "." << flush;
        do_playout (board);
      }
      if (progress_dots) cerr << endl;

      FastMap<Vertex, float> means;
      vertex_for_each_all (v) {
        means [v] = aaf_stats.norm_mean_given_move (Move(player, v)) / influence_scale;;
        if (board->color_at [v] != Color::empty ()) 
          means [v] = 0.0;
      }
      return GtpResult::success (to_string_2d (means));
    }

		if (command == "AAF.predict") {
			Player player;
      
      FastMap<Vertex, float> means;
			
			vertex_for_each_all (v) { means[v] = 0.; }
			
			rep (ii, 100000) { 
				Board b[1];
				b->load(board);

				Playout<ExtPolicy> playout(&policy, b);
				playout.run();

				vertex_for_each_all (v) { means[v] += b->vertex_score(v); }
			}

			vertex_for_each_all (v) { means [v] /= 100000.; }

      return GtpResult::success (to_string_2d (means));
		}

		if (command == "AAF.critical") {
			Player player;
			PlayoutView view; 

			rep (ii, 100000) { 
				Board board2[1];
				board2->load(board);

				Playout<ExtPolicy> playout(&policy, board2);
				playout.run();
				view.update(board2, board2->winner());
			}

      return GtpResult::success (view);
		}

		if (command == "AAF.black_in_atari_all") {
			Vertex bs[50], ws[50];
			uint bc, wc;
			board->find_all_atari(bs, ws, bc, wc, 50);

			FastMap<Vertex, float> fm;
			Player act_player = board->act_player();

			vertex_for_each_all (v) {
				fm[v] = 0.;

				for (uint i = 0; i < bc; ++i) 
					if (bs[i] == v) { fm[v] = 1.; break; }
			}

			return GtpResult::success (to_string_2d (fm));
		}

		if (command == "AAF.white_in_atari_all") {
			Vertex bs[50], ws[50];
			uint bc, wc;
			board->find_all_atari(bs, ws, bc, wc, 50);

			FastMap<Vertex, float> fm;
			Player act_player = board->act_player();

			vertex_for_each_all (v) {
				fm[v] = 0.;

				for (uint i = 0; i < wc; ++i) 
					if (ws[i] == v) { fm[v] = 1.; break; }
			}

			return GtpResult::success (to_string_2d (fm));
		}

		if (command == "AAF.black_in_atari_recent") {
			Vertex bs[10], ws[10];
			uint bc, wc;
			board->find_recent_atari(bs, ws, bc, wc, 10);

			FastMap<Vertex, float> fm;
			Player act_player = board->act_player();

			vertex_for_each_all (v) {
				fm[v] = 0.;

				for (uint i = 0; i < bc; ++i) 
					if (bs[i] == v) { fm[v] = 1.; break; }
			}

			return GtpResult::success (to_string_2d (fm));
		}

		if (command == "AAF.white_in_atari_recent") {
			Vertex bs[10], ws[10];
			uint bc, wc;
			board->find_recent_atari(bs, ws, bc, wc, 10);

			FastMap<Vertex, float> fm;
			Player act_player = board->act_player();

			vertex_for_each_all (v) {
				fm[v] = 0.;

				for (uint i = 0; i < wc; ++i) 
					if (ws[i] == v) { fm[v] = 1.; break; }
			}

			return GtpResult::success (to_string_2d (fm));
		}

		if (command == "AAF.breaths") {
			FastMap<Vertex, float> fm;
			vertex_for_each_all (v) {
				fm[v] = board->chain_at(v).lib_cnt;
			}

			return GtpResult::success(to_string_2d (fm));
		}

		if (command == "AAF.score") {
			cerr 
				<< "tt_score: " << board->tt_score() 
				<< ", tt_winner: " << board->tt_winner_score() << endl;

			return GtpResult::success("=");
		}

		if (command == "AAF.detect_ladders") {
			Vertex bs[50], ws[50];
			uint bc, wc;
			board->find_all_atari(bs, ws, bc, wc, 50);

			FastMap<Vertex, float> fm;
			Player act_player = board->act_player();

			vertex_for_each_all (v) { fm[v] = 0.; }

			for (uint i = 0; i < wc; ++i)
				if (board->is_ladder(ws[i], Player::white()) == Player::black())
					fm[ws[i]] = 1.;

			for (uint i = 0; i < bc; ++i) 
				if (board->is_ladder(bs[i], Player::black()) == Player::white())
					fm[bs[i]] = -1.;

			return GtpResult::success (to_string_2d (fm));
		}

		if (command == "AAF.detect_ladders2") {
			Vertex bs[50], ws[50];
			uint bc, wc;
			board->find_all_atari(bs, ws, bc, wc, 50);

			FastMap<Vertex, float> fm;
			Player act_player = board->act_player();

			vertex_for_each_all (v) { fm[v] = 0.; }

			for (uint i = 0; i < wc; ++i)
				if (board->is_ladder(ws[i], Player::white()) == Player::black())
					fm[ws[i]] = 1.;

			for (uint i = 0; i < bc; ++i) 
				if (board->is_ladder(bs[i], Player::black()) == Player::white())
					fm[bs[i]] = -1.;

			return GtpResult::success (to_string_2d (fm));
		}

    assert (false);
  }
};
