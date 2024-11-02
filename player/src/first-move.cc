#include "first-move.h"

#include "analysis.h"
#include "first-move-table.h"
#include "state.h"

#include <stdio.h>

namespace {

void PrintFirstTile(std::ostream &os, const tile_t &tile) {
  os << "{";
  for (size_t i = 0; i < tile.size(); ++i) {
    if (i > 0) os << ", ";
    os << (int) tile[i];
  }
  os << "}";  
}

void PrintFirstPlacement(std::ostream &os, const Placement &placement) {
  os << "{"
     << (int) placement.row << ", "
     << (int) placement.col << ", "
     << (IsHorizontal(placement.ori) ? "Orientation::HORIZONTAL" : "Orientation::VERTICAL") << "}";
}

static int64_t Factorial(int i) {
  return i <= 1 ? 1 : i * Factorial(i - 1);
}

color_t MapColor(const tile_t &first_tile, color_t color) {
  for (size_t i = 0; i < first_tile.size(); ++i) {
    if (first_tile[i] == color) return i + 1;
  }
  assert(false);
  return -1;
}

}  // namespace

std::vector<BestFirstMove> CalculateBestFirstMoves(
  std::function<std::vector<Placement>(
    int color, const grid_t &grid, const tile_t &tile,
    const std::vector<Placement> &all_placements)> find_best_placements) {
  std::vector<BestFirstMove> res;
  grid_t grid = {};
  tile_t tile;
  for (size_t i = 0; i < tile.size(); ++i) tile[i] = i + 1;
  ExecuteMove(grid, tile, initial_placement);
  long long total = Factorial(tile.size()) * COLORS;
  long long done = 0;
  const std::vector<Placement> all_placements = GeneratePlacements(grid);
  for (int color = 1; color <= COLORS; ++color) {
    do {
      for (const Placement placement : find_best_placements(color, grid, tile, all_placements)) {
        res.push_back({.color = color, .tile=tile, .best_placement=placement});
      }
      ++done;
      fprintf(stderr, "\r%lld / %lld (%6.3f%%) done", done, total, 100.0*done/total);
    } while (std::ranges::next_permutation(tile).found);
  }
  return res;
}

void PrintBestFirstMoves(std::ostream &os, const std::vector<BestFirstMove> &moves) {
  os << "const BestFirstMove best_first_moves[" << moves.size() << "] = {\n";
  for (const BestFirstMove &move : moves) {
    os << "    {" << move.color << ", ";
    PrintFirstTile(os, move.tile);
    os << ", ";
    PrintFirstPlacement(os, move.best_placement);
    os << "},\n";
  }
  os << "};\n";
}

std::vector<Placement> FindBestFirstMoves(
    int secret_color, const Move first_move, const tile_t &tile) {
  assert(first_move.placement == initial_placement);
  int mapped_color = MapColor(first_move.tile, secret_color);
  tile_t mapped_tile = tile;
  for (color_t &c: mapped_tile) c = MapColor(first_move.tile, c);

  std::vector<Placement> res;
  // TODO: speed this up with binary search?
  for (const BestFirstMove &move : best_first_moves) {
    if (move.color == mapped_color && move.tile == mapped_tile) {
      res.push_back(move.best_placement);
    }
  }
  return res;
}
