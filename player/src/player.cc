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

const std::string player_name = "TODO";

DECLARE_OPTION(bool, arg_help, false, "help",
    "show usage information");

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

std::optional<color_t> ParseColor(char ch) {
  int color = ch - '0';
  if (color < 1 || color > 6) return std::nullopt;
  return color;
}

std::optional<coord_t> ParseRow(char ch) {
  int row = ch - 'A';
  if (row < 0 || row >= HEIGHT) return std::nullopt;
  return row;
}

std::optional<coord_t> ParseCol(char ch) {
  int col = ch - 'a';
  if (col < 0 || col >= WIDTH) return std::nullopt;
  return col;
}

std::string FormatPlacement(Placement placement) {
  std::string s;
  s.push_back('A' + placement.row);
  s.push_back('a' + placement.col);
  s.push_back(placement.ori == Orientation::HORIZONTAL ? 'h' : 'v');
  return s;
}

std::optional<Orientation> ParseOrientation(char ch) {
  switch (ch) {
    case 'h': return Orientation::HORIZONTAL;
    case 'v': return Orientation::VERTICAL;
    default: return std::nullopt;
  }
}

std::optional<tile_t> ParseTile(std::string_view s) {
  if (s.size() != COLORS) return std::nullopt;
  tile_t tile = {};
  for (int i = 0; i < COLORS; ++i) {
    std::optional<color_t> color = ParseColor(s[i]);
    if (!color) return std::nullopt;
    if (std::find(&tile[0], &tile[i], *color) != &tile[i]) return std::nullopt;
    tile[i] = *color;
  }
  return tile;
}

std::optional<Move> ParseMove(std::string_view s) {
  if (s.size() != COLORS + 3) return std::nullopt;
  std::optional<coord_t> row = ParseRow(s[0]);
  std::optional<coord_t> col = ParseCol(s[1]);
  std::optional<tile_t> tile = ParseTile(s.substr(2, COLORS));
  std::optional<Orientation> ori = ParseOrientation(s.back());
  if (!row || !col || !tile || !ori) return std::nullopt;
  Placement placement = {*row, *col, *ori};
  if (!placement.IsInBounds()) return {};
  return Move{*tile, placement};
}

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

void WriteOutputLine(std::string_view s) {
  LogSending(s);
  std::cout << s << std::endl;
}

std::vector<Placement> GeneratePlacements(const grid_t &grid) {
  std::vector<Placement> placements;
  for (coord_t row = 0; row < HEIGHT; ++row) {
    for (coord_t col = 0; col < WIDTH; ++col) {
      for (Orientation ori : ORIENTATIONS) {
        Placement placement = {row, col, ori};
        if (placement.IsValid(grid)) {
          placements.push_back(placement);
        }
      }
    }
  }
  return placements;
}

void EvaluateAllColors(const grid_t &grid, const grid_t &fixed, std::array<int, COLORS> &scores) {
  scores = {};
  for (int r1 = 0; r1 < HEIGHT; ++r1) {
    for (int c1 = 0; c1 < WIDTH; ++c1) {
      int color = grid[r1][c1];
      if (color == 0) continue;
      int score = 1;
      //  a  b
      //  c  d
      bool fa = fixed[r1][c1];
      for (int r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
        int size = r2 - r1;
        bool b = grid[r1][c2] == color;
        bool c = grid[r2][c1] == color;
        bool d = grid[r2][c2] == color;
        bool fb = fixed[r1][c2];
        bool fc = fixed[r2][c1];
        bool fd = fixed[r2][c2];
        int num_fixed = fa + fb + fc + fd;
        if (b && c && d) {
          // Square!
          score += 1000 + 100*num_fixed + 100*size;
        } else if (
              (b && c && !fd) ||
              (b && d && !fc) ||
              (c && d && !fb)) {
          // One cell short of a square.
          score += 100 + 10*num_fixed + 10*size;
        } else if (
            (b && !fc && !fd) ||
            (c && !fb && !fd) ||
            (d && !fb && !fc)) {
          // Two points aligned horizontally, vertically, or diagonally.
          // Maybe: assign a different score for the diagonal version?
          score += 10 + 1*num_fixed + 1*size;
        }
      }
      scores[color - 1] += score;
    }
  }
}

Placement GreedyPlacement(int my_color, const grid_t &grid, const tile_t &tile, rng_t &rng) {
  int best_score = std::numeric_limits<int>::min();
  std::vector<Placement> all_placements = GeneratePlacements(grid);
  std::vector<Placement> best_placements;
  for (Placement placement : all_placements) {
    grid_t copy = grid;
    ExecuteMove(copy, tile, placement);
    std::array<int, COLORS> scores = {};
    grid_t fixed = CalcFixed(copy);
    EvaluateAllColors(copy, fixed, scores);

    int my_score = scores[my_color - 1];
    int max_other_score = 0;
    for (int c = 1; c <= COLORS; ++c) {
      if (c != my_color && scores[c - 1] > max_other_score) {
        max_other_score = scores[c - 1];
      }
    }

    int score = my_score - max_other_score;

    if (score > best_score) {
      best_placements.clear();
      best_score = score;
    }
    if (score == best_score) {
      best_placements.push_back(placement);
    }
  }
  LogMoveCount(all_placements.size(), best_placements.size());
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

  for (int turn = 0; !IsGameOver(grid); ++turn) {
    if (turn % 2 == my_player) {
      // My turn!
      tile_t tile = ReadTile();

      auto pause_duration = timer.Resume();
      LogPause(pause_duration, timer.Elapsed(false));

      Placement placement = GreedyPlacement(my_secret_color, grid, tile, rng);
      Move move = {tile, placement};
      assert(move.IsValid(grid));
      move.Execute(grid);

      // Note: we should pause the timer just before writing the output line,
      // since the referee may suspend our process immediately after.
      LogTime(timer.Pause());
      WriteOutputLine(FormatPlacement(placement));  // TODO
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
  LogId(player_name);

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
