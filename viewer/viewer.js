const HEIGHT = 16;
const WIDTH  = 20;
const COLORS =  6;

const ROW_COORDS = 'ABCDEFGHIJKLMNOP';
const COL_COORDS = 'abcdefghijklmnopqrst';

const boardSvg = document.getElementById('board');
const tilesG = document.getElementById('tiles');
const squaresG = document.getElementById('squares');

const SVG_NS = 'http://www.w3.org/2000/svg';

function createSvgElement(parent, tag, classes, attributes) {
  const elem = document.createElementNS(SVG_NS, tag);
  if (parent) {
    parent.appendChild(elem);
  }
  if (classes) {
    elem.classList.add(...classes);
  }
  if (attributes) {
    for (const [key, value] of Object.entries(attributes)) {
      elem.setAttribute(key, value);
    }
  }
  return elem;
}

function parseMove(move) {
  if (!/^[A-P][a-t][1-6]{6}[hv]$/.test(move)) {
    throw new Error('Invalid move!');
  }
  return {
    row: ROW_COORDS.indexOf(move[0]),
    col: COL_COORDS.indexOf(move[1]),
    digits: Array.from(move.substring(2, 8), (ch) => parseInt(ch, 10)),
    vert: move[8] == 'v',
  }
}

function parseMoves(moves) {
  const result = []
  for (const move of moves) {
    try {
      result.push(parseMove(move));
    } catch (e) {
      console.error('Failed to parse move', move, e);
      alert('Failed to parse move! (See console for details)');
      break;
    }
  }
  return result;
}

function detectSquares(grid) {
  const squares = []
  for (let r1 = 0; r1 < HEIGHT; ++r1) {
    for (let c1 = 0; c1 < WIDTH; ++c1) {
      const color = grid[r1][c1];
      if (color !== 0) {
        for (let r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
          if (grid[r1][c2] === color && grid[r2][c1] === color && grid[r2][c2] === color) {
            squares.push([r1, c1, r2, c2]);
          }
        }
      }
    }
  }
  return squares;
}

// TODO: allow the user to upload transcript or input moves instead of hardcoding
moves = parseMoves(['Hh216345h', 'Df145632v', 'Jb613254h', 'Ff634521h', 'Ai124356v', 'Jf641235h', 'Dh362541h', 'Ai641235h', 'Bn632451h', 'Bp321546v', 'Jf536412v', 'Lb652413h', 'Na621453h', 'Db365124h', 'Ds123465v', 'Ac532614h', 'Jr435621v', 'Ga126453h', 'Ba136245h', 'Kj163542v', 'Lh645312h', 'Od234615h', 'Kp245631v', 'Io623154h', 'Kn256431v', 'Em341625v', 'Kl263145v']);

let grid = Array.from({length: HEIGHT}, () => Array.from({length: WIDTH}, () => 0));
for (const {row, col, digits, vert} of moves) {

  let digitsToPlace = [];  // [r, c, digit] triples
  for (let i = 0; i < 6; ++i) {
    digitsToPlace.push([row + (vert ? 5 - i : 0), col + (vert ? 0 : i), digits[i]]);
    digitsToPlace.push([row + (vert ? 5 - i : 1), col + (vert ? 1 : i), digits[5 - i]]);
  }

  const tileG = createSvgElement(tilesG, 'g', ['tile']);
  const borderProps = {
    x: 10 * col,
    y: 10 * row,
    width: vert ? 20 : 60,
    height: vert ? 60 : 20,
    /* Keep 3px in sync with clip-path values in viewer.js! */
    rx: 3,
    ry: 3,
  };
  createSvgElement(tileG, 'rect', ['tile-background'], borderProps);

  const cellsG = createSvgElement(tileG, 'g', ['tile-cells']);

  for (const [r, c, digit] of digitsToPlace) {
    grid[r][c] = digit;

    const cellG = createSvgElement(cellsG, 'g', ['tile-cell', 'color-' + digit]);
    createSvgElement(cellG, 'rect', ['cell-background'], {
      x: 10 * c,
      y: 10 * r,
      width: 10,
      height: 10,
    });
    createSvgElement(cellG, 'text', ['label'], {
      x: 10 * c + 5,
      y: 10 * r + 6,
    }).appendChild(document.createTextNode(String(digit)));
  }

  createSvgElement(tileG, 'rect', ['tile-border'], borderProps);
}

const scores = Array(COLORS + 1).fill(0);  // unused?

const squares = detectSquares(grid);
// Sort by size, descending. This tends to improve visualization because large
// squares don't overlap smaller squares (and the opposite is less of a problem
// since large squares have larger outlines so they are more visible anyway).
squares.sort(([r1, c1, r2, c2], [r3, c3, r4, c4]) => (c4 - c3) - (c2 - c1));
for (const [r1, c1, r2, c2] of squares) {
  const color = grid[r1][c1];
  const size = c2 - c1;
  scores[color] += size;

  const squareG = createSvgElement(squaresG, 'g', ['square', 'color-' + color]);
  const rectAttributes = {
    x: 10*c1,
    y: 10*r1,
    width: 10*size + 10,
    height: 10*size + 10,
  };
  createSvgElement(squareG, 'rect', ['background'], rectAttributes);
  createSvgElement(squareG, 'rect', ['foreground'], rectAttributes);
}
