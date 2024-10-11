#ifndef FAT_STATE_H_INCLUDED
#define FAT_STATE_H_INCLUDED

#include "state.h"

using old_tile_t = std::array<color_t, COLORS*2>;

struct Rect {
  int r1, c1, r2, c2;
};

// During a game, a placement can become available only once (when a tile is
// placed on an adjacent/overlapping field) and become unavailable only once
// (when the number of overlapping fields exceeds 4). That means we can
// precalculate for each placement the adjacent/overlapping placements, and use
// those to dynamically update the list of available placements.
//
// That allows us to keep available placements in a linked list.
//
// This can (probably?) be extended to update the set of fixed tiles by counting
// the number of (pre)activated placements that could still cover a particular
// tile.
//
// Maybe TODO: separate this into constant state and dynamic state (overlap/it)?
struct FatPlacement {
  FatPlacement(int r1, int c1, int r2, int c2, Orientation ori)
    : r2(r2), c2(c2), place(r1, c1, ori) {}
  FatPlacement(const FatPlacement &) = default;
  FatPlacement(FatPlacement &&) = default;
  FatPlacement &operator=(const FatPlacement &) = default;
  FatPlacement &operator=(FatPlacement &&) = default;

  // Number of placed tiles adjacent to this one
  int adjacent = 0;

  // Number of occupied grid cells overlapping this placement
  int overlap = 0;

  bool Active() const { return (adjacent > 0 || overlap > 0) && (overlap <= 4); }

  // Position in the active-placements list.
  int prev = -1, next = -1;

  // TODO: more efficient data structures
  std::vector<int> adjacent_placements;

  int r2, c2;
  Placement place;

  Rect GetRect() const { return {.r1=place.row, .c1=place.col, .r2=r2, .c2=c2}; }
};

struct FatState {
  FatState();
  FatState(const FatState &) = default;
  FatState(FatState &&) = default;
  FatState &operator=(const FatState &) = default;
  FatState &operator=(FatState &&) = default;

  grid_t grid = {};
  grid_t movecount = {};
//  std::array<int, COLORS> scores = {};

  // Index in all_placements.
  int first_active = -1;

  std::vector<FatPlacement> all_placements;

  // TODO: more efficient data structure
  std::array<std::array<std::vector<int>, WIDTH>, HEIGHT> overlapping_placements = {};

  FatPlacement &GetPlace(int place_index) { return all_placements[place_index]; }

  void Place(const tile_t &tile, int place_index, old_tile_t &old_tile);
  void Unplace(const old_tile_t &old_tile, int place_index);

  int FindActivePlaceIndex(const Placement &placement);

private:
  void Activate(int tile_index);
  void Deactivate(int tile_index);
  void Reactivate(int tile_index);
};

FatState InitializeFatState();

#endif // ndef FAT_STATE_H_INCLUDED
