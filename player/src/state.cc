#include "state.h"

namespace {

int CountOverlap(const grid_t &grid, int row, int col, Orientation ori) {
  int result = 0;
  if (IsHorizontal(ori)) {
    for (int i = 0; i < COLORS; ++i) {
      if (grid[row][col + i]) ++result;
      if (grid[row + 1][col + COLORS - 1 - i]) ++result;
    }
  } else {
    for (int i = 0; i < COLORS; ++i) {
      if (grid[row + COLORS - 1 - i][col]) ++result;
      if (grid[row + i][col + 1]) ++result;
    }
  }
  return result;
}

// Checks if the tile is placed adjecent to an occupied cell of the grid.
// Note that the corners don't count; one of the edges of the tile must touch.
bool IsAdjacent(const grid_t &grid, int r1, int c1, Orientation ori) {
  int r2 = r1 + (IsHorizontal(ori) ? 2 : COLORS);
  int c2 = c1 + (IsHorizontal(ori) ? COLORS : 2);
  if (c1 > 0) {
    for (int r = r1; r < r2; ++r) if (grid[r][c1 - 1] != 0) return true;
  }
  if (c2 < WIDTH) {
    for (int r = r1; r < r2; ++r) if (grid[r][c2] != 0) return true;
  }
  if (r1 > 0) {
    for (int c = c1; c < c2; ++c) if (grid[r1 - 1][c] != 0) return true;
  }
  if (r2 < HEIGHT) {
    for (int c = c1; c < c2; ++c) if (grid[r2][c] != 0) return true;
  }
  return false;
}

}  // namespace

bool Placement::IsValid(const grid_t &grid) const {
  if (!IsInBounds()) return false;
  int overlap = CountOverlap(grid, row, col, ori);
  if (overlap > MAX_OVERLAP) return false;
  if (overlap > 0) return true;
  return IsAdjacent(grid, row, col, ori);
}

// The game is over if and only if there is no 6x2 rectangular area of the grid
// (either horizontally or vertically) that contains at most 4 colored cells.
// Not all of these rectangular areas are valid moves (since new tiles must be
// placed adjacent to colored cells) but at least one of them must be.
bool IsGameOver(const grid_t &grid) {
  // TODO: speed this up?
  for (int r = 0; r <= HEIGHT - 2; ++r) {
    for (int c = 0; c <= WIDTH - COLORS; ++c) {
      int overlap = 0;
      for (int i = 0; i < 6; ++i) {
        overlap += grid[r][c + i] != 0;
        overlap += grid[r + 1][c + i] != 0;
      }
      if (overlap <= 4) return false;
    }
  }
  for (int r = 0; r <= HEIGHT - COLORS; ++r) {
    for (int c = 0; c <= WIDTH - 2; ++c) {
      int overlap = 0;
      for (int i = 0; i < 6; ++i) {
        overlap += grid[r + i][c] != 0;
        overlap += grid[r + i][c + 1] != 0;
      }
      if (overlap <= 4) return false;
    }
  }
  return true;
}

grid_t CalcFixed(const grid_t &grid) {
  grid_t fixed;
  for (int r = 0; r < HEIGHT; ++r) {
    for (int c = 0; c < WIDTH; ++c) {
      fixed[r][c] = 1;
    }
  }

  // TODO: speed this up?
  for (int r = 0; r <= HEIGHT - 2; ++r) {
    for (int c = 0; c <= WIDTH - COLORS; ++c) {
      int overlap = 0;
      for (int i = 0; i < 6; ++i) {
        overlap += grid[r][c + i] != 0;
        overlap += grid[r + 1][c + i] != 0;
      }
      if (overlap <= 4) {
        for (int i = 0; i < 6; ++i) {
          fixed[r][c + i] = fixed[r + 1][c + i] = 0;
        }
      }
    }
  }
  for (int r = 0; r <= HEIGHT - COLORS; ++r) {
    for (int c = 0; c <= WIDTH - 2; ++c) {
      int overlap = 0;
      for (int i = 0; i < 6; ++i) {
        overlap += grid[r + i][c] != 0;
        overlap += grid[r + i][c + 1] != 0;
      }
      if (overlap <= 4) {
        for (int i = 0; i < 6; ++i) {
          fixed[r + i][c] = fixed[r + i][c + 1] = 0;
        }
      }
    }
  }
  return fixed;
}

void ExecuteMove(grid_t &grid, const tile_t &tile, const Placement &placement) {
  auto [row, col, ori] = placement;
  if (IsHorizontal(ori)) {
    for (int i = 0; i < COLORS; ++i) {
      grid[row][col + i] = grid[row + 1][col + COLORS - 1 - i] = tile[i];
    }
  } else {
    for (int i = 0; i < COLORS; ++i) {
      grid[row + COLORS - 1 - i][col] = grid[row + i][col + 1] = tile[i];
    }
  }
}
