'use strict';

const transcriptFileInputElem = document.getElementById('transcript-file-input');
const competitionFileInputElem = document.getElementById('competition-file-input');
const competitionTableElem = document.getElementById('competition-table');

function encodeScalarParameter(obj) {
  if (typeof obj === 'string') return encodeURIComponent(obj);
  if (typeof obj === 'number') return String(obj);
  console.error('Cannot encode', obj);
  throw new Error('Unsupported parameter type');
}

function encodeParameter(obj) {
  if (Array.isArray(obj)) return obj.map(encodeScalarParameter).join(',');
  return encodeScalarParameter(obj);
}

function encodeParameters(obj) {
  return Object.entries(obj)
      .filter(([_, val]) => val != null)
      .map(([key, val]) => encodeScalarParameter(key) + '=' + encodeParameter(val))
      .join('&');
}

function parseTranscript(text) {
  const lines = text
      .split(/[\r\n]+/)
      .map(line => line.replace(/#.*/, '').trim())
      .filter(x => x);

  if (lines.length === 0) throw new Error('File is empty!');

  if (!lines[0].match(/[1-6] [1-6]/)) {
    throw new Error('Invalid secret colors!');
  }
  const secretColors = lines[0].split(/\s+/).map((s) => parseInt(s, 10));
  const moveStrings = lines.slice(1);
  return {secretColors, moveStrings};
}

function loadTranscriptFile(file) {
  if (file == null) return;  // nothing selected

  // Try to infer player names from filename ("foo-vs-bar-transcript.txt"):
  const match = file.name && file.name.match(/([^-]*)-vs-([^-]*)-transcript.txt/);
  const playerNames = match ? [match[1], match[2]] : undefined;

  function loadFileContent(text) {
    try {
      const {secretColors, moveStrings} = parseTranscript(text);
      document.location.href = 'viewer.html#?' +
          encodeParameters({secretColors, moveStrings, playerNames});

      // Clear the selected file. Without this, there is an annoying bug where
      // if you use the `back` button to return to this page, the last file is
      // still selected, and you cannot reload it because selecting the same
      // file doesn't count as a change.
      transcriptFileInputElem.value = null;
    } catch (e) {
      console.error('Failed to load transcript!', e);
      alert('Failed to load transcript!\nSee Javascript console for details.');
    }
  }

  // Read file content.
  var reader = new FileReader();
  reader.onload = (e) => loadFileContent(e.target.result);
  reader.readAsText(file);
}

function loadCompetitionFile(file) {
  if (file == null) return;  // nothing selected

  function loadFileContent(text) {
    try {
      const lines = text.split(/[\r\n]+/);
      if (lines.length === 0) throw new Error('File is empty!');
      const rows = lines.map(line => line.split(','));

      const fieldIndex = {};
      for (let i = 0; i < rows[0].length; ++i) {
        fieldIndex[rows[0][i]] = i;
      }

      // Repopulate table header
      {
        const thead = competitionTableElem.querySelector('thead');
        const tr = document.createElement('tr');
        thead.replaceChildren(tr);
        for (let field of rows[0]) {
          const th = document.createElement('th');
          th.appendChild(document.createTextNode(field));
          tr.appendChild(th);
        }
      }

      // Repopulate table body
      {
        const tbody = competitionTableElem.querySelector('tbody');
        tbody.replaceChildren();
        for (let i = 1; i < rows.length; ++i) {
          const tr = document.createElement('tr');
          tbody.appendChild(tr);
          for (let j = 0; j < rows[i].length; ++j) {
            const field = rows[i][j];
            const td = document.createElement('td');
            if (field.match(/^[+-]?\d+$/)) {
              td.style.textAlign = 'right';
            }
            const text = document.createTextNode(field);
            if (rows[0][j] === "Moves") {
              // Add clickable link to view game.
              const params = {
                moveStrings: field.split(/\s+/),
                playerNames: [
                  rows[i][rows[0].indexOf('User1')] ?? 'User 1',
                  rows[i][rows[0].indexOf('User2')] ?? 'User 2',
                ],
                secretColors: [
                  rows[i][rows[0].indexOf('Color1')] ?? 0,
                  rows[i][rows[0].indexOf('Color2')] ?? 0,
                ],
              };
              const a = document.createElement('a');
              a.href = 'viewer.html#?' + encodeParameters(params);
              a.appendChild(text);
              td.appendChild(a);
            } else {
              td.appendChild(text);
            }
            tr.appendChild(td);
          }
        }
      }

      // Clear the selected file. Without this, there is an annoying bug where
      // if you use the `back` button to return to this page, the last file is
      // still selected, and you cannot reload it because selecting the same
      // file doesn't count as a change.
      competitionFileInputElem.value = null;
    } catch (e) {
      console.error('Failed to load competition!', e);
      alert('Failed to load competition!\nSee Javascript console for details.');
    }
  }

  // Read file content.
  var reader = new FileReader();
  reader.onload = (e) => loadFileContent(e.target.result);
  reader.readAsText(file);
}

transcriptFileInputElem.disabled = false;
transcriptFileInputElem.addEventListener('change', (e) => loadTranscriptFile(e.target.files[0]));

competitionFileInputElem.disabled = false;
competitionFileInputElem.addEventListener('change', (e) => loadCompetitionFile(e.target.files[0]));
