#!/usr/bin/env python3

# Arbiter for the 2025 CodeCup game Box.

import argparse
from contextlib import nullcontext
import concurrent.futures
from collections import Counter, defaultdict
from datetime import datetime
from enum import Enum
import functools
import os.path
import random
import subprocess
from subprocess import DEVNULL, PIPE
import shlex
import sys
import time

HEIGHT=16
WIDTH=20
COLORS=6

ROW_COORDS = ''.join(chr(ord('A') + r) for r in range(HEIGHT))
COL_COORDS = ''.join(chr(ord('a') + c) for c in range(WIDTH))

class Direction(Enum):
  HORIZONTAL = 1
  VERTICAL = 2

DIR_TO_CH = {
  Direction.HORIZONTAL: 'h',
  Direction.VERTICAL:   'v',
}

CH_TO_DIR = {
  'h': Direction.HORIZONTAL,
  'v': Direction.VERTICAL,
}

def ParseTile(s):
  digits = []
  for ch in s:
    if not ch.isdigit(): return None
    digit = int(ch)
    if digit < 1 or digit > COLORS: return None
    digits.append(digit)
  if len(digits) != len(set(digits)): return None
  return tuple(digits)

def FormatTile(tile):
  return ''.join(map(str, tile))

def ParsePlacement(s):
  if len(s) != 3: return None
  r = ROW_COORDS.find(s[0])
  c = COL_COORDS.find(s[1])
  if r < 0 or c < 0: return None
  dir = CH_TO_DIR.get(s[2], None)
  if dir is None: return None
  return (r, c, dir)

def ParseTilePlacement(s):
  if len(s) < 3: return None
  placement = ParsePlacement(s[:2] + s[-1])
  if placement is None: return None
  tile = ParseTile(s[2:-1])
  if tile is None: return None
  return (tile, placement)

def FormatTilePlacement(tile, placement):
  r, c, dir = placement
  return ROW_COORDS[r] + COL_COORDS[c] + FormatTile(tile) + DIR_TO_CH[dir]


def test_ParseTile():
  assert ParseTile("123654") == (1, 2, 3, 6, 5, 4)

def test_ParsePlacement():
  assert ParsePlacement("Hiv") == (7, 8, Direction.VERTICAL)
  assert ParsePlacement("Poh") == (15, 14, Direction.HORIZONTAL)

def test_ParseTilePlacement():
  assert ParseTilePlacement("Fc326451h") == ((3, 2, 6, 4, 5, 1), (5, 2, Direction.HORIZONTAL))


def MakeRandomTile():
  tile = list(range(1, COLORS + 1))
  random.shuffle(tile)
  return tuple(tile)


def IsHorizontal(dir):
  if dir == Direction.VERTICAL: return False
  if dir == Direction.HORIZONTAL: return True
  raise ValueError("dir must be HORIZONTAL or VERTICAL")


class GameState:

  def __init__(self):
    self.grid = [[0]*WIDTH for _ in range(HEIGHT)]
    self.secret_colors = random.sample(range(1, COLORS + 1), k=2)
    self.start_tile = MakeRandomTile()
    self.start_placement = (7, 7, Direction.HORIZONTAL)
    self._remaining_moves = GameState._GenerateMoves()
    self._current_move = next(self._remaining_moves)
    self.Place(self.start_tile, self.start_placement)

  def CanPlace(self, placement):
    grid = self.grid
    r1, c1, dir = placement
    r2 = r1 + (2 if IsHorizontal(dir) else COLORS)
    c2 = c1 + (COLORS if IsHorizontal(dir) else 2)
    if r1 < 0 or c1 < 0 or r2 > HEIGHT or c2 > WIDTH: return False
    overlap = GameState._CountOverlap(grid, r1, c1, r2, c2)
    if overlap > 4: return False
    if overlap > 0: return True
    # Must touch at an edge:
    return (
        (c1 > 0 and any(grid[r][c1 - 1] for r in range(r1, r2))) or
        (c2 < WIDTH and any(grid[r][c2] for r in range(r1, r2))) or
        (r1 > 0 and any(grid[r1 - 1][c] for c in range(c1, c2))) or
        (r2 < HEIGHT and any(grid[r2][c] for c in range(c1, c2))))

  # Place a tile on the grid, assuming the placement is valid.
  #
  # This method can be shown to have O(COLORS) amortized time complexity,
  # including the call to _UpdateCurrentMove().
  def Place(self, tile, placement):
    self._Overwrite(self.grid, tile, placement)
    self._UpdateCurrentMove()

  def IsGameOver(self):
    return self._current_move is None

  @staticmethod
  def _CountOverlap(grid, r1, c1, r2, c2):
    return sum(grid[r][c] != 0 for r in range(r1, r2) for c in range(c1, c2))

  @staticmethod
  def _Overwrite(grid, tile, placement):
    r, c, dir = placement
    if IsHorizontal(dir):
      for i, v in enumerate(tile):
        grid[r][c + i] = grid[r + 1][c + len(tile) - 1 - i] = v
    else:
      for i, v in enumerate(tile):
        grid[r + i][c + 1] = grid[r + len(tile) - 1 - i][c] = v

  @staticmethod
  def _GenerateMoves():
    for r1 in range(HEIGHT):
      for c1 in range(WIDTH):
        for dir in Direction:
          r2 = r1 + (2 if IsHorizontal(dir) else COLORS)
          c2 = c1 + (COLORS if IsHorizontal(dir) else 2)
          if r2 <= HEIGHT and c2 <= WIDTH:
            yield r1, c1, r2, c2

  def _UpdateCurrentMove(self):
    while self._current_move is not None:
      r1, c1, r2, c2 = self._current_move
      overlap = GameState._CountOverlap(self.grid, r1, c1, r2, c2)
      if overlap <= 4:
        return True

      try:
        self._current_move = next(self._remaining_moves)
      except StopIteration:
        self._current_move = None
        return False


class Outcome(Enum):
  WIN  = 1
  LOSS = 2
  TIE  = 3
  FAIL = 4  # crash, invalid move, etc.


def Launch(command, logfile):
  if logfile is None:
    stderr = DEVNULL
  else:
    stderr = open(logfile, 'wt')
  return subprocess.Popen(command, shell=True, text=True, stdin=PIPE, stdout=PIPE, stderr=stderr)


def RunGame(command1, command2, transcript, logfile1, logfile2):
  roles = ['First', 'Second']
  commands = [command1, command2]
  procs = [Launch(command1, logfile1), Launch(command2, logfile2)]
  times = [0.0, 0.0]

  state = GameState()
  turn = 0
  last_tile = None
  last_placement = None

  with (open(transcript, 'wt') if transcript is not None else nullcontext()) as transcript:

    if transcript:
      print(*state.secret_colors, file=transcript)
      print(FormatTilePlacement(state.start_tile, state.start_placement), file=transcript)

    while not state.IsGameOver():
      proc = procs[turn % 2]

      # Give player input
      try:
        if turn < 2:
          proc.stdin.write(str(state.secret_colors[turn % 2]) + '\n')
          proc.stdin.write(FormatTilePlacement(state.start_tile, state.start_placement) + '\n')
        if turn == 0:
          proc.stdin.write('Start\n')
        else:
          proc.stdin.write(FormatTilePlacement(last_tile, last_placement) + '\n')
        last_tile = MakeRandomTile()
        proc.stdin.write(FormatTile(last_tile) + '\n')
        proc.stdin.flush()
      except BrokenPipeError:
        break

      # Read player's move
      start_time = time.monotonic()
      line = proc.stdout.readline().strip()
      times[turn % 2] += time.monotonic() - start_time

      # Parse, validate move and execute move
      last_placement = ParsePlacement(line)
      if not last_placement or not state.CanPlace(last_placement):
        if transcript:
          # Include invalid move in transcript (for debugging)
          print('# ' + line, file=transcript)
        break

      state.Place(last_tile, last_placement)
      turn += 1

      if transcript:
        print(FormatTilePlacement(last_tile, last_placement), file=transcript)

  outcomes = [None, None]
  scores = [None, None]
  if state.IsGameOver():
    # Game ended regularly. Calculate scores.
    outcomes[0] = outcomes[1] = Outcome.TIE
    #
    scores = [150, 150]  # TODO
  else:
    # Game ended irregularly, with the current player failing.
    outcomes[turn % 2] = Outcome.FAIL
    outcomes[1 - turn % 2] = Outcome.WIN
    # Assign a token score to the winner (note: this is different from the
    # CodeCup rules where the game is finished with the arbiter playing randomly)
    scores[turn % 2] = 0
    scores[1 - turn % 2] = 200

  # Gracefully quit. (Do this after computing outcomes because a program can
  # still fail if it doesn't exit cleanly, overriding the previous outcome.)
  for p in procs:
    if p.poll() is None:
      try:
        p.stdin.write('Quit\n')
        p.stdin.flush()
      except BrokenPipeError:
        pass

  for i, p in enumerate(procs):
    status = p.wait()
    if status != 0:
      print('%s command exited with nonzero status %d: %s' % (roles[i], status, commands[i]), file=sys.stderr)
      outcomes[i] = Outcome.FAIL

  return outcomes, scores, times


def MakeLogfilenames(logdir, name1, name2, game_index, game_count):
  if not logdir:
    return (None, None, None)
  prefix = '%s-vs-%s' % (name1, name2)
  if game_count > 1:
    prefix = 'game-%s-of-%s-%s' % (game_index + 1, game_count, prefix)
  return (os.path.join(logdir, '%s-%s.txt' % (prefix, role))
      for role in ('transcript', 'first', 'second'))


class Tee:
  '''Writes output to a file, and also to stdout.'''

  def __init__(self, file):
    self.file = file

  def __enter__(self):
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    self.file.close()

  def close(self, s):
    self.file.close()

  def write(self, s):
    sys.stdout.write(s)
    return self.file.write(s)


def RunGames(commands, names, rounds, logdir, executor=None):
  P = len(commands)

  if rounds == 0:
    # Play only one match between ever pair of players, regardless of order
    pairings = [(i, j) for i in range(P) for j in range(i + 1, P)]
  else:
    # Play one match per round between every pair of players and each order
    pairings = [(i, j) for i in range(P) for j in range(P) if i != j] * rounds

  games_per_player = sum((i == 0) + (j == 0) for (i, j) in pairings)

  # Run all of the games, keeping track of total times and outcomes per player.
  player_time_total = [0.0]*P
  player_time_max   = [0.0]*P
  player_outcomes   = [defaultdict(lambda: 0) for _ in range(P)]
  player_scores     = [0]*P

  def AddStatistics(i, j, outcomes, scores, times):
    player_time_total[i] += times[0]
    player_time_total[j] += times[1]
    player_time_max[i] = max(player_time_max[i], times[0])
    player_time_max[j] = max(player_time_max[j], times[1])
    player_outcomes[i][outcomes[0]] += 1
    player_outcomes[j][outcomes[1]] += 1
    player_scores[i] += scores[0]
    player_scores[j] += scores[1]

  futures = []

  symlink_playerlogs = logdir and len(pairings) > 1

  if logdir:
    print('Writing logs to directory', logdir)
    print()

  if symlink_playerlogs:
    playerlog_dirs = []
    for name in names:
      dirname = os.path.join(logdir, name + '-logs')
      os.mkdir(dirname)
      playerlog_dirs.append(dirname)

  with (Tee(open(os.path.join(logdir, 'results.txt'), 'wt'))
        if logdir and len(pairings) > 1 else nullcontext()) as f:

    print('Game Player 1           Player 2           Outc1 Outc2 Pts1 Pts2 Time 1 Time 2', file=f)
    print('---- ------------------ ------------------ ----- ----- ---- ---- ------ ------', file=f)

    def PrintRowStart(game_index, name1, name2):
      print('%4d %-18s %-18s ' % (game_index + 1, name1, name2), end='', file=f)
    def PrintRowFinish(outcomes, scores, times):
      print('%-5s %-5s %4d %4d %6.2f %6.2f' % (outcomes[0].name, outcomes[1].name, scores[0], scores[1], times[0], times[1]), file=f)

    for game_index in range(len(pairings)):
      i, j = pairings[game_index]
      command1, command2 = commands[i], commands[j]
      name1, name2 = names[i], names[j]
      transcript, logfilename1, logfilename2 = MakeLogfilenames(logdir, name1, name2, game_index, len(pairings))

      if symlink_playerlogs:
        os.symlink(os.path.relpath(logfilename1, playerlog_dirs[i]), os.path.join(playerlog_dirs[i], os.path.basename(logfilename1)))
        os.symlink(os.path.relpath(logfilename2, playerlog_dirs[j]), os.path.join(playerlog_dirs[j], os.path.basename(logfilename2)))

      run = functools.partial(RunGame, command1, command2, transcript, logfilename1, logfilename2)

      if executor is None:
        # Run game on the main thread.
        # Print start of row first and flush, so the user can see which players
        # are competing while the game is in progress.
        PrintRowStart(game_index, name1, name2)
        sys.stdout.flush()
        outcomes, scores, times = run()
        PrintRowFinish(outcomes, scores, times)
        AddStatistics(i, j, outcomes, scores, times)
      else:
        # Start the game on a background thread.
        future = executor.submit(run)
        futures.append(future)

    # Print results from asynchronous tasks in the order they were started.
    #
    # This means that earlier tasks can block printing of results for later tasks
    # have have already finished, but this is better than printing the results out
    # of order.
    for game_index, future in enumerate(futures):
      i, j = pairings[game_index]
      outcomes, scores, times = future.result()
      AddStatistics(i, j, outcomes, scores, times)
      PrintRowStart(game_index, names[i], names[j])
      PrintRowFinish(outcomes, scores, times)

    print('---- ------------------ ------------------ ----- ----- ---- ---- ------ ------', file=f)

  # Print summary of players.
  if len(pairings) > 1:
    print()
    with (Tee(open(os.path.join(logdir, 'summary.txt'), 'wt')) if logdir else nullcontext()) as f:
      print('Player             Avg.Tm Max.Tm Tot. Wins Ties Loss Fail Points', file=f)
      print('------------------ ------ ------ ---- ---- ---- ---- ---- ------', file=f)
      for p in sorted(range(P), key=lambda p: player_outcomes[p][Outcome.WIN], reverse=True):
        print('%-18s %6.2f %6.2f %4d %4d %4d %4d %4d %6d' %
          ( names[p],
            player_time_total[p] / games_per_player,
            player_time_max[p],
            games_per_player,
            player_outcomes[p][Outcome.WIN],
            player_outcomes[p][Outcome.TIE],
            player_outcomes[p][Outcome.LOSS],
            player_outcomes[p][Outcome.FAIL],
            player_scores[p],
          ), file=f)
      print('------------------ ------ ------ ---- ---- ---- ---- ---- ------', file=f)

  if logdir:
    print()
    print('Logs written to directory', logdir)


def DeduplicateNames(names):
  unique_names = []
  name_count = Counter(names)
  name_index = defaultdict(lambda: 0)
  for name in names:
    if name_count[name] == 1:
      unique_names.append(name)
    else:
      name_index[name] += 1
      unique_names.append('%s-%d' % (name, name_index[name]))
  return unique_names


def MakeLogdir(logdir, rounds):
  if logdir is None:
    return None

  assert os.path.isdir(logdir)
  if rounds > 0:
    # Create a subdirectory based on current date & time
    dirname = datetime.now().strftime('%Y%m%dT%H%M%S')
    logdir = os.path.join(logdir, dirname)
    os.mkdir(logdir)  # fails if path already exists
  return logdir


def Main():
  parser = argparse.ArgumentParser(prog='arbiter.py')
  parser.add_argument('command1', type=str,
      help='command to execute for player 1')
  parser.add_argument('command2', type=str,
      help='command to execute for player 2')
  parser.add_argument('commandN', type=str, nargs='*',
      help='command to execute for player N')
  parser.add_argument('--rounds', type=int, default=0,
      help='number of full rounds to play (or 0 for a single game)')
  parser.add_argument('--logdir', type=str, default=None,
      help='directory where to write player logs')
  parser.add_argument('-t', '--threads', type=int, default=0,
      help='parallelize execution using threads (no more than physical cores!)')

  args = parser.parse_args()
  logdir = MakeLogdir(args.logdir, args.rounds)

  commands = [args.command1, args.command2] + args.commandN

  names = DeduplicateNames([os.path.basename(shlex.split(command)[0]) for command in commands])

  with (concurrent.futures.ThreadPoolExecutor(max_workers=args.threads)
        if args.threads > 0 else nullcontext()) as executor:
    RunGames(commands, names, rounds=args.rounds, logdir=logdir, executor=executor)

if __name__ == '__main__':
  Main()
