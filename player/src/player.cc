#include "analysis.h"
#include "first-move.h"
#include "logging.h"
#include "options.h"
#include "random.h"
#include "state.h"

#include <algorithm>
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

DECLARE_OPTION(bool, arg_deep, true, "deep",
    "Search deeper (2 ply instead of default 1)");

DECLARE_OPTION(bool, arg_guess, true, "guess",
    "Guess opponent's secret color (instead of considering all possibilities)");

DECLARE_OPTION(std::string, arg_seed, "", "seed",
    "Random seed in hexadecimal format. If empty, pick randomly. "
    "The chosen seed will be logged to stderr for reproducibility.");

DECLARE_OPTION(int, arg_time_limit, LOCAL_BUILD ? 0 : 25, "time-limit",
    "Time limit in seconds (or 0 to disable time-based performance). "
    "On each turn, the player uses a fraction of time remaining on analysis. "
    "Note that this should be slightly lower than the official time limit to "
    "account for overhead.");

DECLARE_OPTION(bool, arg_precompute_first_moves, false, "precompute-first-moves",
    "Precomputes first moves and outputs the resulting array.");

DECLARE_OPTION(bool, arg_first_move_table, true, "first-move-table",
    "Use the precomputed first move table.");

DECLARE_OPTION(int, arg_extra_ply, 0, "extra-ply",
    "Insert an extra search ply if remaining placements is strictly less than this value");

// A simple timer. Can be running or paused. Tracks time both while running and
// while paused. Use Elapsed() to query, Pause() and Resume() to switch states.
class Timer {
public:
  Timer(bool running = true) : running(running) {}

  bool Running() const { return running; }
  bool Paused() const { return !running; }

  // Returns how much time passed in the given state, in total.
  log_duration_t Elapsed(bool while_running = true) const {
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

// For debugging: count total number of squares generated in EvaluateTwoPly2().
//static int64_t total_square_count;

std::string ReadInputLine() {
  std::string s;
  if (!std::getline(std::cin, s)) {
    LogError() << "Unexpected end of input!";
    exit(1);
  }
  LogReceived(s);
  if (s == "Quit") {
    LogInfo() << "Exiting.";
    //LogInfo() << "total_square_count=" << total_square_count;
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

// Generates the tiles that differ only in the position of the two given colors,
// with other colors in an arbitrary location. (This is a bit more complicated
// than it needs to be because I currently generate the lexicographical minimal
// tiles. Maybe I can simplify/optimize it later, if it matters.)
void GenerateRelevantTiles(int my_color, int his_color, std::array<tile_t, 6*5> &tiles) {
  assert(
    0 < my_color && my_color <= COLORS &&
    0 < his_color && his_color <= COLORS &&
    my_color != his_color);
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

#if 0
int Evaluate(int my_color, int his_color, const grid_t &grid) {
  std::array<int, COLORS> scores = {};
  grid_t fixed = CalcFixed(grid);
  EvaluateAllColors(grid, fixed, scores);
  // Sanity check. Delete this to make it slightly faster.
  assert(scores[my_color - 1] - scores[his_color - 1] == EvaluateTwoColors(grid, fixed, my_color, his_color));
  return scores[my_color - 1] - scores[his_color - 1];
}

// Slower version of EvaluateSecondPly2().
int EvaluateSecondPly(int my_color, int his_color, const grid_t &grid) {
  std::vector<Placement> placements = GeneratePlacements(grid);
  if (placements.empty()) {
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
    for (Placement placement : placements) {
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
#endif

int EvaluateEndOfGame(int my_color, int his_color, const grid_t &original_input_grid) {
  // No more moves.
  grid_t fixed;
  for (int r = 0; r < HEIGHT; ++r) {
    for (int c = 0; c < WIDTH; ++c) {
      fixed[r][c] = 1;
    }
  }
  if (true) {
    // Just evaluate normally and multiply by the 6 * 5 weight that would
    // apply when considering all next possible placements.
    return 6 * 5 * EvaluateTwoColors(original_input_grid, fixed, my_color, his_color);
  } else {
    // We could evaluate by final score instead, since partial squares are
    // worthless at this point. However, empirically it doesn't seem to make
    // a significant difference, which makes sense because at this point all
    // squares are fixed anyway, and the total score is dominated by squares.
    std::array<int, COLORS> scores;
    EvaluateFinalScore(original_input_grid, scores);
    return 1000000*(scores[my_color] - scores[his_color]);
  }
}

// During the second ply, the opponent gets a random tile, then choses a
// placement. Since the tile is random, we can average the outcome over all
// possibilities (or equivalently, since the number of possible tiles is
// constant, calculate the sum, which is what we do below).
//
// Since the opponent wants us to lose, he will chose the placement that leads
// to a minimum score for us:
//
//                 state                |
//               /   |   \              |
//             /    avg    \            |
//           /       |        \         |
//      tile1      tile2       tile3    |
//       /|\        /|\         /|\     |
//      /min\      /min\       /min\    |
//     /  |  \    /  |  \     /  |  \   |
//    place1..N  place1..N   place1..N  |
//
// Note that the placements are the same for all tiles, so we can calculate
// the list of placements up front. For a given placement, we can also
// precalculate part of the score, since only the squares that partially overlap
// with the newly-placed square are affected by which square is drawn!
//
int EvaluateSecondPly2(int my_color, int his_color, const grid_t &original_input_grid) {
  std::vector<Placement> placements = GeneratePlacements(original_input_grid);
  if (placements.empty()) {
    return EvaluateEndOfGame(my_color, his_color, original_input_grid);
  }

  struct Square {
    int r1, c1, r2, c2;
  };

  struct ExtraData {
    Placement placement;
    grid_t fixed;
    int base_score;
    std::vector<Square> undecided_my_color;
    std::vector<Square> undecided_his_color;
  };

  std::vector<ExtraData> extra_data;
  extra_data.reserve(placements.size());

  const color_t placeholder_color = COLORS + 1;
  tile_t placeholder_tile;
  std::ranges::fill(placeholder_tile, placeholder_color);
  for (const Placement &placement : placements) {
    grid_t copy = original_input_grid;
    ExecuteMove(copy, placeholder_tile, placement);
    grid_t fixed = CalcFixed(copy);

    int base_score = 0;
    std::vector<Square> undecided_my_color;
    std::vector<Square> undecided_his_color;
    for (int r1 = 0; r1 < HEIGHT; ++r1) {
      for (int c1 = 0; c1 < WIDTH; ++c1) {
        if (copy[r1][c1] == my_color)  base_score += Evaluate1(fixed, r1, c1);
        if (copy[r1][c1] == his_color) base_score -= Evaluate1(fixed, r1, c1);
        for (int r2, c2, size = 1; (r2 = r1 + size) < HEIGHT && (c2 = c1 + size) < WIDTH; ++size) {
          if (copy[r1][c1] == placeholder_color ||
              copy[r1][c2] == placeholder_color ||
              copy[r2][c1] == placeholder_color ||
              copy[r2][c2] == placeholder_color) {
            // Square contains a placeholder. Leave it as undecided for now.
            if (copy[r1][c1] == placeholder_color &&
                copy[r2][c2] == placeholder_color) {
              // Special case: square covers placeholder tile entirely.
              // TODO: limit this to the central square of the tile only, which is the only one
              // that can contain two digits of the same color.
              undecided_my_color.push_back({r1, c1, r2, c2});
              undecided_his_color.push_back({r1, c1, r2, c2});
            } else {
              // Otherwise, only need to score this square if it already contains one point
              // of a player's color, and the other points are not fixed to something other
              // than my color/placeholder.
              if ((copy[r1][c1] == my_color ||
                   copy[r1][c2] == my_color ||
                   copy[r2][c1] == my_color ||
                   copy[r2][c2] == my_color) &&
                  (!fixed[r1][c1] || copy[r1][c1] == my_color || copy[r1][c1] == placeholder_color) &&
                  (!fixed[r1][c2] || copy[r1][c2] == my_color || copy[r1][c2] == placeholder_color) &&
                  (!fixed[r2][c1] || copy[r2][c1] == my_color || copy[r2][c1] == placeholder_color) &&
                  (!fixed[r2][c2] || copy[r2][c2] == my_color || copy[r2][c2] == placeholder_color)) {
                undecided_my_color.push_back({r1, c1, r2, c2});
              }
              if ((copy[r1][c1] == his_color ||
                   copy[r1][c2] == his_color ||
                   copy[r2][c1] == his_color ||
                   copy[r2][c2] == his_color) &&
                  (!fixed[r1][c1] || copy[r1][c1] == his_color || copy[r1][c1] == placeholder_color) &&
                  (!fixed[r1][c2] || copy[r1][c2] == his_color || copy[r1][c2] == placeholder_color) &&
                  (!fixed[r2][c1] || copy[r2][c1] == his_color || copy[r2][c1] == placeholder_color) &&
                  (!fixed[r2][c2] || copy[r2][c2] == his_color || copy[r2][c2] == placeholder_color)) {
                undecided_his_color.push_back({r1, c1, r2, c2});
              }
            }
          } else {
            // Square contains no placeholders, so we can score it in advance.
            base_score += EvaluateRectangle(copy, fixed, my_color,  r1, c1, r2, c2);
            base_score -= EvaluateRectangle(copy, fixed, his_color, r1, c1, r2, c2);
          }
        }
      }
    }
    //total_square_count += undecided_my_color.size();
    //total_square_count += undecided_his_color.size();
    extra_data.push_back({
      placement, fixed, base_score,
      std::move(undecided_my_color),
      std::move(undecided_his_color)});
  }
  assert(extra_data.size() == placements.size());

  std::array<tile_t, 6*5> tiles;
  GenerateRelevantTiles(my_color, his_color, tiles);

  int total_score = 0;
  for (tile_t tile : tiles) {
    int best_score = std::numeric_limits<int>::max();
    for (const ExtraData &extra : extra_data) {
      grid_t copy = original_input_grid;
      ExecuteMove(copy, tile, extra.placement);
      int score = extra.base_score;
      Rect tile_bounds = extra.placement.GetBounds();
      for (int r = tile_bounds.r1; r < tile_bounds.r2; ++r) {
        for (int c = tile_bounds.c1; c < tile_bounds.c2; ++c) {
          if (copy[r][c] == my_color)  score += Evaluate1(extra.fixed, r, c);
          if (copy[r][c] == his_color) score -= Evaluate1(extra.fixed, r, c);
        }
      }
      for (auto [r1, c1, r2, c2] : extra.undecided_my_color) {
        score += EvaluateRectangle(copy, extra.fixed, my_color,  r1, c1, r2, c2);
      }
      for (auto [r1, c1, r2, c2] : extra.undecided_his_color) {
        score -= EvaluateRectangle(copy, extra.fixed, his_color, r1, c1, r2, c2);
      }
      best_score = std::min(best_score, score);
    }
    total_score += best_score;
  }
  return total_score;
}

int EvaluateExtraPly(int my_color, int his_color, const grid_t &original_input_grid) {
  std::vector<Placement> placements = GeneratePlacements(original_input_grid);
  if (placements.empty()) {
    return EvaluateEndOfGame(my_color, his_color, original_input_grid);
  }

  std::array<tile_t, 6*5> tiles;
  GenerateRelevantTiles(my_color, his_color, tiles);
  int total_score = 0;
  for (tile_t tile : tiles) {
    int best_score = std::numeric_limits<int>::max();
    for (Placement placement : placements) {
      grid_t copy = original_input_grid;
      ExecuteMove(copy, tile, placement);
      int score = -EvaluateSecondPly2(his_color, my_color, copy);
      if (score < best_score) {
        best_score = score;
      }
    }
    total_score += best_score;
  }
  return total_score;
}

std::pair<std::vector<Placement>, int> FindBestPlacements(
    int my_color, int his_color, const grid_t &grid, const tile_t &tile,
    const std::vector<Placement> &all_placements, const Timer *timer) {
  int best_score = std::numeric_limits<int>::min();
  bool extra_ply = false;
  if (arg_extra_ply > 0 && (int) all_placements.size() < arg_extra_ply) {
    size_t p = all_placements.size();
    if (arg_time_limit == 0) {
      // No time limit set.
      extra_ply = true;
      LogExtraPly(p, extra_ply);
    } else {
      // Estimated time needed for the extra ply as p^4 / 50 milliseconds,
      // where p = all_placements.size().
      auto time_needed = std::chrono::milliseconds((int64_t) p * p * p * p / 50);
      using namespace std::chrono;
      auto time_left = std::chrono::seconds(arg_time_limit) - timer->Elapsed();
      extra_ply = time_needed < time_left;
      LogExtraPly(p, extra_ply, time_needed, time_left);
    }
  }
  std::vector<Placement> best_placements;
  for (Placement placement : all_placements) {
    grid_t copy = grid;
    ExecuteMove(copy, tile, placement);
    int score = std::numeric_limits<int>::max();
    if (extra_ply) {
      assert(his_color);
      score = EvaluateExtraPly(my_color, his_color, copy);
    } else if (arg_deep) {
      if (his_color == 0) {
        for (int c = 1; c <= 6; ++c) {
          if (c == my_color) continue;
          int s = EvaluateSecondPly2(my_color, c, copy);
          // int t = EvaluateSecondPly(my_color, c, copy);
          // std::cerr << s << ' ' << t << '\n';
          // assert(s == t);
          score = std::min(score, s);
        }
      } else {
        score = EvaluateSecondPly2(my_color, his_color, copy);
        // int tmp = EvaluateSecondPly(my_color, his_color, copy);
        // std::cerr << score << ' ' << tmp << '\n';
        // assert(score == tmp);
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
  return {std::move(best_placements), best_score};
}

void PlayGame(rng_t &rng) {
  Timer timer(false);

  // First line of input contains my secret color.
  const int my_secret_color = ReadSecretColor();

  // Second line of input contains the first tile placed in the center.
  Move start_move = ReadMove();
  assert(start_move.placement == initial_placement);
  grid_t grid = {};
  start_move.Execute(grid);

  // Third line of input contains either "Start" if I play first, or else the
  // first move played by the opponent.
  std::string input = ReadInputLine();
  const int my_player = (input == "Start" ? 0 : 1);

  SecretColorGuesser guesser;
  std::array<int, COLORS> last_scores;
  int his_secret_color = 0;

  for (int turn = 0; !IsGameOver(grid); ++turn) {

    if (arg_guess) {
      std::array<int, COLORS> scores;
      EvaluateAllColors(grid, CalcFixed(grid), scores);
      if (turn > 0 && turn % 2 == my_player) {
        guesser.Update(last_scores, scores);
        his_secret_color = guesser.Color(my_secret_color);
        LogGuess(his_secret_color);
      }
      last_scores = scores;
    }

    if (turn % 2 == my_player) {
      // My turn! Read input.
      tile_t tile = ReadTile();
      auto pause_duration = timer.Resume();
      LogPause(pause_duration, timer.Elapsed(false));

      // Calculate my move.
      std::vector<Placement> best_placements;
      if (turn == 0 && arg_first_move_table) {
        best_placements = FindBestFirstMoves(my_secret_color, start_move, tile);
        // Note: this is only expected to pass if the table was generated with
        // the exact same options:
        //assert(best_placements == FindBestPlacements(my_secret_color, 0, grid, tile, GeneratePlacements(grid)).first);
      } else {
        std::vector<Placement> all_placements = GeneratePlacements(grid);
        auto res = FindBestPlacements(my_secret_color, his_secret_color, grid, tile, all_placements, &timer);
        best_placements = res.first;
        int best_score = res.second;
        LogMoveCount(all_placements.size(), best_placements.size(), best_score);
      }
      Move move = {tile, RandomSample(best_placements, rng)};
      assert(move.IsValid(grid));
      move.Execute(grid);

      // Write output.
      std::string output = FormatPlacement(move.placement);
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
  //LogInfo() << "total_square_count=" << total_square_count;
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

  if (arg_extra_ply > 0 && !(arg_deep && arg_guess)) {
    std::cerr << "--extra-ply requires --deep and --guess\n";
    return EXIT_FAILURE;
  }

  InitializeAnalysis();

  if (arg_precompute_first_moves) {
    PrintBestFirstMoves(std::cout, CalculateBestFirstMoves(
      [](int color, const grid_t &grid, const tile_t &tile,
          const std::vector<Placement> &all_placements) {
        return FindBestPlacements(color, 0, grid, tile, all_placements, nullptr).first;
      }
    ));
    return 0;
  }

  // Initialize RNG.
  rng_seed_t seed;
  if (!InitializeSeed(seed, arg_seed)) return EXIT_FAILURE;
  LogSeed(seed);
  rng_t rng = CreateRng(seed);

  PlayGame(rng);
}
