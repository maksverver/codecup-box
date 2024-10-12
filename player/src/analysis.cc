#include "analysis.h"

#include "state.h"

std::vector<Placement> GeneratePlacements(const grid_t &grid) {
  std::vector<Placement> placements;
  for (coord_t row = 0; row < HEIGHT; ++row) {
    for (coord_t col = 0; col < WIDTH; ++col) {
      for (Orientation ori : ORIENTATIONS) {
        Placement placement = {row, col, ori};
        if (placement.IsValid(grid)) {
          placements.push_back(placement);
        }
      }
    }
  }
  return placements;
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

static int EvaluateRectangle(const grid_t &grid, const grid_t &fixed, color_t color, int r1, int c1, int r2, int c2) {
  //  a  b
  //  c  d
  bool a = grid[r1][c1] == color;
  bool b = grid[r1][c2] == color;
  bool c = grid[r2][c1] == color;
  bool d = grid[r2][c2] == color;
  bool fa = fixed[r1][c1];
  bool fb = fixed[r1][c2];
  bool fc = fixed[r2][c1];
  bool fd = fixed[r2][c2];
  int size = r2 - r1;
  int num_fixed = fa + fb + fc + fd;
  int score = 0;
  if (a && b && c && d) {
    // Square!
    score += 1000 + 100*num_fixed + 100*size;
  } else if (
        (a && b && c && !fd) ||
        (a && b && d && !fc) ||
        (a && c && d && !fb) ||
        (b && c && d && !fa)) {
    // One cell short of a square.
    score += 100 + 10*num_fixed + 10*size;
  } else if (
      (a && b && !fc && !fd) ||
      (a && c && !fb && !fd) ||
      (a && d && !fb && !fc) ||
      (b && c && !fa && !fd)
    // These are excluded to avoid double-processing as edges a-b and a-c
    // (b && d && !fa && !fc) ||
    // (c && d && !fa && !fb)
  ) {
    // Two points aligned horizontally, vertically, or diagonally.
    // Maybe: assign a different score for the diagonal version?
    score += 10 + 1*num_fixed + 1*size;
  }
  return score;
}

void EvaluateAllColors(const grid_t &grid, const grid_t &fixed, std::array<int, COLORS> &scores) {
  scores = {};
  for (int color = 1; color <= COLORS; ++color) {
    int score = 0;
    for (int r1 = 0; r1 < HEIGHT; ++r1) {
      for (int c1 = 0; c1 < WIDTH; ++c1) {
        if (grid[r1][c1] == color) {
          // Always assign a point to each cell.
          score += 1;
        }
        for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
          score += EvaluateRectangle(grid, fixed, color, r1, c1, r2, c2);
        }
      }
    }
    scores[color - 1] += score;
  }
}

void EvaluateFinalScore(const grid_t &grid, std::array<int, COLORS> &scores) {
  scores = {};
  for (int r1 = 0; r1 < HEIGHT; ++r1) {
    for (int c1 = 0; c1 < WIDTH; ++c1) {
      color_t color = grid[r1][c1];
      if (color < 1 || color > COLORS) continue;
      for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
        if (grid[r1][c2] == color && grid[r2][c1] == color && grid[r2][c2] == color) {
          scores[color - 1] += r2 - r1;
        }
      }
    }
  }
}
