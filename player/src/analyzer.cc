#include "analysis.h"
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

  grid_t grid = {};
  for (char* arg : plain_args) {
    std::optional<Move> move = ParseMove(arg);
    if (!move) {
      std::cerr << "Could not parse move: " << arg << '\n';
      return EXIT_FAILURE;
    }
    if (!(arg == plain_args[0] ? move->placement.IsInBounds() : move->IsValid(grid))) {
      std::cerr << "Move is not valid: " << arg << '\n';
      return EXIT_FAILURE;
    }
    move->Execute(grid);

    grid_t fixed = CalcFixed(grid);
    std::array<int, COLORS> scores = {};
    EvaluateAllColors(grid, fixed, scores);
    std::cerr << scores << '\n';

    for (int i = 1; i < COLORS; ++i) {
      for (int j = i + 1; j < COLORS; ++j) {
        std::cerr << i << ' ' << j << ' ' << EvaluateTwoColors(grid, fixed, i, j) << ' ' << scores[i - 1] - scores[j - 1] << '\n';
        assert(EvaluateTwoColors(grid, fixed, i, j) == scores[i - 1] - scores[j - 1]);
      }
    }
  }

  if (!GeneratePlacements(grid).empty()) {
    std::cerr << "Game is not over!\n";
  }

  std::cerr << '\n';
  DebugDumpGrid(grid, std::cerr);

  std::cerr << "\nFinal scores:\n";
  std::array<int, COLORS> scores = {};
  EvaluateFinalScore(grid, scores);
  std::cerr << scores << '\n';
}
