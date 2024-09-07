const HEIGHT = 16;
const WIDTH  = 20;
const COLORS =  6;

const ROW_COORDS = 'ABCDEFGHIJKLMNOP';
const COL_COORDS = 'abcdefghijklmnopqrst';

const boardSvg = document.getElementById('board');
const tilesG = document.getElementById('tiles');
const squaresG = document.getElementById('squares');
const movesTbody = document.getElementById('moves-tbody');
const squaresTBody = document.getElementById('squares-tbody');

const SVG_NS = 'http://www.w3.org/2000/svg';

function createSvgElement(parent, tag, classes, attributes) {
  const elem = document.createElementNS(SVG_NS, tag);
  if (parent) {
    parent.appendChild(elem);
  }
  if (typeof classes === 'string') {
    elem.classList.add(classes);
  } else if (classes?.[Symbol.iterator]) {
    elem.classList.add(...classes);
  } else if (classes != null) {
    log.error('Invalid classes argument', classes)
  }
  if (attributes) {
    for (const [key, value] of Object.entries(attributes)) {
      elem.setAttribute(key, value);
    }
  }
  return elem;
}

function createElement(parent, tag, classes) {
  const elem = document.createElement(tag);
  if (parent) {
    parent.appendChild(elem);
  }
  if (typeof classes === 'string') {
    elem.classList.add(classes);
  } else if (classes?.[Symbol.iterator]) {
    elem.classList.add(...classes);
  } else if (classes != null) {
    console.error('Invalid classes argument', classes)
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
    tile: Array.from(move.substring(2, 8), (ch) => parseInt(ch, 10)),
    vert: move[8] == 'v',
  };
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
  const squares = [];
  for (let r1 = 0; r1 < HEIGHT; ++r1) {
    for (let c1 = 0; c1 < WIDTH; ++c1) {
      const color = grid[r1][c1];
      if (color !== 0) {
        for (let r2 = r1 + 1, c2 = c1 + 1; r2 < HEIGHT && c2 < WIDTH; ++r2, ++c2) {
          if (grid[r1][c2] === color && grid[r2][c1] === color && grid[r2][c2] === color) {
            squares.push({color, r1, c1, r2, c2, size: c2 - c1});
          }
        }
      }
    }
  }
  return squares;
}

function updateScoresTable(playerNames, secretColors, scores) {
  const [c1, c2] = secretColors;
  document.getElementById('scores-table-player-1').textContent = playerNames[0];
  document.getElementById('scores-table-player-2').textContent = playerNames[1];
  document.getElementById('scores-table-color-1').textContent = c1;
  document.getElementById('scores-table-color-1').className = 'center background-color-' + c1;
  document.getElementById('scores-table-color-2').textContent = c2;
  document.getElementById('scores-table-color-2').className = 'center background-color-' + c2;
  document.getElementById('scores-table-score-1').textContent = scores[c1];
  document.getElementById('scores-table-score-2').textContent = scores[c2];
}

function repopulateMovesTable(table, moveStrings) {
  table.replaceChildren();

  function addMove(i, s) {
    const tr = createElement(table, 'tr');
    const th = createElement(tr, 'th');
    th.appendChild(document.createTextNode(i));
    if (i > 0 && i % 2 == 0) createElement(tr, 'td');
    const td = createElement(tr, 'td');
    td.className = 'move';
    if (i == 0) td.colSpan = 2;
    td.appendChild(document.createTextNode(s));
    if (i > 0 && i % 2 == 1) createElement(tr, 'td');  // needed for styling
  }

  if (moveStrings.length === 0) return;
  addMove(0, moveStrings[0], 2);
  for (let i = 1; i < moveStrings.length; ++i) {
    addMove(i, moveStrings[i]);
  }
}

function repopulateSquaresTable(playerNames, secretColors, squares) {
  const tableRows = [];
  for (const {color, r1, c1, r2, c2, size} of squares) {
    const tr = document.createElement('tr');
    const player = secretColors.indexOf(color);
    const coords = ROW_COORDS[r1] + COL_COORDS[c1] + ROW_COORDS[r2] + COL_COORDS[c2];
    const playerTd = createElement(tr, 'td');
    if (player >= 0) playerTd.textContent = playerNames[player];
    const colorTd = createElement(tr, 'td');
    colorTd.textContent = color;
    colorTd.classList = 'background-color-' + color;
    createElement(tr, 'td', 'square-coords').textContent = coords;
    createElement(tr, 'td', 'square-size').textContent = size;
    tableRows.push(tr);
  }
  squaresTBody.replaceChildren(...tableRows);
}

// TODO: allow the user to upload transcript or input moves instead of hardcoding
const moveStrings = ['Hh216345h', 'Df145632v', 'Jb613254h', 'Ff634521h', 'Ai124356v', 'Jf641235h', 'Dh362541h', 'Ai641235h', 'Bn632451h', 'Bp321546v', 'Jf536412v', 'Lb652413h', 'Na621453h', 'Db365124h', 'Ds123465v', 'Ac532614h', 'Jr435621v', 'Ga126453h', 'Ba136245h', 'Kj163542v', 'Lh645312h', 'Od234615h', 'Kp245631v', 'Io623154h', 'Kn256431v', 'Em341625v', 'Kl263145v'];
const moves = parseMoves(moveStrings);
const playerNames = ["Player 1", "Player 2"];
const secretColors = [3, 1];

repopulateMovesTable(movesTbody, moveStrings);

let grid = Array.from({length: HEIGHT}, () => Array.from({length: WIDTH}, () => 0));

const turns = [];
for (const move of moves) {
  const {row, col, tile, vert} = move;
  const digits = [];  // [r, c, digit] triples
  for (let i = 0; i < 6; ++i) {
    digits.push([row + (vert ? 5 - i : 0), col + (vert ? 0 : i), tile[i]]);
    digits.push([row + (vert ? 5 - i : 1), col + (vert ? 1 : i), tile[5 - i]]);
  }
  for (const [r, c, digit] of digits) {
    grid[r][c] = digit;
  }
  turns.push({move, digits});
}

for (const {move, digits} of turns) {
  const {row, col, vert} = move;
  const tileG = createSvgElement(tilesG, 'g', 'tile');
  const borderProps = {
    x: 10 * col,
    y: 10 * row,
    width: vert ? 20 : 60,
    height: vert ? 60 : 20,
    /* Keep 3px in sync with clip-path values in viewer.js! */
    rx: 3,
    ry: 3,
  };
  createSvgElement(tileG, 'rect', 'tile-background', borderProps);

  const cellsG = createSvgElement(tileG, 'g', 'tile-cells');

  for (const [r, c, digit] of digits) {
    const cellG = createSvgElement(cellsG, 'g', ['tile-cell', 'color-' + digit]);
    createSvgElement(cellG, 'rect', 'cell-background', {
      x: 10 * c,
      y: 10 * r,
      width: 10,
      height: 10,
    });
    createSvgElement(cellG, 'text', 'label', {
      x: 10 * c + 5,
      y: 10 * r + 6,
    }).appendChild(document.createTextNode(String(digit)));
  }

  createSvgElement(tileG, 'rect', 'tile-border', borderProps);
}

function calculateScores(squares) {
  const scores = Array(COLORS + 1).fill(0);
  for (const {color, size} of squares) scores[color] += size;
  return scores;
}

const squares = detectSquares(grid);
const scores = calculateScores(squares);

// Sort by size, descending. This tends to improve visualization because large
// squares don't overlap smaller squares (and the opposite is less of a problem
// since large squares have larger outlines so they are more visible anyway).
const sortedSquares = Array.from(squares);
sortedSquares.sort((a, b) => b.size - a.size);
for (const {color, r1, c1, size} of sortedSquares) {
  scores[color] += size;

  const squareG = createSvgElement(squaresG, 'g', ['square', 'color-' + color]);
  const rectAttributes = {
    x: 10*c1 + 0.75,
    y: 10*r1 + 0.75,
    width: 10*size + 8.5,
    height: 10*size + 8.5,
  };
  createSvgElement(squareG, 'rect', 'background', rectAttributes);
  createSvgElement(squareG, 'rect', 'foreground', rectAttributes);
}

updateScoresTable(playerNames, secretColors, scores);
repopulateSquaresTable(playerNames, secretColors, squares);
