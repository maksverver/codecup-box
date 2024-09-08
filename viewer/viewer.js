'use strict';

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

function calculateScores(squares) {
  const scores = Array(COLORS + 1).fill(0);
  for (const {color, size} of squares) scores[color] += size;
  return scores;
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

function repopulateMovesTable(moveStrings, onRowClicked) {
  const rowElements = [];
  let selectedIndex = -1;

  function setSelectedRow(i) {
    if (Object.hasOwn(rowElements, selectedIndex)) {
      rowElements[selectedIndex].classList.remove('selected');
    }
    selectedIndex = i;
    if (Object.hasOwn(rowElements, selectedIndex)) {
      rowElements[selectedIndex].classList.add('selected');
    }
  }

  function addMove(i, s) {
    const tr = createElement(null, 'tr');
    tr.classList.add('clickable');
    tr.onclick = () => onRowClicked(i);
    const th = createElement(tr, 'th');
    th.appendChild(document.createTextNode(i));
    if (i > 0 && i % 2 == 0) createElement(tr, 'td');
    const td = createElement(tr, 'td');
    td.className = 'move';
    if (i == 0) td.colSpan = 2;
    td.appendChild(document.createTextNode(s));
    if (i > 0 && i % 2 == 1) createElement(tr, 'td');  // needed for styling
    return tr;
  }

  if (moveStrings.length > 0) {
    rowElements.push(addMove(0, moveStrings[0], 2));
    for (let i = 1; i < moveStrings.length; ++i) {
      rowElements.push(addMove(i, moveStrings[i]));
    }
  }

  if (Object.hasOwn(rowElements, selectedIndex)) {
    rowElements[selectedIndex].classList.add('selected');
  }

  movesTbody.replaceChildren(...rowElements);
  return {setSelectedRow};
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

function repopulateBoard(boardState) {
  const {turns, squares} = boardState;
  tilesG.replaceChildren();
  let lastTileG = undefined;
  for (const {move, digits} of turns) {
    const {row, col, vert} = move;
    const tileG = lastTileG = createSvgElement(tilesG, 'g', 'tile');
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

  if (lastTileG) {
    lastTileG.classList.add('last-move');
  }

  // Sort by size, descending. This tends to improve visualization because large
  // squares don't overlap smaller squares (and the opposite is less of a problem
  // since large squares have larger outlines so they are more visible anyway).
  const sortedSquares = Array.from(squares);
  sortedSquares.sort((a, b) => b.size - a.size);
  const squareGs = sortedSquares.map(({color, r1, c1, size}) => {
    const squareG = createSvgElement(null, 'g', ['square', 'color-' + color]);
    const rectAttributes = {
      x: 10*c1 + 0.75,
      y: 10*r1 + 0.75,
      width: 10*size + 8.5,
      height: 10*size + 8.5,
    };
    createSvgElement(squareG, 'rect', 'background', rectAttributes);
    createSvgElement(squareG, 'rect', 'foreground', rectAttributes);
    return squareG;
  });
  squaresG.replaceChildren(...squareGs);
}

function calculateBoardState(moves) {
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
  const squares = detectSquares(grid);
  const scores = calculateScores(squares);
  return {grid, turns, squares, scores};
}

function initialize(playerNames, secretColors, moveStrings) {
  const moves = moveStrings.map(parseMove);

  const lastMoveIndex = moves.length - 1;
  let selectedMoveIndex = -1;

  const beginButton = document.getElementById('navigate-begin');
  const endButton = document.getElementById('navigate-end');
  const prevButton = document.getElementById('navigate-prev');
  const nextButton = document.getElementById('navigate-next');
  beginButton.onclick = () => setSelectedMoveIndex(0);
  endButton.onclick = () => setSelectedMoveIndex(lastMoveIndex);
  prevButton.onclick = () => adjustSelectedMoveIndex(-1);
  nextButton.onclick = () => adjustSelectedMoveIndex(+1);

  document.onkeydown = (e) => {
    switch (e.key) {
      case 'ArrowLeft': adjustSelectedMoveIndex(-1); break;
      case 'ArrowRight': adjustSelectedMoveIndex(+1); break;
      case 'ArrowUp': adjustSelectedMoveIndex(-2); break;
      case 'ArrowDown': adjustSelectedMoveIndex(+2); break;
      case 'Home': setSelectedMoveIndex(0); break;
      case 'End': setSelectedMoveIndex(lastMoveIndex); break;
    }
  };

  function adjustSelectedMoveIndex(delta) {
    if (selectedMoveIndex < 0) return;
    setSelectedMoveIndex(Math.max(0, Math.min(lastMoveIndex, selectedMoveIndex + delta)));
  }

  function setSelectedMoveIndex(i) {
    if (i === selectedMoveIndex) return;
    const boardState = calculateBoardState(moves.slice(0, i + 1));
    repopulateBoard(boardState);
    updateScoresTable(playerNames, secretColors, boardState.scores);
    repopulateSquaresTable(playerNames, secretColors, boardState.squares);
    selectedMoveIndex = i;
    setSelectedRow(i);
    beginButton.disabled = prevButton.disabled = i <= 0;
    endButton.disabled = nextButton.disabled = i >= lastMoveIndex;
  }

  const {setSelectedRow} = repopulateMovesTable(moveStrings, setSelectedMoveIndex);
  setSelectedMoveIndex(lastMoveIndex);
}

(function() {
  const hash = window.location.hash;
  const sep = hash.indexOf('?');
  if (sep < 0) {
    console.warn('Missing hash parameters');
    return;
  }
  let secretColors = [0, 0];
  let moveStrings = [];
  let playerNames = ['Player 1', 'Player 2'];
  for (const part of hash.substring(sep + 1).split('&')) {
    const sep = part.indexOf('=');
    if (sep < 0) {
      console.warn('Ignoring hash parameter without equals sign');
      continue;
    }
    const key = decodeURIComponent(part.substring(0, sep));
    const value = part.substring(sep + 1);
    switch (key) {
      case 'secretColors':
        secretColors = value.split(',').map((s) => parseInt(s, 10));
        break;
      case 'moveStrings':
        moveStrings = value.split(',').map(decodeURIComponent);
        break;
      case 'playerNames':
        playerNames = value.split(',').map(decodeURIComponent);
        break;
      default:
        console.warn('Ignoring hash parameter with unkown key', key);
        break;
    }
  }
  initialize(playerNames, secretColors, moveStrings);
})();
