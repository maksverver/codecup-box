#ifndef ANALYSIS_H_DEFINED
#define ANALYSIS_H_DEFINED

#include "state.h"

// Generates a list of all placements that are valid in the current grid,
// in lexicographical order (row, column, orientation).
std::vector<Placement> GeneratePlacements(const grid_t &grid);

// Returns a boolean grid where cells are 0 if they could still be changed by
// a future move, or 1 if they are fixed, because no valid move overlaps.
grid_t CalcFixed(const grid_t &grid);

// Evaluates the score for all colors.
void EvaluateAllColors(const grid_t &grid, const grid_t &fixed, std::array<int, COLORS> &scores);

// Evaluates the score for two colors, and returns the difference of my score
// minus his score.
int EvaluateTwoColors(const grid_t &grid, const grid_t &fixed, int my_color, int his_color);

// Evaluates the points awared for squares only. This corresponds with the final
// score of the game, but it's not very useful for an intermediate evaluation
// function, because it does not award points for partially-formed squares, and
// doesn't distinguish between fixed and non-fixed cells.
void EvaluateFinalScore(const grid_t &grid, std::array<int, COLORS> &scores);

#endif // ndef ANALYSIS_H_DEFINED
