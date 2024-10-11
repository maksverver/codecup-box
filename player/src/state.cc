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

std::optional<color_t> ParseColor(char ch) {
  int color = ch - '0';
  if (color < 1 || color > 6) return std::nullopt;
  return color;
}

std::optional<coord_t> ParseRow(char ch) {
  int row = ch - 'A';
  if (row < 0 || row >= HEIGHT) return std::nullopt;
  return row;
}

std::optional<coord_t> ParseCol(char ch) {
  int col = ch - 'a';
  if (col < 0 || col >= WIDTH) return std::nullopt;
  return col;
}

std::optional<Orientation> ParseOrientation(char ch) {
  switch (ch) {
    case 'h': return Orientation::HORIZONTAL;
    case 'v': return Orientation::VERTICAL;
    default: return std::nullopt;
  }
}

std::optional<tile_t> ParseTile(std::string_view s) {
  if (s.size() != COLORS) return std::nullopt;
  tile_t tile = {};
  for (int i = 0; i < COLORS; ++i) {
    std::optional<color_t> color = ParseColor(s[i]);
    if (!color) return std::nullopt;
    if (std::find(&tile[0], &tile[i], *color) != &tile[i]) return std::nullopt;
    tile[i] = *color;
  }
  return tile;
}

std::optional<Move> ParseMove(std::string_view s) {
  if (s.size() != COLORS + 3) return std::nullopt;
  std::optional<coord_t> row = ParseRow(s[0]);
  std::optional<coord_t> col = ParseCol(s[1]);
  std::optional<tile_t> tile = ParseTile(s.substr(2, COLORS));
  std::optional<Orientation> ori = ParseOrientation(s.back());
  if (!row || !col || !tile || !ori) return std::nullopt;
  Placement placement = {*row, *col, *ori};
  if (!placement.IsInBounds()) return {};
  return Move{*tile, placement};
}

std::string FormatPlacement(Placement placement) {
  std::string s(3, '\0');
  s[0] = 'A' + placement.row;
  s[1] = 'a' + placement.col;
  s[2] = IsHorizontal(placement.ori) ? 'h' : 'v';
  return s;
}

std::string FormatTile(tile_t tile) {
  std::string s(tile.size(), '\0');
  for (size_t i = 0; i < tile.size(); ++i) s[i] = '0' + tile[i];
  return s;
}

std::string FormatMove(Move move) {
  std::string s(COLORS + 3, '\0');
  s[0] = 'A' + move.placement.row;
  s[1] = 'a' + move.placement.col;
  for (int i = 0; i < COLORS; ++i) s[i + 2]  = '0' + move.tile[i];
  s[COLORS + 2] = IsHorizontal(move.placement.ori) ? 'h' : 'v';
  return s;
}

std::ostream &operator<<(std::ostream &os, Placement placement) {
  return os
      << static_cast<char>('A' + placement.row)
      << static_cast<char>('a' + placement.col)
      << (IsHorizontal(placement.ori) ? 'h' : 'v');
}

std::ostream &operator<<(std::ostream &os, tile_t tile) {
  for (size_t i = 0; i < tile.size(); ++i) {
    if (!(os << static_cast<char>('0' + tile[i]))) break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, Move move) {
  return os
      << static_cast<char>('A' + move.placement.row)
      << static_cast<char>('a' + move.placement.col)
      << move.tile
      << (IsHorizontal(move.placement.ori) ? 'h' : 'v');
}

void DebugDumpGrid(grid_t grid, std::ostream &os) {
  for (int r = 0; r < HEIGHT; ++r) {
    for (int c = 0; c < WIDTH; ++c) {
      os << (grid[r][c] ? static_cast<char>('0' + grid[r][c]) : '.');
    }
    os << '\n';
  }
  os << std::endl;
}
