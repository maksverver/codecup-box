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
};

// Checks if the game is over.
//
// Currently this function is slow, so only suitable for calling in the outer
// game loop. (TODO: optimize this if it matters.)
bool IsGameOver(const grid_t &grid);

// Places a tile on the grid, overwriting the previous digits.
void ExecuteMove(grid_t &grid, const tile_t &tile, const Placement &placement);

struct Move {
  tile_t tile;
  Placement placement;

  bool IsValid(const grid_t &grid) const { return placement.IsValid(grid); }
  void Execute(grid_t &grid) { return ExecuteMove(grid, tile, placement); }
};

#endif // ndef STATE_H_INCLUDED
