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

static constexpr int HEIGHT = 16;
static constexpr int WIDTH  = 20;
static constexpr int COLORS =  6;

enum class Orientation : uint8_t {
  HORIZONTAL,
  VERTICAL,
};

using coord_t = uint8_t;
using color_t = uint8_t;
using grid_t = std::array<std::array<color_t, WIDTH>, HEIGHT>;
using tile_t = std::array<color_t, COLORS>;

struct Placement {
  coord_t row, col;
  Orientation ori;

  bool IsValid() const;
};

struct Move {
  tile_t tile;
  Placement placement;

  bool IsValid(const grid_t &grid) const;
};

void Place(grid_t &grid, const Move &move);

#endif // ndef STATE_H_INCLUDED
