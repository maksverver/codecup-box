#include "analysis.h"

#include "options.h"
#include "state.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

const struct ScoreWeights {
  int base4, fixed4;
  int base3, fixed3;
  int base2, fixed2;
  int base1, fixed1;
} default_score_weights = {
    250, 2500,
    100, 1000,
     10,  100,
      1,   10};

bool ParseScoreWeights(std::string_view s, ScoreWeights &weights) {
  size_t i = -1;
  return sscanf(std::string(s).c_str(), "%d,%d,%d,%d,%d,%d,%d,%d%zn",
    &weights.base4, &weights.fixed4,
    &weights.base3, &weights.fixed3,
    &weights.base2, &weights.fixed2,
    &weights.base1, &weights.fixed1, &i) == 8 && i == s.size();
}

std::string FormatScoreWeights(const ScoreWeights &weights) {
  char buf[180];
  snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d,%d,%d",
    weights.base4, weights.fixed4,
    weights.base3, weights.fixed3,
    weights.base2, weights.fixed2,
    weights.base1, weights.fixed1);
  return buf;
}

ScoreWeights arg_score_weights =
  (RegisterOption(
      "score-weights",
      "Weights used by the evaluation function.",
      FormatScoreWeights(default_score_weights),
      [](std::string_view s) {
        return ParseScoreWeights(s, arg_score_weights);
      }),
  default_score_weights);

// For reference, the original evaluation function
// (now superceded by square_points_memo):
#if 0
int EvalSquarePoints(
    bool a, bool b, bool c, bool d,
    bool fa, bool fb, bool fc, bool fd,
    int size) {
  int num_fixed = fa + fb + fc + fd;
  if (a && b && c && d) {
    // Square!
    return 1000 + 100*num_fixed + 100*size;
  } else if (
        (a && b && c && !fd) ||
        (a && b && d && !fc) ||
        (a && c && d && !fb) ||
        (b && c && d && !fa)) {
    // One cell short of a square.
    return 100 + 10*num_fixed + 10*size;
  } else if (
      (a && b && !fc && !fd) ||
      (a && c && !fb && !fd) ||
      (a && d && !fb && !fc) ||
      (b && c && !fa && !fd) ||
      (b && d && !fa && !fc) ||
      (c && d && !fa && !fb)) {
    // Two points aligned horizontally, vertically, or diagonally.
    // Maybe: assign a different score for the diagonal version?
    return 10 + 1*num_fixed + 1*size;
  } else {
    return 0;
  }
}
#endif

short square_points_memo[2][2][2][2][2][2][2][2];

void InitializeSquarePointsMemo(const ScoreWeights &weights) {
  for (int a = 0; a < 2; ++a) {
    for (int b = 0; b < 2; ++b) {
      for (int c = 0; c < 2; ++c) {
        for (int d = 0; d < 2; ++d) {
          for (int fa = 0; fa < 2; ++fa) {
            for (int fb = 0; fb < 2; ++fb) {
              for (int fc = 0; fc < 2; ++fc) {
                for (int fd = 0; fd < 2; ++fd) {
                  auto &base = square_points_memo[a][b][c][d][fa][fb][fc][fd];
                  int num_fixed = fa + fb + fc + fd;
                  if (a && b && c && d) {
                    // Square!
                    base = weights.base4 + weights.fixed4*num_fixed;
                  } else if (
                        (a && b && c && !fd) ||
                        (a && b && d && !fc) ||
                        (a && c && d && !fb) ||
                        (b && c && d && !fa)) {
                    // One cell short of a square.
                    base = weights.base3 + weights.fixed3*num_fixed;
                  } else if (
                      (a && b && !fc && !fd) ||
                      (a && c && !fb && !fd) ||
                      (a && d && !fb && !fc) ||
                      (b && c && !fa && !fd) ||
                      (b && d && !fa && !fc) ||
                      (c && d && !fa && !fb)) {
                    // Two points aligned horizontally, vertically, or diagonally.
                    // Maybe: assign a different score for the diagonal version?
                    base = weights.base2 + weights.fixed2*num_fixed;
                  } else {
                    base = 0;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

static int EvalSquarePointsMemoized(
    bool a, bool b, bool c, bool d,
    bool fa, bool fb, bool fc, bool fd,
    int size) {
  int base = square_points_memo[a][b][c][d][fa][fb][fc][fd];
  return base * (size + 4);  // +4 determined empirically, though effect is small
}

}  // namespace

void InitializeAnalysis() {
  InitializeSquarePointsMemo(arg_score_weights);
}

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

int Evaluate1(const grid_t &fixed, int r, int c) {
  return fixed[r][c] ? arg_score_weights.fixed1 : arg_score_weights.base1;
}

int EvaluateRectangle(const grid_t &grid, const grid_t &fixed, color_t color, int r1, int c1, int r2, int c2) {
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
  return EvalSquarePointsMemoized(a, b, c, d, fa, fb, fc, fd, r2 - r1);
  // int res = EvalSquarePointsMemoized(a, b, c, d, fa, fb, fc, fd, r2 - r1);
  // assert(res == EvalSquarePoints(a, b, c, d, fa, fb, fc, fd, r2 - r1));
  // return res;
}

void EvaluateAllColors(const grid_t &grid, const grid_t &fixed, std::array<int, COLORS> &scores) {
  scores = {};
  for (int color = 1; color <= COLORS; ++color) {
    int score = 0;
    for (int r1 = 0; r1 < HEIGHT; ++r1) {
      for (int c1 = 0; c1 < WIDTH; ++c1) {
        if (grid[r1][c1] == color) {
          score += Evaluate1(fixed, r1, c1);
        }
        for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
          score += EvaluateRectangle(grid, fixed, color, r1, c1, r2, c2);
        }
      }
    }
    scores[color - 1] += score;
  }
}

int EvaluateTwoColors(const grid_t &grid, const grid_t &fixed, int my_color, int his_color) {
  int res = 0;
  for (int r = 0; r < HEIGHT; ++r) {
    for (int c = 0; c < WIDTH; ++c) {
      if (grid[r][c] == my_color) {
        res += Evaluate1(fixed, r, c);
      } else if (grid[r][c] == his_color) {
        res -= Evaluate1(fixed, r, c);
      }
    }
  }
  for (int r1 = 0; r1 < HEIGHT; ++r1) {
    for (int c1 = 0; c1 < WIDTH; ++c1) {
      if (grid[r1][c1] == my_color) {
        //  xx  x.   x.
        //  ..  x.   .x
        //
        //  xx  x.   xx
        //  x.  xx   .x
        //
        for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
          res += EvaluateRectangle(grid, fixed, my_color, r1, c1, r2, c2);
        }
        //  .x ..
        //  x. xx
        //
        //  .x
        //  xx
        //
        for (int r2 = r1 - 1, c2 = c1 + 1; r2 >= 0 && c2 < WIDTH; --r2, ++c2) {
          if (grid[r2][c1] != my_color) {
            res += EvaluateRectangle(grid, fixed, my_color, r2, c1, r1, c2);
          }
        }
        //  .x
        //  .x
        for (int r2 = r1 - 1, c2 = c1 - 1; r2 >= 0 && c2 >=0; --r2, --c2) {
          if (grid[r1][c2] != my_color && grid[r2][c2] != my_color) {
            res += EvaluateRectangle(grid, fixed, my_color, r2, c2, r1, c1);
          }
        }
      } else if (grid[r1][c1] == his_color) {
        //  xx  x.   x.
        //  ..  x.   .x
        //
        //  xx  x.   xx
        //  x.  xx   .x
        //
        for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
          res -= EvaluateRectangle(grid, fixed, his_color, r1, c1, r2, c2);
        }
        //  .x  ..
        //  x.  xx
        //
        //  .x
        //  xx
        //
        for (int r2 = r1 - 1, c2 = c1 + 1; r2 >= 0 && c2 < WIDTH; --r2, ++c2) {
          if (grid[r2][c1] != his_color) {
            res -= EvaluateRectangle(grid, fixed, his_color, r2, c1, r1, c2);
          }
        }
        //  .x
        //  .x
        for (int r2 = r1 - 1, c2 = c1 - 1; r2 >= 0 && c2 >=0; --r2, --c2) {
          if (grid[r1][c2] != his_color && grid[r2][c2] != his_color) {
            res -= EvaluateRectangle(grid, fixed, his_color, r2, c2, r1, c1);
          }
        }
      }
    }
  }
  return res;
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
