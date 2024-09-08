#include "state.h"

bool Placement::IsValid() const {
  return true;  // TODO
}

bool Move::IsValid(const grid_t &grid) const {
  if (!placement.IsValid()) return false;
  return true;  // TODO
}

void Place(grid_t &grid, const Move &move) {
  if (move.placement.ori == Orientation::HORIZONTAL) {
    for (int i = 0; i < COLORS; ++i) {
      grid[move.placement.row + 0][move.placement.col + i] =
      grid[move.placement.row + 1][move.placement.col + COLORS - 1 - i] =
          move.tile[i];
    }
  } else {
    for (int i = 0; i < COLORS; ++i) {
      grid[move.placement.row + COLORS - 1 - i][move.placement.col + 0] =
      grid[move.placement.row + i][move.placement.col + 1] =
          move.tile[i];
    }
  }
}
