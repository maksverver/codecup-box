#include "analysis.h"
#include "options.h"
#include "logging.h"
#include "random.h"
#include "state.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#ifndef LOCAL_BUILD
#define LOCAL_BUILD 0
#endif

namespace {

const std::string player_name = "L7";

DECLARE_OPTION(bool, arg_help, false, "help",
    "show usage information");

DECLARE_OPTION(bool, arg_deep, false, "deep",
    "Search deeper (2 ply instead of default 1)");

DECLARE_OPTION(bool, arg_guess, false, "guess",
    "Guess opponent's secret color (instead of considering all possibilities)");

DECLARE_OPTION(std::string, arg_seed, "", "seed",
    "Random seed in hexadecimal format. If empty, pick randomly. "
    "The chosen seed will be logged to stderr for reproducibility.");

DECLARE_OPTION(int, arg_time_limit, LOCAL_BUILD ? 0 : 28, "time-limit",
    "Time limit in seconds (or 0 to disable time-based performance). "
    "On each turn, the player uses a fraction of time remaining on analysis. "
    "Note that this should be slightly lower than the official time limit to "
    "account for overhead.");

// A simple timer. Can be running or paused. Tracks time both while running and
// while paused. Use Elapsed() to query, Pause() and Resume() to switch states.
class Timer {
public:
  Timer(bool running = true) : running(running) {}

  bool Running() const { return running; }
  bool Paused() const { return !running; }

  // Returns how much time passed in the given state, in total.
  log_duration_t Elapsed(bool while_running = true) {
    clock_t::duration d = elapsed[while_running];
    if (running == while_running) d += clock_t::now() - start;
    return std::chrono::duration_cast<log_duration_t>(d);
  }

  log_duration_t Pause() {
    assert(Running());
    return TogglePause();
  }

  log_duration_t Resume() {
    assert(Paused());
    return TogglePause();
  }

  // Toggles running state, and returns how much time passed since last toggle.
  log_duration_t TogglePause() {
    auto end = clock_t::now();
    auto delta = end - start;
    elapsed[running] += delta;
    start = end;
    running = !running;
    return std::chrono::duration_cast<log_duration_t>(delta);
  }

private:
  using clock_t = std::chrono::steady_clock;

  bool running = false;
  clock_t::time_point start = clock_t::now();
  clock_t::duration elapsed[2] = {clock_t::duration{0}, clock_t::duration{0}};
};

std::string ReadInputLine() {
  std::string s;
  if (!std::getline(std::cin, s)) {
    LogError() << "Unexpected end of input!";
    exit(1);
  }
  LogReceived(s);
  if (s == "Quit") {
    LogInfo() << "Exiting.";
    exit(0);
  }
  return s;
}

int ReadSecretColor() {
  std::string s = ReadInputLine();
  std::optional<color_t> color;
  if (s.size() == 1 && (color = ParseColor(s[0]))) return *color;
  LogError() << "Could not parse secret color: " << s;
  exit(1);
}

tile_t ReadTile() {
  std::string s = ReadInputLine();
  if (std::optional<tile_t> tile = ParseTile(s)) return *tile;
  LogError() << "Could not parse tile: " << s;
  exit(1);
}

Move ReadMove() {
  std::string s = ReadInputLine();
  if (std::optional<Move> move = ParseMove(s)) return *move;
  LogError() << "Could not parse move: " << s;
  exit(1);
}

int Evaluate(int my_color, const grid_t &grid) {
  std::array<int, COLORS> scores = {};
  grid_t fixed = CalcFixed(grid);
  EvaluateAllColors(grid, fixed, scores);
  int my_score = scores[my_color - 1];
  int max_other_score = 0;
  for (int c = 1; c <= COLORS; ++c) {
    if (c != my_color && scores[c - 1] > max_other_score) {
      max_other_score = scores[c - 1];
    }
  }
  return my_score - max_other_score;
}

int Evaluate(int my_color, int his_color, const grid_t &grid) {
  std::array<int, COLORS> scores = {};
  grid_t fixed = CalcFixed(grid);
  EvaluateAllColors(grid, fixed, scores);
  // Sanity check. Delete this to make it slightly faster.
  assert(scores[my_color - 1] - scores[his_color - 1] == EvaluateTwoColors(grid, fixed, my_color, his_color));
  return scores[my_color - 1] - scores[his_color - 1];
}

// Generates the tiles that differ only in the position of the two given colors,
// with other colors in an arbitrary location. (This is a bit more complicated
// than it needs to be because I currently generate the lexicographical minimal
// tiles. Maybe I can simplify/optimize it later, if it matters.)
void GenerateRelevantTiles(int my_color, int his_color, std::array<tile_t, 6*5> &tiles) {
  size_t pos = 0;
  for (int i = 0; i < 6; ++i) {
    for (int j = 0; j < 6; ++j) {
      if (i == j) continue;
      int next_color = 1;
      while (next_color == my_color || next_color == his_color) ++next_color;
      tile_t &tile = tiles[pos++];
      for (int k = 0; k < 6; ++k) {
        if (k == i) {
          tile[k] = my_color;
        } else if (k == j) {
          tile[k] = his_color;
        } else {
          tile[k] = next_color++;
          while (next_color == my_color || next_color == his_color) ++next_color;
        }
      }
      assert(next_color == 7);
    }
  }
  assert(pos == 6*5);
}

int EvaluateSecondPly(int my_color, int his_color, const grid_t &grid) {
  std::vector<Placement> all_placements = GeneratePlacements(grid);
  if (all_placements.empty()) {
    // No more moves.
    grid_t fixed;
    for (int r = 0; r < HEIGHT; ++r) {
      for (int c = 0; c < WIDTH; ++c) {
        fixed[r][c] = 1;
      }
    }
    return 6 * 5 * EvaluateTwoColors(grid, fixed, my_color, his_color);
  }

  std::array<tile_t, 6*5> tiles;
  GenerateRelevantTiles(my_color, his_color, tiles);

  int total_score = 0;
  for (tile_t tile : tiles) {
    int best_score = std::numeric_limits<int>::max();
    for (Placement placement : all_placements) {
      grid_t copy = grid;
      ExecuteMove(copy, tile, placement);
      int score;
      if (false) {
        // Slow version.
        score = Evaluate(my_color, his_color, copy);
      } else {
        score = EvaluateTwoColors(copy, CalcFixed(copy), my_color, his_color);
      }
      best_score = std::min(best_score, score);
    }
    total_score += best_score;
  }
  return total_score;
}

Placement GreedyPlacement(int my_color, int his_color, const grid_t &grid, const tile_t &tile, rng_t &rng) {
  int best_score = std::numeric_limits<int>::min();
  std::vector<Placement> all_placements = GeneratePlacements(grid);
  std::vector<Placement> best_placements;
  for (Placement placement : all_placements) {
    grid_t copy = grid;
    ExecuteMove(copy, tile, placement);
    int score = std::numeric_limits<int>::max();
    if (arg_deep) {
      if (his_color == 0) {
        for (int c = 1; c <= 6; ++c) {
          if (c == my_color) continue;
          score = std::min(score, EvaluateSecondPly(my_color, c, copy));
        }
      } else {
        score = std::min(score, EvaluateSecondPly(my_color, his_color, copy));
      }
    } else {
      if (his_color == 0) {
        score = Evaluate(my_color, copy);
      } else {
        score = EvaluateTwoColors(copy, CalcFixed(copy), my_color, his_color);
      }
    }

    if (score > best_score) {
      best_placements.clear();
      best_score = score;
    }
    if (score == best_score) {
      best_placements.push_back(placement);
    }
  }
  LogMoveCount(all_placements.size(), best_placements.size(), best_score);
  return RandomSample(best_placements, rng);
}

void PlayGame(rng_t &rng) {
  Timer timer(false);

  // First line of input contains my secret color.
  const int my_secret_color = ReadSecretColor();

  // Second line of input contains the first tile placed in the center.
  Move start_move = ReadMove();
  grid_t grid = {};
  start_move.Execute(grid);

  // Third line of input contains either "Start" if I play first, or else the
  // first move played by the opponent.
  std::string input = ReadInputLine();
  const int my_player = (input == "Start" ? 0 : 1);

  SecretColorGuesser guesser;
  std::array<int, COLORS> last_scores;

  for (int turn = 0; !IsGameOver(grid); ++turn) {

    if (arg_guess) {
      std::array<int, COLORS> scores;
      EvaluateAllColors(grid, CalcFixed(grid), scores);
      if (turn > 0 && turn % 2 == my_player) {
        guesser.Update(last_scores, scores);
        LogGuess(guesser.Color());
      }
      last_scores = scores;
    }

    if (turn % 2 == my_player) {
      // My turn! Read input.
      tile_t tile = ReadTile();
      auto pause_duration = timer.Resume();
      LogPause(pause_duration, timer.Elapsed(false));

      // Calculate my move.
      int his_secret_color = turn > 0 && arg_guess ? guesser.Color() : 0;
      Placement placement = GreedyPlacement(my_secret_color, his_secret_color, grid, tile, rng);
      Move move = {tile, placement};
      assert(move.IsValid(grid));
      move.Execute(grid);

      // Write output.
      std::string output = FormatPlacement(placement);
      LogSending(output);
      // Pause the timer just before writing the output line, since the referee
      // may suspend our process immediately after.
      auto turn_duration = timer.Pause();
      LogTime(turn_duration, timer.Elapsed(true));
      std::cout << output << std::endl;
    } else {
      // Opponent's turn.
      if (turn > 0) input = ReadInputLine();
      std::optional<Move> move = ParseMove(input);
      if (!move) {
        LogError() << "Could not parse opponent's move: " << input;
        exit(1);
      } else if (!move->IsValid(grid)) {
        LogError() << "Opponent's move is invalid: " << input;
        exit(1);
      } else {
        move->Execute(grid);
      }
    }
  }
  LogInfo() << "Game over.";
}

bool InitializeSeed(rng_seed_t &seed, std::string_view hex_string) {
  if (hex_string.empty()) {
    // Generate a new random 128-bit seed
    seed = GenerateSeed(4);
    return true;
  }
  if (auto s = ParseSeed(hex_string)) {
    seed = *s;
    return true;
  } else {
    LogError() << "Could not parse RNG seed: [" << hex_string << "]";
    return false;
  }
}

} // namespace

int main(int argc, char *argv[]) {
  LogId('R', player_name);

  if (!ParseOptions(argc, argv) || arg_help) {
    std::ostream &os = arg_help ? std::cout : std::clog;
    os << "\nOptions:\n";
    PrintOptionUsage(os);
    return EXIT_FAILURE;
  }

  // Initialize RNG.
  rng_seed_t seed;
  if (!InitializeSeed(seed, arg_seed)) return EXIT_FAILURE;
  LogSeed(seed);
  rng_t rng = CreateRng(seed);

  PlayGame(rng);
}
