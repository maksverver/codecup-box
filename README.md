Box player for the 2025 CodeCup competition
===========================================

See: https://codecup.nl/ (later: https://archive.codecup.nl/2025/)

I got 4th place in the final competition, with very small difference between
top players, so I'm pretty proud. (I also won the last few test competitions
by a large margin.)


### Player algorithm

The player essentially uses a fixed three-ply minimax algorithm:

 1. Place my tile (try all available placements)
 2. Draw a random tile (try all possible tiles)
 3. Place my opponent's tile (try all available placements)
 4. Evaluate position

This creates a search tree with four layers. On the fourth layer I evaluate
how good the position is for me (see below for details). On the third layer,
I take the minimum value across all possible placements, since the opponent
would try to minimize my outcome. On layer two, I take the average (or
equivalently the sum) across all tiles, because each tile is equally likely.
On the top layer, I take the maximum, since I obviously want to maximize my
outcome.

This seemed to work extremely well in the final test competitions, and was
competitive with the best players (which were based on Monte Carlo methods).

The algorithm as described above would be too slow, because of the large
number of placements (100-200) and tiles (720). I applied three optimizations
that together made the player (usually) stay within the time limit:

 1. guessing the opponent's color
 2. optimizing the evaluation function
 3. precalculating first moves


### Guessing the opponent's color

Outside the search algorithm, I try to guess my opponent's secret color. This
is done by running the same evaluation function as in the minimax search, once
before and once after the player made his move, and tracking which color's
scores go up the most across the game.

This very simple method turns out to be extremely reliable in practice; I was
able to guess most player's colors after just a few turns. It's possible to
verify this using the game logs which are posted after the competitions (see
[tools/color-guess.sh](tools/color-guess.sh)).

The reason this works so well is that due to the nature of the game it's hard to
mislead the opponent about your secret color without sacrificing your own score,
so all good players just try to maximize their score, which makes it easy to
guess what their secret color is. Ironically, it's bad players (especially
completely random ones) that are hard to pin down, but those are pretty easy to
defeat anyway.

Guessing the opponent's color is important because it allows me to reduce the
number of random tiles I need to consider at step 2 in the player algorithm,
because all that matters is the position of my and my opponent's color. That
reduces the options from 6! = 720 to 6×5 = 30, a factor 24 speedup.


### Evaluation function

The evaluation function scores each color separately. The final score used at
step 4 simply the difference between the score for the player's color and the
opponent's (guessed) color.

For a given color, we consider each square on the board independently, and count
how many corners are already occupied by that color. The more corners are
already occupied, the better, because the more likely it is that we will be able
to complete a full square.

In addition to that, we consider which grid cells can still be overwritten by
future moves, and which cannot. The cells that cannot be overwritten, because
all possible placements that overlap that grid cell would overlap too many
existing cells, are considered *fixed*, and that affects the score in two
ways:

 1. We completely disregard incomplete squares where one of the missing corners
    is fixed, because it would be impossible to complete it.

 2. For other squares, we give an extra high score to corners of the correct
    color that are also fixed, because the opponent will not be able to replace
    them.

Phrased differently, each corner of a square is in one of four states:

  1. My color and fixed (great!)
  2. My color and not fixed (still good)
  3. Any other color and not fixed (not great, but we can still win)
  4. Any other color and fixed (unwinnable; discard this square)

Note that "other color" can also mean an empty cell here.

I tried to experiment with different weightings of the different types of
squares, and factoring in the sizes of squares, but none of it seemed to have a
measurable impact on playing strength.


### Optimizing the evaluation function

An early version of my player was based on a one-ply search, but it wasn't very
strong. I had the three-ply structure (counting drawing the random tile as a
separate ply) in mind for a while, but it was too slow, until I figured out how
to optimize it.

As a reminder, the search routine consists essentially of three nested loops,
corresponding with the three levels of the search three:

```
Search(my_tile):
    max_value = -oo
    for my_place of all free places:
        Place(my_tile, my_place)
        avg_value = 0
        for his_tile of all possible tiles:
            min_value = oo
            for his_place of all free places:
                Place(his_tile, his_place)
                leaf_value = Evaluate()
                min_value = min(min_value, leaf_value)
            avg_value = avg_value + min_value / number of tiles
        max_value = max(max_value, avg_value)
    return max_value
```

The evaluation function normally goes over all possible squares, but this is
wasteful because most of the state is fixed, and the newly-placed tile can only
change the value of a few of the existing squares, namely those that have
at least one corner that overlaps the newly placed tile.

To avoid doing duplicate work in the innermost loop, I calculate a preliminary
score plus a list of undecided squares for each (my_place, his_place) pair.
Then instead of doing a full evaluation, I can just take the preliminary
score and update it by evaluating the undecided squares using the actual colors
from his_tile.

Note that the innermost loop runs for each (my_place, his_tile, his_place)
triple, of which there are 30 times as many as (my_place, his_place) pairs, so
optimizing evaluation there can save a lot of time, even taking into account
the precalculation cost.

See `EvaluateSecondPly2()` in [player.cc](player/src/player.cc) for
implementation details.


### First move tables

To squeeze out the final bit of performance, I precalculated the optimal opening
move for each combination of (opening tile, my random tile, my secret color).
I normalize the opening tiles to 123456 by permuting the colors, so there are
only 720 × 6 = 4320 different positions to consider.

For each position, not just one but all best moves are stored, so the player
can select a move at random, though in practice there are rarely more than
2 options.

Since each player only makes around 15 moves per game, and evaluation is slowest
near the beginning of the game, this provides a significant speedup, and saves
3 to 5 seconds per game.

The logic to precompute the table is in
[first-move.cc](player/src/first-move.cc) and the computed table
is just imported as a source file in
[first-move-table.cc](player/src/first-move-table.cc).


### The end
