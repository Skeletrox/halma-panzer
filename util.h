#ifndef UTIL_H
#define UTIL_H
#include "components.h"
#include "State.h"
#include <limits>
#include <cfloat>
#include <string>

PositionsVector getPositions(StateVector boardState, char team);

bool isJump(PositionsVector positions);

float utility(int x, int y);
float max(float a, float b);
float min(float a, float b);

State* doAlphaBetaPruning(State *root);

float doMinValue(State* state, float alpha, float beta);
float doMaxValue(State* state, float alpha, float beta);

void printState(StateVector state);

long calibrate();

int getDepth(float timeRemaining, long calibratedValue, float currentScore);

bool isIllegal(int xStart, int yStart, int xEnd, int yEnd, PositionsVector baseAnchors, char team);
bool found(int x, int y, PositionsVector baseAnchors, bool reverse);
bool isOrderedAs(int first, int second, int third, bool reverse);

PositionsVector getMirror(PositionsVector original);
PositionsSet getMirrorSet(PositionsVector original);

void printPositions(PositionsVector positions);

std::string generateString(PositionsVector positions, bool isJump);
#endif // !UTIL_H