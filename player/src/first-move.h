#ifndef FIRST_MOVE_H_INCLUDED
#define FIRST_MOVE_H_INCLUDED

#include "state.h"

#include <functional>

struct BestFirstMove {
  int color;
  tile_t tile;
  Placement best_placement;

  auto operator<=>(const BestFirstMove&) const = default;
};

// Calculates the best first moves for all possible initial colvers and tiles,
// using the given evaluation function.
std::vector<BestFirstMove> CalculateBestFirstMoves(
  std::function<std::vector<Placement>(
    int color, const grid_t &grid, const tile_t &tile,
    const std::vector<Placement> &all_placements)> find_best_placements);

// Prints the result from CalculateBestFirstMoves() as C++ source code.
void PrintBestFirstMoves(std::ostream &os, const std::vector<BestFirstMove> &moves);

// Returns the list of best moves for the given secret color, first move, and initial tile.
std::vector<Placement> FindBestFirstMoves(
    int secret_color, const Move first_move, const tile_t &tile);

#endif // ndef FIRST_MOVE_H_INCLUDED
