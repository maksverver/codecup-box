#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "random.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

using coord_t = uint8_t;
using color_t = uint8_t;

static constexpr coord_t HEIGHT      = 16;
static constexpr coord_t WIDTH       = 20;
static constexpr color_t COLORS      =  6;
static constexpr int     MAX_OVERLAP =  4;

using grid_t = std::array<std::array<color_t, WIDTH>, HEIGHT>;
using tile_t = std::array<color_t, COLORS>;

enum class Orientation : uint8_t {
  HORIZONTAL,
  VERTICAL,
};

static constexpr Orientation ORIENTATIONS[2] = {
  Orientation::HORIZONTAL,
  Orientation::VERTICAL,
};

inline bool IsHorizontal(const Orientation &ori) {
  return ori == Orientation::HORIZONTAL;
}

inline bool IsVertical(const Orientation &ori) {
  return ori == Orientation::VERTICAL;
}

struct Rect {
  coord_t r1, c1, r2, c2;
};

struct Placement {
  coord_t row, col;
  Orientation ori;

  // Verifies that the placed tile fits inside the board coordinates.
  bool IsInBounds() const {
    return
        static_cast<unsigned>(row) <= HEIGHT - (IsHorizontal(ori) ? 2 : 6) &&
        static_cast<unsigned>(col) <=  WIDTH - (IsHorizontal(ori) ? 6 : 2);
  }

  // Verifies that a tile can be placed on the grid so that it is adjacent to
  // an existing colored cell and its overlap doesn't exceed MAX_OVERLAP.
  bool IsValid(const grid_t &grid) const;

  static Placement Horizontal(int row, int col) {
    return Placement{
        .row = static_cast<coord_t>(row),
        .col = static_cast<coord_t>(col),
        .ori = Orientation::HORIZONTAL};
  }

  static Placement Vertical(int row, int col) {
    return Placement{
        .row = static_cast<coord_t>(row),
        .col = static_cast<coord_t>(col),
        .ori = Orientation::VERTICAL};
  }

  Rect GetBounds() const {
    coord_t r2 = row + (IsHorizontal(ori) ? 2 : COLORS);
    coord_t c2 = col + (IsHorizontal(ori) ? COLORS : 2);
    return {row, col, r2, c2};
  };

  auto operator<=>(const Placement&) const = default;
};

const Placement initial_placement = Placement::Horizontal(7, 7);

// Checks if the game is over.
//
// Currently this function is slow, so only suitable for calling in the outer
// game loop. (TODO: optimize this if it matters.)
bool IsGameOver(const grid_t &grid);

// Places a tile on the grid, overwriting the previous digits.
void ExecuteMove(grid_t &grid, const tile_t &tile, const Placement &placement);

// Returns a boolean grid where cells are 0 if they could still be changed by
// a future move, or 1 if they are fixed, because no valid move overlaps.
grid_t CalcFixed(const grid_t &grid);

struct Move {
  tile_t tile;
  Placement placement;

  bool IsValid(const grid_t &grid) const { return placement.IsValid(grid); }
  void Execute(grid_t &grid) { return ExecuteMove(grid, tile, placement); }
};

// I/O support

void DebugDumpGrid(grid_t grid, std::ostream &os = std::clog);

std::optional<color_t> ParseColor(char ch);
std::optional<coord_t> ParseRow(char ch);
std::optional<coord_t> ParseCol(char ch);
std::optional<Orientation> ParseOrientation(char ch);
std::optional<tile_t> ParseTile(std::string_view s);
std::optional<Move> ParseMove(std::string_view s);

std::string FormatPlacement(Placement placement);
std::string FormatTile(tile_t tile);
std::string FormatMove(Move move);

std::ostream &operator<<(std::ostream &os, Placement placement);
std::ostream &operator<<(std::ostream &os, tile_t tile);
std::ostream &operator<<(std::ostream &os, Move move);

#endif // ndef STATE_H_INCLUDED
