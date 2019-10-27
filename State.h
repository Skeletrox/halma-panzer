#ifndef STATE_H
#define STATE_H
#include <iostream>
#include "components.h"
#include <map>
/*
	Stores a board state with the following values:
		1. The state of the board
		2. The appropriate move made to get the current state
		3. Is this a jump?
		4. Score
*/
class State
{
private:
	StateVector state;
	PositionsVector positions;
	bool jump;
	State* parent;
	int desiredChildLoc;
	std::vector<State *> children;
	float score, alphaBetaPrediction;

public:
	State(StateVector inpState, PositionsVector inPositions, State *inParent, bool root);
	// Getter and setter for children
	std::vector<State *> getChildren();
	State* getDesiredChild();
	void setDesiredChildLoc(int i);
	int getDesiredChildLoc();
	void setChildren(std::vector<State *> argChildren);
	void setChildrenAndDesired(std::vector<State*> argChildren, int desiredLoc);

	//Future state predictors
	void setFutureStates(PositionsVector positions, char team, PositionsVector baseAnchors, std::map<std::array<int, 4>, State*>* solutions);
	std::pair<std::vector<State*>, int> getSteps(PositionsVector positions, char team, PositionsVector baseAnchors);
	std::pair<std::vector<State*>, int> getJumps(PositionsVector positions, char team, PositionsVector baseAnchors, std::map<std::array<int, 2>, bool> *visited, std::map<std::array<int, 4>, State *> *precomputed);

	// Getter and setter for score and alphaBetaPrediction
	void computeScore(char player, PositionsVector playersBases);
	void setScore(float newScore);
	float getScore();
	void setAlphaBetaPrediction(float value);
	float getAlphaBetaPrediction();

	// Getter and setter for state and positions
	StateVector getState();
	void setState(StateVector s);
	PositionsVector getPositions();
	void setPositions(PositionsVector p);

	// jump getter
	bool isStateAJump();

};
#endif // !STATE_H
