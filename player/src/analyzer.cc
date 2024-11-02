#include "analysis.h"
#include "options.h"
#include "state.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace {

DECLARE_OPTION(bool, arg_help, false, "help",
    "show usage information");

DECLARE_OPTION(int, color1, 0, "color1", "Player 1's secret color (if known)");
DECLARE_OPTION(int, color2, 0, "color2", "Player 2's secret color (if known)");

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

  SecretColorGuesser guesser[2] = {};
  int color_guess_last_incorrect[2] = {};
  std::array<int, COLORS> last_scores = {};
  grid_t grid = {};
  for (size_t move_index = 0; move_index < plain_args.size(); ++move_index) {
    const char *arg = plain_args[move_index];
    std::optional<Move> move = ParseMove(arg);
    if (!move) {
      std::cerr << "Could not parse move: " << arg << '\n';
      return EXIT_FAILURE;
    }
    if (!(move_index == 0 ? move->placement.IsInBounds() : move->IsValid(grid))) {
      std::cerr << "Move is not valid: " << arg << '\n';
      return EXIT_FAILURE;
    }
    move->Execute(grid);

    grid_t fixed = CalcFixed(grid);
    std::array<int, COLORS> scores = {};
    EvaluateAllColors(grid, fixed, scores);
    std::cerr << scores << '\n';
    if (move_index > 0) {
      int player = (move_index - 1) % 2;
      guesser[player].Update(last_scores, scores);
      if (guesser[player].Color(0) != (player == 0 ? color1 : color2)) {
        color_guess_last_incorrect[player] = move_index;
      }
    }
    last_scores = scores;

    for (int i = 1; i < COLORS; ++i) {
      for (int j = i + 1; j < COLORS; ++j) {
        assert(EvaluateTwoColors(grid, fixed, i, j) == scores[i - 1] - scores[j - 1]);
      }
    }
  }

  if (!GeneratePlacements(grid).empty()) {
    std::cerr << "Game is not over!\n";
  }

  std::cerr << '\n';
  DebugDumpGrid(grid, std::cerr);

  std::cerr << "Final scores:\n";
  std::array<int, COLORS> scores = {};
  EvaluateFinalScore(grid, scores);
  std::cerr << scores << '\n';

  if (color1 > 0 || color2 > 0) {
    std::cerr << '\n';
  }
  if (color1 > 0) {
    std::cerr << "Player 1 color (" << color1
        <<") guessed last incorrect on move "
        << (color_guess_last_incorrect[0] + 1) / 2 << '\n';
  }
  if (color2 > 0) {
    std::cerr << "Player 2 color (" << color2
        <<") guessed last incorrect on move "
        << color_guess_last_incorrect[1] / 2 << '\n';
  }
}
