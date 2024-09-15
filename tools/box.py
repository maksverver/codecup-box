from enum import Enum
import random

# Just to make sure there is absolutely no detectable pattern in the random
# numbers, use SystemRandom to get high-quality entropy instead of a PRNG.
rng = random.SystemRandom()

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
    rng.shuffle(tile)
    return tuple(tile)


def MakeRandomSecretColors():
    return rng.sample(range(1, COLORS + 1), k=2)


def IsHorizontal(dir):
    if dir == Direction.VERTICAL: return False
    if dir == Direction.HORIZONTAL: return True
    raise ValueError("dir must be HORIZONTAL or VERTICAL")


DEFAULT_START_PLACEMENT = (7, 7, Direction.HORIZONTAL)

class BoardState:

    def __init__(self, start_tile, start_placement=DEFAULT_START_PLACEMENT):
        self.grid = [[0]*WIDTH for _ in range(HEIGHT)]
        self._remaining_moves = BoardState._GenerateMoves()
        self._current_move = next(self._remaining_moves)
        self.Place(start_tile, start_placement)

    def CanPlace(self, placement):
        grid = self.grid
        r1, c1, dir = placement
        r2 = r1 + (2 if IsHorizontal(dir) else COLORS)
        c2 = c1 + (COLORS if IsHorizontal(dir) else 2)
        if r1 < 0 or c1 < 0 or r2 > HEIGHT or c2 > WIDTH: return False
        overlap = BoardState._CountOverlap(grid, r1, c1, r2, c2)
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

    def CalculateScores(self, secret_colors):
        return [BoardState._CalculateScore(self.grid, c) for c in secret_colors]

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
            overlap = BoardState._CountOverlap(self.grid, r1, c1, r2, c2)
            if overlap <= 4:
                return True

            try:
                self._current_move = next(self._remaining_moves)
            except StopIteration:
                self._current_move = None
                return False

    @staticmethod
    def _CalculateScore(grid, color):
        score = 0
        for r1 in range(HEIGHT):
            for c1 in range(WIDTH):
                if grid[r1][c1] == color:
                    r2 = r1 + 1
                    c2 = c1 + 1
                    while r2 < HEIGHT and c2 < WIDTH:
                        if color == grid[r1][c2] == grid[r2][c1] == grid[r2][c2]:
                            score += r2 - r1
                        r2 += 1
                        c2 += 1
        return score
