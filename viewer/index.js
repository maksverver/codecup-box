'use strict';

const inputFileElem = document.getElementById('input-file');

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

  // Try to infer filenames from filename ("foo-vs-bar-transcript.txt"):
  const match = file.name && file.name.match(/(.*)-vs-(.*)-transcript.txt/);
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
      inputFileElem.value = null;
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

inputFileElem.disabled = false;
inputFileElem.addEventListener('change', (e) => loadTranscriptFile(e.target.files[0]));
