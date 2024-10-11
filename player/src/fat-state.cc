#include "fat-state.h"
#include "state.h"

#include <ranges>

namespace {

/*
void PlaceOnGrid(grid_t &grid, const tile_t &tile, const Placement &placement,
    old_tile_t &old_tile) {
  auto [row, col, ori] = placement;
  if (IsHorizontal(ori)) {
    for (int i = 0; i < COLORS; ++i) {
      old_tile[i + 0*COLORS] = grid[row + 0][col + i];
      old_tile[i + 1*COLORS] = grid[row + 1][col + i];
    }
    for (int i = 0; i < COLORS; ++i) {
      grid[row][col + i] = grid[row + 1][col + COLORS - 1 - i] = tile[i];
    }
  } else {
    for (int i = 0; i < COLORS; ++i) {
      old_tile[i + 0*COLORS] = grid[row + 0][col + i];
      old_tile[i + 1*COLORS] = grid[row + 1][col + i];
    }
    for (int i = 0; i < COLORS; ++i) {
      grid[row + COLORS - 1 - i][col] = grid[row + i][col + 1] = tile[i];
    }
  }
}

void UnplaceOnGrid(grid_t &grid, old_tile_t &old_tile, const Placement &placement) {
  auto [row, col, ori] = placement;
  if (IsHorizontal(ori)) {
    for (int i = 0; i < COLORS; ++i) {
      old_tile[i + 0*COLORS] = grid[row + 0][col + i];
      old_tile[i + 1*COLORS] = grid[row + 1][col + i];
    }
    for (int i = 0; i < COLORS; ++i) {
      grid[row][col + i] = grid[row + 1][col + COLORS - 1 - i] = tile[i];
    }
  } else {
    for (int i = 0; i < COLORS; ++i) {
      old_tile[i + 0*COLORS] = grid[row + 0][col + i];
      old_tile[i + 1*COLORS] = grid[row + 1][col + i];
    }
    for (int i = 0; i < COLORS; ++i) {
      grid[row + COLORS - 1 - i][col] = grid[row + i][col + 1] = tile[i];
    }
  }
}
*/

old_tile_t ExpandTile(const tile_t &tile, const Placement &placement) {
  old_tile_t res;
  if (IsHorizontal(placement.ori)) {
    for (int i = 0; i < COLORS; ++i) {
      res[i] = res[2*COLORS - 1 - i] = tile[i];
    }
  } else {
    for (int i = 0; i < COLORS; ++i) {
      res[2*i + 1] = res[2*COLORS - 2 - 2*i] = tile[i];
    }
  }
  return res;
}

void UpdateMoveCount(grid_t &grid, Rect rect, signed char val) {
  auto [r1, c1, r2, c2] = rect;
  for (int r = r1; r < r2; ++r) {
    for (int c = c1; c < c2; ++c) {
      grid[r][c] += val;
    }
  }
}

int EvaluateSquare(const grid_t &grid, const grid_t &movecount, int r1, int c1, int r2, int c2, color_t color) {
  // a b
  // c d
  int size = abs(r2 - r1);
  bool a = grid[r1][c1] == color;
  bool b = grid[r1][c2] == color;
  bool c = grid[r2][c1] == color;
  bool d = grid[r2][c2] == color;
  bool fa = grid[r1][c1] != 0 && !movecount[r1][c1];
  bool fb = grid[r1][c2] != 0 && !movecount[r1][c2];
  bool fc = grid[r2][c1] != 0 && !movecount[r2][c1];
  bool fd = grid[r2][c2] != 0 && !movecount[r2][c2];
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

/*
int EvaluateScoreAt(const grid_t &grid, const grid_t &movecount, color_t color, int r1, int c1) {
  int score = grid[r1][c1] == color;
  for (int r2 = r1 - 1, c2 = c1 - 1; r2 >= 0 && c2 >= 0; --r2, --c2) {
    score += EvaluateSquare(grid, movecount, r1, c1, r2, c2, color);
  }
  for (int r2 = r1 - 1, c2 = c1 + 1; r2 >= 0 && c2 < WIDTH; --r2, ++c2) {
    score += EvaluateSquare(grid, movecount, r1, c1, r2, c2, color);
  }
  for (int r2 = r1 + 1, c2 = c1 - 1; r2 < HEIGHT && c2 >= 0; ++r2, --c2) {
    score += EvaluateSquare(grid, movecount, r1, c1, r2, c2, color);
  }
  for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
    score += EvaluateSquare(grid, movecount, r1, c1, r2, c2, color);
  }
  return score;
}
*/

}  // namespace

FatState::FatState() {
  int first_move = -1;
  for (int r = 0; r <= HEIGHT - 2; ++r) {
    for (int c = 0; c <= WIDTH - COLORS; ++c) {
      if (r == (HEIGHT - 2) / 2 && c == (WIDTH - COLORS) / 2) {
        assert(first_move == -1);
        first_move = all_placements.size();
      }
      all_placements.push_back(FatPlacement(r, c, r + 2, c + COLORS, Orientation::HORIZONTAL));
    }
  }
  for (int r = 0; r <= HEIGHT - COLORS; ++r) {
    for (int c = 0; c <= WIDTH - 2; ++c) {
      all_placements.push_back(FatPlacement(r, c, r + COLORS, c + 2, Orientation::VERTICAL));
    }
  }
  assert(first_move != -1);
  GetPlace(first_move).adjacent++;  // fake-activate
  Activate(first_move);

  int n = all_placements.size();

  assert(n == 434);

  for (int i = 0; i < n; ++i) {
    const auto &fp = all_placements[i];
    for (int r = fp.place.row; r < fp.r2; ++r) {
      for (int c = fp.place.col; c < fp.c2; ++c) {
        overlapping_placements[r][c].push_back(i);
        movecount[r][c]++;
      }
    }
  }

  // Adjacent on right:        Adjacent on bottom:
  //
  //    c1    c2             .       c1    c2           .
  //  r1 +-----+             .     r1 +-----+           .
  //     |     |-----+ r3    .        |     |           .
  //  r2 +-----+     |       .     r2 +-----+---+ r3    .
  //           |     |       .           |      |       .
  //           +-----+ r4    .           +------+ r4    .
  //          c3    c4       .          c3     c4       .
  //
  // Note: left and top are handled by symmetry (e.g., if a is left of b,
  // then b must be right of a).
  for (int i = 0; i < n; ++i) {
    auto &p1 = GetPlace(i);
    auto [r1, c1, r2, c2] = p1.GetRect();
    for (int j = 0; j < n; ++j) if (i != j) {
      auto &p2 = GetPlace(j);
      auto [r3, c3, r4, c4] = p2.GetRect();
      if ((c2 == c3 && r3 < r2 && r4 > r1) ||
          (r2 == r3 && c3 < c2 && c4 > c1)) {
        p1.adjacent_placements.push_back(j);
        p2.adjacent_placements.push_back(i);
      }
    }
  }
}

void FatState::Place(const tile_t &tile, int place_index, old_tile_t &old_tile) {
//std::cerr << "Place(" << place_index << ")\n";
  const auto &fp = GetPlace(place_index);
  assert(fp.Active());
  old_tile_t new_tile = ExpandTile(tile, fp.place);
  auto [r1, c1, r2, c2] = fp.GetRect();
  size_t i = 0;
  for (int r = r1; r < r2; ++r) {
    for (int c = c1; c < c2; ++c) {
      /*
      for (int color = 1; color <= COLORS; ++color) {
        scores[color - 1] -= EvaluateScoreAt(grid, movecount, color, r, c);
      }
      */
      old_tile[i] = grid[r][c];
      if (grid[r][c] == 0) {
        for (int j : overlapping_placements[r][c]) {
          auto &fp2 = GetPlace(j);
          int old_overlap = fp2.overlap++;
          if (old_overlap == 0 && fp2.adjacent == 0) Activate(j);
          if (old_overlap == 4) Deactivate(j);
        }
      }
      grid[r][c] = new_tile[i++];
      /*
      for (int color = 1; color <= COLORS; ++color) {
        scores[color - 1] += EvaluateScoreAt(grid, movecount, color, r, c);
      }
      */
    }
  }
  for (int j : fp.adjacent_placements) {
    auto &fp2 = GetPlace(j);
    int old_adjacent = fp2.adjacent++;
    if (old_adjacent == 0 && fp2.overlap == 0) Activate(j);
  }
}

void FatState::Unplace(const old_tile_t &old_tile, int place_index) {
//std::cerr << "Unplace(" << place_index << ")\n";
  auto &fp = GetPlace(place_index);
  auto [r1, c1, r2, c2] = fp.GetRect();
  for (int j : std::views::reverse(fp.adjacent_placements)) {
    auto &fp2 = GetPlace(j);
    assert(fp2.adjacent > 0);
    --fp2.adjacent;
    if (fp2.adjacent == 0 && fp2.overlap == 0) Deactivate(j);
  }
  size_t i = old_tile.size();
  for (int r = r2 - 1; r >= r1; --r) {
    for (int c = c2 - 1; c >= c1; --c) {
      /*
      for (int color = 1; color <= COLORS; ++color) {
        scores[color - 1] -= EvaluateScoreAt(grid, movecount, color, r, c);
      }
      */
      grid[r][c] = old_tile[--i];
      if (grid[r][c] == 0) {
        for (int j : std::views::reverse(overlapping_placements[r][c])) {
          auto &fp2 = GetPlace(j);
          assert(fp2.overlap > 0);
          --fp2.overlap;
          if (fp2.overlap == 0 && fp2.adjacent == 0) Deactivate(j);
          if (fp2.overlap == 4) Reactivate(j);
        }
      }
      /*
      for (int color = 1; color <= COLORS; ++color) {
        scores[color - 1] += EvaluateScoreAt(grid, movecount, color, r, c);
      }
      */
    }
  }
}

void FatState::Activate(int place_index) {
//std::cerr << "Activate(" << place_index << ")\n";
  FatPlacement &p = all_placements[place_index];
  assert(p.Active());
  //UpdateMoveCount(movecount, p.GetRect(), 1);  // important!
  p.next = first_active;
  p.prev = -1;
  if (first_active != -1) all_placements[first_active].prev = place_index;
  first_active = place_index;
}

void FatState::Deactivate(int place_index) {
//std::cerr << "Deactivate(" << place_index << ")\n";
  FatPlacement &p = all_placements[place_index];
  assert(!p.Active());
  UpdateMoveCount(movecount, p.GetRect(), -1);
  if (p.prev == -1) {
    first_active = p.next;
  } else {
    all_placements[p.prev].next = p.next;
  }
  if (p.next != -1) all_placements[p.next].prev = p.prev;
  // preserve p.next and p.prev to allow reactivation in place (see below)
}

void FatState::Reactivate(int place_index) {
//std::cerr << "Reactivate(" << place_index << ")\n";
  FatPlacement &p = all_placements[place_index];
  assert(p.Active());

  if (p.prev == -1) {
    assert(first_active == p.next);
    first_active = place_index;
  } else {
    assert(all_placements[p.prev].next == p.next);
    all_placements[p.prev].next = place_index;
  }

  if (p.next != -1) {
    assert(all_placements[p.next].prev == p.prev);
    all_placements[p.next].prev = place_index;
  }
}

int FatState::FindActivePlaceIndex(const Placement &placement) {
  // Note: this could be made much more efficient since valid placements have
  // fixed indices, regardless of whether they are active or not.
  int i = first_active;
  while (i != -1) {
    const auto &fp = GetPlace(i);
    assert(fp.Active());
// std::cerr << (int) fp.place.row << ' ' << (int) fp.place.col << ' ' << (int) fp.place.ori << '\n';
// std::cerr << (int) placement.row << ' ' << (int) placement.col << ' ' << (int) placement.ori << '\n';
    if (fp.place == placement) return i;
    i = fp.next;
  }
  return -1;
}
