ptego_path := ./ptego/
libego_path := ./ego-2009-05-07+/
includes := -I$(ptego_path) -I$(libego_path)

cxxflags := -ggdb3 -Wall -Wextra -Wswitch-enum -Wunused -O2 -static \
	-march=native -fomit-frame-pointer -frename-registers -ffast-math

cxxflagsdebug := -Wall -Wextra -ggdb3 -fno-inline -DTESTING -O0

options := -DUSE_PLAYOUT_VIEW -DUSE_UCT_LOCALITY \
	-DUSE_BEGINING_IN_PLAYOUT -DUSE_ATARI_IN_PLAYOUT \
	-DUSE_LOCALITY_IN_PLAYOUT -DUSE_LADDER_DETECTION

all: Makefile.deps tags engine

debug: Makefile.deps tags engine-debug

tags:
	ctags $(ptego_path)/*.{h,cpp} $(libego_path)/*.{h,cpp}

Makefile.deps:
	g++ -MM $(includes) $(ptego_path)/*.{h,cpp} $(libego_path)/*.{h,cpp} > Makefile.deps

-include Makefile.deps

engine: $(libego_path)/ego.cpp $(ptego_path)/main.cpp
	g++ $(includes) $(cxxflags) $(options) -o $@ $^

engine-debug: $(libego_path)/ego.cpp $(ptego_path)/main.cpp
	g++ $(includes) $(cxxflagsdebug) $(options) -o $@ $^

clean:
	rm -f engine engine-debug Makefile.deps tags

