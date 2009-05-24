#ifndef _VIEW_H_
#define _VIEW_H_

class PlayoutView {
	public:
		PlayoutView(void) { reset(); }
		~PlayoutView(void) { }

		FastMap<Vertex, float> winning;
		FastMap<Player, FastMap<Vertex, float> > pfield;
		FastMap<Player, float> pcount;
		float count;

		void reset(void)
		{
			count = 0.;
			pcount[Player::black()] = 0.;
			pcount[Player::white()] = 0.;

			vertex_for_each_all(x) {
				winning[x] = 0.;
				pfield[Player::black()][x] = 0.;
				pfield[Player::white()][x] = 0.;
			}
		}

		void update(Board *board, const Player &winner)
		{
			++pcount[winner];
			++count;

			if (Player::black() == winner) {
				vertex_for_each_all(x) {
					int owner = board->vertex_score(x);

					if (owner > 0) {
						++pfield[Player::black()][x];
						++winning[x];
					} else if (owner < 0) {
						++pfield[Player::white()][x];
					}
				}
			} else {
				vertex_for_each_all(x) {
					int owner = board->vertex_score(x);

					if (owner > 0) {
						++pfield[Player::black()][x];
					} else if (owner < 0) {
						++pfield[Player::white()][x];
						++winning[x];
					}
				}
			}
		}

		float operator[](Vertex x) const
		{
			if (!count) return 0.;
			
			return 
				winning[x] / count 
				- 
				( 
				 pfield[Player::black()][x] / count * pcount[Player::black()] / count
				 +
				 pfield[Player::white()][x] / count * pcount[Player::white()] / count
				);
		}

		operator string (void) const
		{
			FastMap<Vertex, float> m;
			
			vertex_for_each_all (x)
				m[x] = this->operator[](x);

			return to_string_2d(m);
		}

};

#endif
