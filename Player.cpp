#include "Player.h"
#include "State.h"
#include <array>

Player::Player(char argTeam, PositionsVector argLocations) {
	team = argTeam;
	score = 0;
	locations = argLocations;
}

char Player::getTeam() {
	return team;
}

float Player::getScore() {
	return score;
}

PositionsVector Player::makeMove(StateVector state) {
	PositionsVector p;
	// Given a state, choose the appropriate movement and get a score
	// Get all future states for the current state
	return p;
}

PositionsVector Player::getLocations() {
	return locations;
}


