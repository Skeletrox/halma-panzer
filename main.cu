#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include "components.h"
#include "Board.h"
#include "Player.h"
#include "State.h"
#include "util.h"
#include <chrono>
#include <cfloat>
#include <ctime>
#include <cstdlib>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

using namespace std;

long runProgram(float performanceMeasure) {
	ifstream inputFile;
	fstream playDataFile;
	inputFile.open("./input.txt");
	StateVector initState{};
	string executionType = "SINGLE", s = "";
	char team = 'B';
	bool randNeeded;
	float timeLeft = 100.0, previousScore = -1.0;
	int counter = 0;
	while (inputFile >> s) {
		switch (counter) {
		case 0:
			// First line: Type of execution. By default is SINGLE
			if (s.length() > 0) {
				executionType = s;
			}
			break;
		case 1:
			// Second line: Team
			team = s == "WHITE" ? 'W' : 'B';
			break;
		case 2:
			// Third line: Number of seconds remaining
			try {
				timeLeft = stof(s);
			}
			catch (...) {
				// If there is an error in the input then timeLeft is default
			}
			break;
		default:
			// The board
			vector<char> row(s.begin(), s.end());
			initState.push_back(row);
		}
		counter++;
	}
	Board board = Board(initState);
	/*
		If the execution type is a game, then check for a playdata.txt
		The playdata.txt contains the time you can take for a move, and maybe a sequence of steps.
	*/
	State* currState = new State(initState, { {} }, NULL, true);
	currState->computeScore(team, board.getBase(team));

	if (executionType == "GAME") {
		struct stat playFile;
		if (stat("./playdata.txt", &playFile) != -1) {
			// File exists, read it.
			playDataFile.open("./playdata.txt");
			playDataFile >> s;
			float timeLeftFromPlayData = stof(s);
			playDataFile >> s;
			previousScore = stof(s);
			if (timeLeftFromPlayData > timeLeft) {
				// We don't have the time we think we have
				// Try to squeeze out 10 moves from the remaining time
				timeLeft = timeLeft / 10;
			}
			else {
				timeLeft = timeLeftFromPlayData;
			}
		}
		else {
			timeLeft = timeLeft / 110;
		}
		// Average game length of 110 moves
	}
	int depth = getDepth(timeLeft, performanceMeasure, currState->getScore());
	PositionsVector playerPositions = getPositions(initState, team);
	Player player = Player(team, playerPositions);
	/*
		Generate the minmax tree with the following attributes:
			The current State
			How deep can the player jump
			The number of turns
			The locations of the player's points
			Alpha and Beta [For Alpha-Beta Pruning]
	*/
	auto start = chrono::high_resolution_clock::now();
	currState = board.generateMinMaxTree(currState, 2, player.getLocations(), -FLT_MAX + 1, FLT_MAX, true);

	// Get the argmax of all alphabetas of currState's children
	vector<State*> children = currState->getChildren();
	float maxChildScore = -FLT_MAX;
	int maxChildLoc = -1;

	for (int i = 0; i < children.size(); i++) {
		if (children[i]->getAlphaBetaPrediction() > maxChildScore) {
			maxChildScore = children[i]->getAlphaBetaPrediction();
			maxChildLoc = i;
		}
	}
	string result;
	if (maxChildLoc == -1) {
		// No valid moves, gracefully exit
		result = "NO VALID MOVES";
	}
	else {
		State* desiredChild = children[maxChildLoc];
		if (abs(previousScore - desiredChild->getAlphaBetaPrediction()) < 1.5) {
			srand(time(NULL));
			int someNumber = rand();
			maxChildLoc = someNumber % children.size();
			desiredChild = children[maxChildLoc];
		}
		result = generateString(desiredChild->getPositions(), desiredChild->isStateAJump());
	}
	ofstream outFile;
	outFile.open("./output.txt");
	outFile << result;
	outFile.close();
	if (executionType == "GAME") {
		// Some persistent data we may be able to use
		// Store the actual playtime.
		playDataFile.open("./playdata.txt", fstream::out);
		char* timeLeftString = (char*)malloc(20 * sizeof(char));
		snprintf(timeLeftString, 20, "%.4f", timeLeft);
		playDataFile << timeLeftString << endl;
		playDataFile << currState->getScore();
		playDataFile.close();
	}
	auto end = chrono::high_resolution_clock::now();
	long actual = chrono::duration_cast<chrono::microseconds>(end - start).count();
	return actual;
}

int execProg() {
	float performanceMeasure = calibrate();
	runProgram(performanceMeasure);
	return 0;
}