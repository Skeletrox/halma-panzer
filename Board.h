#ifndef BOARD_H
#define BOARD_H

#include "components.h"
#include "State.h"
#include <cfloat>

class Board {
private:
	StateVector state;
	PositionsVector blackBase, whiteBase;
	std::map<std::array<int, 4>, bool>* visited;
	std::map<std::array<int, 4>, State*>* solutions;

public:
	Board(StateVector inpState);
	PositionsVector getBase(char team);
	State* generateMinMaxTree(State *s, int turnCount, PositionsVector argLocations, float alpha, float beta, bool max);
};

#endif // !BOARD_H