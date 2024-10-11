#include "analysis.h"
#include "fat-state.h"
#include "options.h"
#include "state.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace {

DECLARE_OPTION(bool, arg_help, false, "help",
    "show usage information");

std::ostream &operator<<(std::ostream &os, const std::array<int, COLORS> &scores) {
  for (size_t i = 0; i < scores.size(); ++i) {
    if (i > 0) os << ' ';
    os << std::setw(6) << scores[i];
  }
  return os;
}

} // namespace

int main(int argc, char *argv[]) {
  std::vector<char*> plain_args;
  if (!ParseOptions(argc, argv, plain_args) ||  plain_args.empty() || arg_help) {
    std::ostream &os = arg_help ? std::cout : std::clog;
    os << "Usage: analyze [<options>] <moves...>\n\nOptions:\n";
    PrintOptionUsage(os);
    return EXIT_FAILURE;
  }

  FatState state;
  for (char* arg : plain_args) {
    std::optional<Move> move = ParseMove(arg);
    if (!move) {
      std::cerr << "Could not parse move: " << arg << '\n';
      return EXIT_FAILURE;
    }
    int place_index = state.FindActivePlaceIndex(move->placement);
    if (place_index == -1) {
      std::cerr << "Move is not valid: " << arg << '\n';
      return EXIT_FAILURE;
    }
    old_tile_t old_tile;
    state.Place(move->tile, place_index, old_tile);
    std::array<int, COLORS> scores_x;
    EvaluateAllColorsX(state.grid, state.movecount, scores_x);

    std::cerr << scores_x << '\n';

    // Sanity check
    grid_t fixed = CalcFixed(state.grid);
    std::array<int, COLORS> scores;
    EvaluateAllColors(state.grid, fixed, scores);
    if (scores != scores_x) {
      std::cerr << "Inconsistent scores:\n" << scores << '\n';
      DebugDumpGrid(state.grid, std::cerr);
    }
    assert(scores == scores_x);
    for (int i = 1; i < COLORS; ++i) {
      for (int j = i + 1; j < COLORS; ++j) {
        std::cerr << i << ' ' << j << ' ' << EvaluateTwoColorsX(state.grid, state.movecount, i, j) << ' ' << scores[i - 1] - scores[j - 1] << '\n';
        assert(EvaluateTwoColorsX(state.grid, state.movecount, i, j) == scores[i - 1] - scores[j - 1]);
      }
    }
  }

  if (state.first_active != -1) {
    std::cerr << "Game is not over!\n";
  }

  std::cerr << '\n';
  DebugDumpGrid(state.grid, std::cerr);

  std::cerr << "\nFinal scores:\n";
  std::array<int, COLORS> scores = {};
  EvaluateFinalScore(state.grid, scores);
  std::cerr << scores << '\n';
}
