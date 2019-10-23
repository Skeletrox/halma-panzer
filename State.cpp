#include "State.h"
#include "util.h"
#include <array>
#include <math.h>

using namespace std;

State::State(StateVector inpState, PositionsVector inPositions, State *inParent, bool root) {
	state = inpState;
	positions = inPositions;
	jump = !root && isJump(positions);
	parent = inParent;
	desiredChildLoc = -1;
	score = -FLT_MAX + 1;
	alphaBetaPrediction = score;
}

float State::getScore() {
	// Gets the score for this state
	return score;
}

void State::computeScore(char player, PositionsVector playersBases) {

	// Sets the score for this state
	/*
		Given a set of bases, the state returns the appropriate score for this player
			Reward for: Pieces in opponent's base, pieces towards opponent's base
			Punish for: Pieces in own base, pieces going everywhere except towards opponent's base

			Update for addendum: If a piece exists at home and this state does not jump from home,
			Set the score to negative so that this is not chosen.

		If this is a victory condition, then set the score to FLT_MAX
		If this is a loss condition, then set the score to -FLT_MAX + 1
	*/

	// Assume squatters don't exist. We shall set it to true if there are squatters in your base
	// Also identify who is the source of this evaluation to ensure we calculate the appropriate squatters
	bool squatters = false, squatterMovingOut = false;

	// The piece in the last location of positions corresponds to the team making the call.
	char evaluatedBy = state[positions[positions.size() - 1][1]][positions[positions.size() - 1][0]];

	// If the evaluation wasn't called by the player, then the squatters report is false.
	bool squattersIntelLegit = evaluatedBy == player;

	// Get number of pieces of the player in his own base and the number of pieces in the opponent's base
	float piecesInBase = 0.0, piecesInOpponent = 0.0;
	for (std::array<int, 2> loc : playersBases) {
		if (loc == positions[0]) {
			squatterMovingOut = true;
		}
		if (state[loc[1]][loc[0]] == player) {
			squatters = true;
			piecesInBase++;
		} else if (state[15 - loc[1]][15 - loc[0]] == player) {
			piecesInOpponent++;
		}
	}
	// Do the same for the opponent
	char opponent = player == 'B' ? 'W' : 'B';
	PositionsVector opponentBases = getMirror(playersBases);
	// cout << "Opponent bases are: " << endl; printPositions(opponentBases);
	// cout << "Your bases are: " << endl; printPositions(playersBases);
	float opponentPiecesInBase = 0.0, opponentPiecesInOpponent = 0.0;
	for (std::array<int, 2> loc : opponentBases) {
		if (state[loc[1]][loc[0]] == opponent) {
			opponentPiecesInOpponent++;
		}
		else if (state[15 - loc[1]][15 - loc[0]] == opponent) {
			opponentPiecesInBase++;
		}
	}
	// if at least one piece is in opponent bases and the opponent base is filled, you win. And vice-versa.
	if (piecesInOpponent > 0 && (piecesInOpponent + opponentPiecesInOpponent) == 19) {
		// cout << "Win condition for move " << positions[0][0] << "," << positions[0][1] << " " << positions[positions.size() - 1][0] << "," << positions[positions.size() - 1][1] << endl;
		score = FLT_MAX;
		return;
	}
	else if (opponentPiecesInBase > 0 && (piecesInBase + opponentPiecesInBase) == 19) {
		// cout << "Loss condition for move " << positions[0][0] << "," << positions[0][1] << " " << positions[positions.size() - 1][0] << "," << positions[positions.size() - 1][1] << " for team " << player << endl;
		// cout << "OPIB: " << opponentPiecesInBase << " PIB: " << piecesInBase << endl;
		// cout << "Loss State: " << endl;
		// printState(state);
		score = -FLT_MAX + 1;
		return;
	}

	/*	We want our pieces to move like a phalanx. Reward behavior that exhibits phalanx-like movement. 
		Get the closeness of the points from their "target" destinations, i.e. their appropriate goals
		Get the relativity of superimposition of pieces with the region y = x + 4 and y = x - 4.
		If all pieces are in this region then the score is 1, else reduce using distance of the point from y = x
	*/
	// Only perform the above step for 19 pieces
	int pieceCounter = 0;
	float directionalScore = 0.0;
	float fromBaseScore = 0.0, toOpponentScore = 0.0;
	bool breakable = false;
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			if (state[i][j] == player) {
				// get the utility of this point [the distance from y = x]
				float utilityScore = utility(j, i);
				fromBaseScore += sqrt(pow(j - playersBases[0][0], 2) + pow(i - playersBases[0][1], 2));
				toOpponentScore += sqrt(pow(j - opponentBases[0][0], 2) + pow(i - opponentBases[0][1], 2));
				// Consider an anomaly if the point is outside y = x + 4 or y = x  - 4
				// All points on y = x + 4 or y = x - 4 are at a distance of (4-0)/(sqrt(2)) from y = x
				if (utilityScore > 2.828427) {
					directionalScore += 1.0 / utilityScore;
				} else {
					directionalScore += 1.0;
				}
				// We have examined all 19 pieces
				if (++pieceCounter == 19) {
					breakable = true;
					break;
				}
			}
		}
		// No need to loop further
		if (breakable) {
			break;
		}
	}

	// The opponent score is less when you are closer to the opposite base. Subtract from a constant to ensure maximizability.
	toOpponentScore = 404 - toOpponentScore; // 15*(19*sqrt(2)) which is the maximum distance of a point from the oppponent base anchor
	/*
		Bring the self base score, diversion score and opponent base score together
			The opponent base score and diversion score affect the score POSITIVELY
			The self base score affects the score NEGATIVELY
			The furtherness from your own base and closeness to the opponent's base also affects the score POSITIVELY
	*/
	float totalScore = piecesInOpponent + directionalScore - piecesInBase + fromBaseScore;

	// If squatters exist and the guy moving is not a squatter then this move will cause a loss.
	if (squattersIntelLegit && squatters && !squatterMovingOut) {
		score = totalScore / 2; //Only use this as a last-ditch effort when you cannot move your pieces anywhere
		return;
	}
	score = totalScore;
}

void State::setFutureStates(PositionsVector positions, char team, PositionsVector baseAnchors, map<array<int, 4>, State*>* solutions) {
	map<array<int, 2>, bool>* visited = new map<array<int, 2>, bool>;
	std::vector<State*> allChildren{}, stepChildren, jumpChildren;
	std::pair<std::vector<State*>, int> stepResult, jumpResult;
	// If there are pieces still in the base, only choose those pieces. If not, choose all.
	PositionsSet base = team == 'B' ? getMirrorSet(getMirror(baseAnchors)) : getMirrorSet(baseAnchors);
	PositionsVector basePieces{}, posArgument{};
	// If a point exists in the base, use only that point. Nothing else.
	for (int i = 0; i < positions.size(); i++) {
		if (base.count(positions[i]) > 0) {
			basePieces.push_back(positions[i]);
		}
	}
	// If there are any base children, then we use them. Else we use all the positions
	if (basePieces.size() >= 1) {
		posArgument = basePieces;
	} else {
		posArgument = positions;
	}

	stepResult = getSteps(posArgument, team, baseAnchors);
	stepChildren = stepResult.first;
	int stepChildIndex = stepResult.second;
	// if a step returns a victory, then don't bother jumping.
	if (stepChildIndex > -1 && stepChildren[stepChildIndex]->getScore() == FLT_MAX) {
		setChildren(stepChildren);
		desiredChildLoc = stepChildIndex;
		return;
	}
	jumpResult = getJumps(posArgument, team, baseAnchors, visited, solutions);
	jumpChildren = jumpResult.first;
	/*
		Special check for addendum: Allow for out-of-base pieces to move if all in-base pieces cannot move
		"further away" from the base. We assume 2 conditions:
			1. If the lack of children is due to the disregard of the out-of-base pieces, then regard them.
			2. No moves actually exist [There is no situation where the in-base piece moving "closer" is legal].

		The second attempt fixes condition 1, doesn't change condition 2
	*/
	// std::cout << "Step children size: " << stepResult.first.size() << " Jump children size: " << jumpResult.first.size() << std::endl;
	if (stepChildren.size() == 0 && jumpChildren.size() == 0) {
		// cout << "No children within constraints" << endl;
		stepResult = getSteps(positions, team, baseAnchors);
		jumpResult = getJumps(positions, team, baseAnchors, visited, solutions);
		// std::cout << "Step children size: " << stepResult.first.size() <<  " Jump children size: " << jumpResult.first.size() << std::endl;
	}

	// Refresh the results
	stepChildren = stepResult.first;
	jumpChildren = jumpResult.first;

	// Get the index of the best child from each result
	int stepChildrenIndex = stepResult.second, jumpChildrenIndex = jumpResult.second;
	// clean JumpChildren
	/*
				Move constraints:
					Track illegal children and ensure their children lead to legality.
	*/
	PositionsVector jumpPositions;
	int i = 0;
	while (i < jumpChildren.size()) {
		jumpPositions = jumpChildren[i]->getPositions();
		int startX = jumpPositions[0][0], startY = jumpPositions[0][1];
		int endX = jumpPositions[jumpPositions.size() - 1][0], endY = jumpPositions[jumpPositions.size() - 1][1];
		if (isIllegal(startX, startY, endX, endY, baseAnchors, team)) {
			// cout << "Jump illegal from " << startX << "," << startY << " to " << endX << "," << endY << " for team " << team << endl;
			// Remove this child from the node
			jumpChildren.erase(jumpChildren.begin() + i);
			if (i < jumpChildrenIndex) {
				// The desired jump child is now one index before where it originally was
				jumpChildrenIndex--;
			}
			else if (i == jumpChildrenIndex) {
				// Oops. We chose an illegal child as the preferred one and erased it.
				// Iterate through the children and reset preferred jump child.
				int maxJumpIndex = 0;
				float maxJumpScore = -FLT_MAX + 1;
				for (int j = 0; j < jumpChildren.size(); j++) {
					if (jumpChildren[j]->getScore() > maxJumpScore) {
						maxJumpIndex = j;
						maxJumpScore = jumpChildren[j]->getScore();
					}
				}
				jumpChildrenIndex = maxJumpIndex;
			} // else we do not care. The max jump child does not change its position in the vector.

			i--; // Show some respect a state has JUST DIED!
		}
		i++;
	}
	// All jumps are illegal!
	if (jumpChildren.size() == 0) {
		jumpChildrenIndex = -1;
	}

	if (stepChildrenIndex == -1 && jumpChildrenIndex == -1) {
		// No Children for this state
		return; 
	}
	else if (stepChildrenIndex == -1) {
		// only Jump Children
		this->desiredChildLoc = jumpChildrenIndex;
	} else if (jumpChildrenIndex == -1) {
		// only Step Children
		this->desiredChildLoc = stepChildrenIndex;
	} else {
		// both exist
		if (stepChildren[stepChildrenIndex]->getScore() > jumpChildren[jumpChildrenIndex]->getScore()) {
			desiredChildLoc = stepChildrenIndex; // The better child
		}
		else {
			desiredChildLoc = stepChildren.size() + jumpChildrenIndex; // As jump results are appended AFTER step results 
		}
	}
	allChildren.insert(allChildren.end(), stepChildren.begin(), stepChildren.end());
	allChildren.insert(allChildren.end(), jumpChildren.begin(), jumpChildren.end());
	setChildren(allChildren);
}

std::pair<std::vector<State*>, int> State::getSteps(PositionsVector positions, char team, PositionsVector baseAnchors) {
	int numPositions = positions.size();
	/*
		Initialize best step index and the best step score to -1 and -infinity
	*/
	int bestStepIndex = -1;
	float bestStepScore = -FLT_MAX + 1;
	std::vector<State*> stepChildren{};
	for (int i = 0; i < numPositions; i++) {
		// The point is expressed as x, y
		// See if all adjacent points are okay
		array<int, 2> current = positions[i];
		int x = current[0];
		int y = current[1];
		// Get adjacent positions
		int xAdjacents[3] = { x - 1, x, x + 1 };
		int yAdjacents[3] = { y - 1, y, y + 1 };

		// Initialize the adjacent cells vector
		vector<array<int, 2>> adjacentCells{};

		// counter to handle only 8 points being chosen instead of 9
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				int xNow = xAdjacents[j], yNow = yAdjacents[k];
				// Ignore the current cell, out of bounds, and visited cells
				if ((xNow > 15) || (xNow < 0) || (yNow > 15) || (yNow < 0) || (yNow == y && xNow == x)) {
					continue;
				}
				// Create a new array for the new points and append that to the adjacent cells
				array<int, 2> currentAdjacent = { xNow, yNow };
				adjacentCells.push_back(currentAdjacent);
			}
		}
		// Iterate through the list of all adjacent cells and ensure we return only the acceptable states.
		for (int j = 0; j < adjacentCells.size(); j++) {
			StateVector newState(state);
			int currX = adjacentCells[j][0], currY = adjacentCells[j][1];

			/*
				Move constraints:
					Do not jump back into your base, do not jump out of your opponent's base
			*/
			if (isIllegal(x, y, currX, currY, baseAnchors, team)) {
				continue;
			}

			// Generate the state with the current coin swapped with the void in the other cell
			// If there is something in that cell, check if it can be jumped over
			if (newState[currY][currX] == '.') {
				// This is an empty cell. Swap the position with it and append to the FutureStates vector.
				//XOR swapping
				newState[y][x] = newState[y][x] ^ newState[currY][currX];
				newState[currY][currX] = newState[y][x] ^ newState[currY][currX];
				newState[y][x] = newState[y][x] ^ newState[currY][currX];

				// Create a child state object
				PositionsVector positionPair = { {x, y}, {currX, currY} };
				State* childState = new State(newState, positionPair, this, false);
				childState->computeScore(team, team == 'B' ? baseAnchors : getMirror(baseAnchors));

				// Best score yet
				if (childState->getScore() > bestStepScore) {
					bestStepIndex = stepChildren.size();
					bestStepScore = childState->getScore();
				}
				stepChildren.push_back(childState);
				
				if (childState->getScore() == FLT_MAX) {
					// Ding ding ding we have a winner. Don't bother expanding the rest.
					return std::pair<std::vector<State*>, int>{stepChildren, bestStepIndex};
				}
				
			}
		}
	}
	return std::pair<std::vector<State*>, int>{stepChildren, bestStepIndex};
}

std::pair<std::vector<State*>, int> State::getJumps(PositionsVector positions, char team, PositionsVector baseAnchors, map<array<int, 2>, bool> *visited, std::map<std::array<int, 4>, State*>* precomputed) {
	int numPositions = positions.size();
	/*
		Define the best jump index [which index gives the best jump] and set that as the desired child
		Define the best score to avoid having to access each state over and over again
	*/
	int bestJumpIndex = -1;
	float bestJumpScore = -FLT_MAX + 1;

	std::vector<State*> jumpChildren;
	for (int i = 0; i < numPositions; i++) {
		// The point is expressed as x, y
		// See if all adjacent points are okay
		array<int, 2> current = positions[i];
		int x = current[0];
		int y = current[1];
		map<array<int, 2>, bool>* needed;
		// If you are expanding over a child, then use the parent's visited to avoid coming back. Else don't.
		if (numPositions == 19) {
			needed = new map<array<int, 2>, bool>;
			needed->insert(pair<array<int, 2>, bool>({ x, y }, true));
		}
		else {
			needed = visited;
		}
		// Get adjacent positions
		int xAdjacents[3] = { x - 1, x, x + 1 };
		int yAdjacents[3] = { y - 1, y, y + 1 };

		// Initialize the adjacent cells vector
		vector<array<int, 2>> adjacentCells{};

		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				int xNow = xAdjacents[j], yNow = yAdjacents[k];
				// Ignore the current cell and out of bounds
				if ((xNow > 15) || (xNow < 0) || (yNow > 15) || (yNow < 0) || (yNow == y && xNow == x)) {
					continue;
				}
				// Create a new array for the new points and append that to the adjacent cells
				array<int, 2> currentAdjacent = { xNow, yNow };
				adjacentCells.push_back(currentAdjacent);
			}
		}
		for (int j = 0; j < adjacentCells.size(); j++) {
			StateVector newState(state);
			int currX = adjacentCells[j][0], currY = adjacentCells[j][1];
			if (newState[currY][currX] == '.') { // Do not jump over an empty cell
				continue;
			}
			int xFactor = int(x <= currX) - int(x == currX) - int(x > currX);
			int yFactor = int(y <= currY) - int(y == currY) - int(y > currY);
			// Check the next value, while ensuring that it is accessible
			int newTargetx = currX + xFactor, newTargety = currY + yFactor;
			// Ensure that the new target is reachable, hasn't been already visited
			if ((newTargetx > 15) || (newTargetx < 0) || (newTargety > 15) || (newTargety < 0) || needed->count({ newTargetx, newTargety }) > 0) {
				continue;
			}
			if (newState[newTargety][newTargetx] == '.') {
				// Check if we have already expanded this before from this source.
				if (precomputed->count({ x, y, newTargetx, newTargety }) > 0) {
					// Get the score of this state, and update bestPossible if required.
					State* needed = precomputed->at({ x, y, newTargetx, newTargety });
					if (needed->getScore() > bestJumpScore) {
						bestJumpScore = needed->getScore();
						bestJumpIndex = jumpChildren.size();
					}
					jumpChildren.push_back(needed);
					visited->insert(std::pair<std::array<int, 2>, bool>({ newTargetx, newTargety}, true));
					continue;
				}
				// Perform the jump and update.

				newState[y][x] = newState[y][x] ^ newState[newTargety][newTargetx];
				newState[newTargety][newTargetx] = newState[y][x] ^ newState[newTargety][newTargetx];
				newState[y][x] = newState[y][x] ^ newState[newTargety][newTargetx];
				PositionsVector positionPair = { {x, y}, {newTargetx, newTargety} };
				State* childState = new State(newState, positionPair, this, false);
				/*
					There are two outcomes, either expand childState and potentially see a drop in utility
						Or just stop here and if this is the best you can get, save this.
				*/
				childState->computeScore(team, team == 'B' ? baseAnchors : getMirror(baseAnchors));

				/*
					Expand this child [perform another jump] and check its utility recursively.
				*/
				State* childStateWithChildren = new State(newState, positionPair, this, false);

				// Use this to get child states so that you can choose the appropriate child later
				// Only consider jumps from this new target
				needed->insert(std::pair<std::array<int, 2>, bool>({ newTargetx, newTargety }, true));
				std::pair<std::vector<State*>, int> result = childStateWithChildren->getJumps({ {newTargetx, newTargety} }, team, baseAnchors, needed, precomputed);
				if (result.second != -1) {
					// We can squash a sequence of moves as a child exists for this child
					childStateWithChildren->setChildrenAndDesired(result.first, result.second);
					/*
								"Squash" this sequence of jumps into a single state.
								Also update the state member to reflect the state of the preferred child.
					*/
					State* childOfChild = childStateWithChildren->getDesiredChild();
					PositionsVector childOfChildPos = childOfChild->getPositions();
					PositionsVector childOfChildPosNew = PositionsVector(childOfChildPos.begin() + 1, childOfChildPos.end());
					PositionsVector squashedPositions = childStateWithChildren->getPositions();
					squashedPositions.insert(squashedPositions.end(), childOfChildPosNew.begin(), childOfChildPosNew.end());
					childStateWithChildren->setState(childOfChild->getState());
					childStateWithChildren->setPositions(squashedPositions);
					childStateWithChildren->computeScore(team, team == 'B' ? baseAnchors : getMirror(baseAnchors));
				} else {
					childStateWithChildren->setScore(childState->getScore());
				}
				float singleScore = childState->getScore(), expandedScore = childStateWithChildren->getScore();
				if (singleScore >= expandedScore) {
					precomputed->insert(std::pair<std::array<int, 4>, State*>({x, y, newTargetx, newTargety}, childState));
					childState->setScore(childState->getScore());
					if (singleScore > bestJumpScore) {
						bestJumpScore = singleScore;
						bestJumpIndex = jumpChildren.size(); // The best jump was added last!
					}
					jumpChildren.push_back(childState);
				} else {
					precomputed->insert(std::pair<std::array<int, 4>, State*>({x, y, newTargetx, newTargety }, childStateWithChildren));
					childStateWithChildren->setScore(childStateWithChildren->getScore());
					if (expandedScore > bestJumpScore) {
						bestJumpScore = expandedScore;
						bestJumpIndex = jumpChildren.size();
						setDesiredChildLoc(bestJumpIndex);
						setScore(bestJumpScore);
					}
					jumpChildren.push_back(childStateWithChildren);
				}
			}
		}

	}
	return std::pair<std::vector<State*>, int>{jumpChildren, bestJumpIndex};;
}

StateVector State::getState() {
	return state;
}

std::vector<State *> State::getChildren() {
	return children;
}

State* State::getDesiredChild() {
	return children[desiredChildLoc];
}

void State::setDesiredChildLoc(int i) {
	desiredChildLoc = i;
}

float State::getAlphaBetaPrediction() {
	return alphaBetaPrediction;
}

void State::setAlphaBetaPrediction(float value) {
	alphaBetaPrediction = value;
}

void State::setChildren(std::vector<State *> argChildren) {
	children = argChildren;
}

int State::getDesiredChildLoc() {
	return desiredChildLoc;
}

PositionsVector State::getPositions() {
	return positions;
}

void State::setScore(float newScore) {
	score = newScore;
}

void State::setState(StateVector s) {
	state = s;
}

void State::setPositions(PositionsVector p) {
	positions = p;
}

void State::setChildrenAndDesired(std::vector<State*> argChildren, int desiredLoc) {
	children = argChildren;
	desiredChildLoc = desiredLoc;
}

bool State::isStateAJump() {
	return jump;
}