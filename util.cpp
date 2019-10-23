/*
	Utility file. Define utility functions such as position initialization here.
*/
#include "util.h"
#include <math.h>
#include <chrono>

float max(float a, float b) {
	return a > b ? a : b;
}

float min(float a, float b) {
	return a < b ? a : b;
}


// Returns the positions of all the pieces of a certain team.
PositionsVector getPositions(StateVector boardState, char team) {
	PositionsVector neededVector{};
	// Iterate through the 16x16 board
	int count = 0;
	bool breakable = false;
	for (int i = 0; i < boardState.size(); i++) {
		for (int j = 0; j < boardState[i].size(); j++) {
			if (boardState[i][j] == team) {
				neededVector.push_back({ j, i });
				count++;
				if (count == 19) {
					breakable = true;
					break;
				}
			}
		}
		if (breakable) {
			break;
		}
	}
	return neededVector;
}

// Returns true if the move involves a jump, useful for checking if recursion needs to be handled.
bool isJump(PositionsVector positions) {
	return (abs(positions[0][0] - positions[1][0]) > 1 || abs(positions[0][1] - positions[1][1]) > 1);
}

// Utility function, defined as the distance from (x, y) to y = x
float utility(int x, int y) {
	float numerator = abs(float(x) - float(y));
	return ( numerator / float(sqrt(2)));
}

void doSomething(int x) {
	if (x == 0) {
		return;
	}
	doSomething(x / 2);
}

long calibrate() {
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 1000; i++) {
		doSomething(i);
	}
	auto end = std::chrono::high_resolution_clock::now();
	long diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	// Avoid zeros for powerful machines
	return diff + 1;
}

int getDepth(float timeRemaining, long calibratedValue, float currentScore) {
	/*
		The hard time limits are as follows:
			The following time limits are generated on a calibration factor of 1,
			i.e. incrementing a variable 1000 times takes 1 miecrosecond

			The time remaining can be expressed in microseconds, and multiply time taken calibratedValue, as a slower computer
			can cause our values to be awry.
	*/
	float timeRemainingMicrosec = timeRemaining * 1000000;
	std::cout << "Remaining time: " << timeRemainingMicrosec << " and calibrated value " << calibratedValue <<  " and score " << currentScore << std::endl;
	std::vector<long> timesTerminal{ 7416, 24016, 129892, 1457973, 5266859, 51633889 }, timesMidway = { 1419, 77200, 359061, 6874063, 46678036 }, times;
	times = currentScore < 127 ? timesTerminal : timesMidway;
	for (int i = 0; i < times.size()-1; i++) {
		if (timeRemainingMicrosec < (times[i + 1] * calibratedValue)) {
			// We don't have enough time to deepen to the next level; stop here
			std::cout << "Expected duration: " << times[i] << " " << i << std::endl;
			return i+1; // Zero-indexed array
		}
	}
	std::cout << "Expected duration: " << times[times.size() - 1] << " " << times.size() << std::endl;
	return times.size();
}

bool isIllegal(int xStart, int yStart, int xEnd, int yEnd, PositionsVector baseAnchors, char team) {
	if (team == 'B') {
		// 1. Your own base is the baseAnchors. Make sure that your piece, if not in your base, does not jump back
		// 2. Your opponent's base is at 15 - baseAnchors. Do not jump out of it.
		if (!found(xStart, yStart, baseAnchors, false) && found(xEnd, yEnd, baseAnchors, false)) { // Case 1
			// std::cout << "Black base cowardice" << std::endl;
			return true;
		} else if (found(xStart, yStart, baseAnchors, true) && !found(xEnd, yEnd, baseAnchors, true)) { // Case 2
			// std::cout << "Black base escapism" << std::endl;
			return true;
		} else if (found(xStart, yStart, baseAnchors, false) && found(xEnd, yEnd, baseAnchors, false) && (isOrderedAs(xStart, xEnd, baseAnchors[0][0], false) || isOrderedAs(yStart, yEnd, baseAnchors[0][1], false))) {
			// std::cout << "Black base cowardice within" << std::endl;
			return true;
		}
	}
	else {
		// The above lines switched in context.
		if (!found(xStart, yStart, baseAnchors, true) && found(xEnd, yEnd, baseAnchors, true)) { // Case 1
			// std::cout << "White base cowardice" << std::endl;
			return true;
		} else if (found(xStart, yStart, baseAnchors, false) && !found(xEnd, yEnd, baseAnchors, false)) { // Case 2
			// std::cout << "White base escapism" << std::endl;
			return true;
		} else if (found(xStart, yStart, baseAnchors, true) && found(xEnd, yEnd, baseAnchors, true) && (isOrderedAs(xStart, xEnd, baseAnchors[0][0], true) || isOrderedAs(yStart, yEnd, baseAnchors[0][1], true))) {
			// std::cout << "White base cowardice within" << std::endl;
			return true;
		}
	}
	// Legal move only
	return false;
}

bool found(int x, int y, PositionsVector baseAnchors, bool reverse) {
	for (std::array<int, 2> b : baseAnchors) {
		if ((x == (reverse ? 15 - b[0] : b[0])) && (y == (reverse ? 15 - b[1] : b[1]))) {
			return true;
		}
	}
	return false;
}

/*
	Returns true if second is either at third or on the way between first and third.
*/
bool isOrderedAs(int first, int second, int third, bool reverse) {
	if (reverse) {
		third = 15 - third;
		// std::cout << "First: " << first << " second: " << second << " third: " << third << std::endl;
		return first < second && second <= third;
	}
	// std::cout << "First: " << first << " second: " << second << " third: " << third << std::endl;
	return first > second && second >= third;
}

// Returns the diagonal mirror of the positions along y + x = 15
PositionsVector getMirror(PositionsVector original) {
	PositionsVector mirror;
	for (std::array<int, 2> o : original) {
		std::array<int, 2> v{ 15 - o[0], 15 - o[1] };
		mirror.push_back(v);
	}
	return mirror;
}

PositionsSet getMirrorSet(PositionsVector original) {
	PositionsSet mirror;
	for (std::array<int, 2> o : original) {
		std::array<int, 2> v{ 15 - o[0], 15 - o[1] };
		mirror.insert(v);
	}
	return mirror;
}

void printPositions(PositionsVector positions) {
	for (std::array<int, 2> p : positions) {
		std::cout << p[0] << "," << p[1] << " ";
	}
	std::cout << std::endl;
}

void printState(StateVector state) {
	for (int i = 0; i < state.size(); i++) {
		for (int j = 0; j < state[i].size(); j++) {
			std::cout << state[i][j];
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

std::string generateString(PositionsVector positions, bool isJump) {
	std::string returnable = "";
	for (int i = 0; i < positions.size() - 1; i++) {
		if (isJump) {
			returnable += "J ";
		}
		else {
			returnable += "E ";
		}
		std::array<int, 2> first = positions[i], second = positions[i + 1];
		char* currentLine = (char*)malloc(40 * sizeof(char));
		snprintf(currentLine, 40, "%d,%d %d,%d\n", first[0], first[1], second[0], second[1]);
		returnable.append(currentLine);
	}
	return returnable;
}