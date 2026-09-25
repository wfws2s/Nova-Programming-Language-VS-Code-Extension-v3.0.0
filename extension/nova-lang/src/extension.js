const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

// ---------------------------------------------------------------------------
// Static language data (NOVA v0.3.0)
// ---------------------------------------------------------------------------

const RESERVED_KEYWORDS = [
  'let', 'if', 'elif', 'else', 'end', 'while', 'for', 'in', 'fn', 'return',
  'break', 'continue', 'enum', 'and', 'or', 'not', 'true', 'false', 'null',
  'try', 'catch', 'is', 'import', 'as', 'class', 'extends', 'self', 'static',
  'type', 'typeof', 'global', 'new', 'super'
];

const BLOCK_OPENERS = ['if', 'while', 'for', 'fn', 'class', 'enum', 'try'];
const LOOP_OPENERS = ['while', 'for'];

const BUILTIN_FUNCTIONS = {
  print: { doc: 'print(...) -> Null\n\nPrints all arguments separated by spaces to stdout with a newline.', sig: 'print(...values)' },
  input: { doc: 'input(prompt) -> String\n\nReads a single line of input from stdin.', sig: 'input(prompt="")' },
  read: { doc: 'read(prompt) -> Any\n\nReads stdin and parses literals (numbers, booleans, lists, etc.) recursively.', sig: 'read(prompt="")' },
  type: { doc: 'type(val) -> String\n\nReturns the canonical type name of the value (e.g. "String", "Number", "List", or custom Class name).', sig: 'type(val)' },
  typeof: { doc: 'typeof(val) -> String\n\nAlias for type(val).', sig: 'typeof(val)' },
  number: { doc: 'number(val) -> Number\n\nParses a String or converts a Boolean into a Number.', sig: 'number(val)' },
  string: { doc: 'string(val) -> String\n\nFormats any value to its String representation.', sig: 'string(val)' },
  list: { doc: 'list(val) -> List\n\nConverts a String, Dictionary, or iterable to a List.', sig: 'list(val)' },
  len: { doc: 'len(val) -> Number\n\nReturns the length of a String, List, or Dictionary.', sig: 'len(val)' },
  split: { doc: 'split(str, delim) -> List\n\nSplits a String into a List of substrings by a delimiter.', sig: 'split(str, delim=" ")' },
  append: { doc: 'append(list, val) -> Null\n\nAppends an item to the end of a list in place.', sig: 'append(list, val)' },
  pop: { doc: 'pop(list) -> Value\n\nRemoves and returns the last element of a list.', sig: 'pop(list)' },
  push: { doc: 'push(list, val) -> Null\n\nAlias for append(list, val).', sig: 'push(list, val)' },
  keys: { doc: 'keys(dict) -> List\n\nReturns a list of all keys in the dictionary.', sig: 'keys(dict)' },
  values: { doc: 'values(dict) -> List\n\nReturns a list of all values in the dictionary.', sig: 'values(dict)' },
  range: { doc: 'range(start, end, step) -> List\n\nGenerates a list of numbers in the specified range.', sig: 'range(start, end, step=1)' },
  int: { doc: 'int(val) -> Number\n\nConverts a value to an integer (truncating decimals).', sig: 'int(val)' },
  float: { doc: 'float(val) -> Number\n\nConverts a value to a floating-point number.', sig: 'float(val)' },
  bool: { doc: 'bool(val) -> Boolean\n\nConverts a value to its truthy/falsy boolean value.', sig: 'bool(val)' },
  str: { doc: 'str(val) -> String\n\nAlias for string(val).', sig: 'str(val)' },
  abs: { doc: 'abs(val) -> Number\n\nReturns the absolute value of a number.', sig: 'abs(num)' },
  min: { doc: 'min(...) -> Number\n\nReturns the minimum value among arguments or list.', sig: 'min(a, b, ...)' },
  max: { doc: 'max(...) -> Number\n\nReturns the maximum value among arguments or list.', sig: 'max(a, b, ...)' },
  round: { doc: 'round(val) -> Number\n\nReturns the number rounded to the nearest integer.', sig: 'round(num)' },
  chr: { doc: 'chr(code) -> String\n\nReturns the character with the given ASCII/Unicode code point.', sig: 'chr(code)' },
  ord: { doc: 'ord(char) -> Number\n\nReturns the ASCII/Unicode code point of the single character.', sig: 'ord(char)' }
};

const MODULES = {
  file: {
    read: { doc: 'file.read(path) -> String\n\nReads the entire content of a file as a string.', sig: 'file.read(path)' },
    write: { doc: 'file.write(path, content) -> Boolean\n\nOverwrites the specified file with text content.', sig: 'file.write(path, content)' },
    append: { doc: 'file.append(path, content) -> Boolean\n\nAppends text content to the end of the file.', sig: 'file.append(path, content)' },
    exists: { doc: 'file.exists(path) -> Boolean\n\nChecks whether the file or directory exists.', sig: 'file.exists(path)' },
    remove: { doc: 'file.remove(path) -> Boolean\n\nDeletes the specified file.', sig: 'file.remove(path)' },
    lines: { doc: 'file.lines(path) -> List\n\nReads the file and returns a list of lines.', sig: 'file.lines(path)' }
  },
  io: {
    read: { doc: 'io.read(path) -> String\n\n(Alias for file.read) Reads file contents.', sig: 'io.read(path)' },
    write: { doc: 'io.write(path, content) -> Boolean\n\n(Alias for file.write) Writes file contents.', sig: 'io.write(path, content)' },
    append: { doc: 'io.append(path, content) -> Boolean\n\n(Alias for file.append) Appends to file.', sig: 'io.append(path, content)' },
    exists: { doc: 'io.exists(path) -> Boolean\n\n(Alias for file.exists) Checks file existence.', sig: 'io.exists(path)' },
    remove: { doc: 'io.remove(path) -> Boolean\n\n(Alias for file.remove) Deletes file.', sig: 'io.remove(path)' },
    lines: { doc: 'io.lines(path) -> List\n\n(Alias for file.lines) Reads lines into a list.', sig: 'io.lines(path)' }
  },
  os: {
    platform: { doc: 'os.platform() -> String\n\nReturns the operating system platform ("windows", "linux", "macos").', sig: 'os.platform()' },
    cwd: { doc: 'os.cwd() -> String\n\nReturns the current working directory path.', sig: 'os.cwd()' },
    env: { doc: 'os.env(name) -> String|Null\n\nGets the value of an environment variable.', sig: 'os.env(name)' },
    exit: { doc: 'os.exit(code) -> Null\n\nTerminates the program immediately with an exit code.', sig: 'os.exit(code=0)' },
    exec: { doc: 'os.exec(command) -> Number\n\nExecutes a shell command synchronously and returns the exit code.', sig: 'os.exec(command)' },
    args: { doc: 'os.args() -> List\n\nReturns the CLI arguments passed to the script.', sig: 'os.args()' }
  },
  sys: {
    platform: { doc: 'sys.platform() -> String\n\n(Alias for os.platform) Returns OS platform.', sig: 'sys.platform()' },
    cwd: { doc: 'sys.cwd() -> String\n\n(Alias for os.cwd) Returns current working directory.', sig: 'sys.cwd()' },
    env: { doc: 'sys.env(name) -> String|Null\n\n(Alias for os.env) Gets an environment variable.', sig: 'sys.env(name)' },
    exit: { doc: 'sys.exit(code) -> Null\n\n(Alias for os.exit) Exits the process.', sig: 'sys.exit(code=0)' },
    exec: { doc: 'sys.exec(command) -> Number\n\n(Alias for os.exec) Runs a shell command.', sig: 'sys.exec(command)' },
    args: { doc: 'sys.args() -> List\n\n(Alias for os.args) CLI arguments.', sig: 'sys.args()' }
  },
  json: {
    parse: { doc: 'json.parse(json_str) -> Any\n\nParses a JSON string into NOVA data types (Dictionary, List, String, Number, Boolean, Null).', sig: 'json.parse(json_str)' },
    stringify: { doc: 'json.stringify(val) -> String\n\nSerializes a NOVA value into a JSON formatted string.', sig: 'json.stringify(val)' }
  },
  time: {
    now: { doc: 'time.now() -> Number\n\nReturns the current UNIX timestamp in seconds.', sig: 'time.now()' },
    sleep: { doc: 'time.sleep(seconds) -> Null\n\nPauses execution for the specified number of seconds (floats supported).', sig: 'time.sleep(seconds)' },
    clock: { doc: 'time.clock() -> Number\n\nReturns the processor time consumed by the program in seconds.', sig: 'time.clock()' }
  },
  random: {
    random: { doc: 'random.random() -> Number\n\nReturns a random float in the range [0.0, 1.0).', sig: 'random.random()' },
    randint: { doc: 'random.randint(min, max) -> Number\n\nReturns a random integer in the range [min, max].', sig: 'random.randint(min, max)' },
    choice: { doc: 'random.choice(list) -> Any\n\nReturns a random element from a list.', sig: 'random.choice(list)' },
    shuffle: { doc: 'random.shuffle(list) -> Null\n\nShuffles the elements of a list in place.', sig: 'random.shuffle(list)' },
    uniform: { doc: 'random.uniform(a, b) -> Number\n\nReturns a random float uniformly distributed between a and b.', sig: 'random.uniform(a, b)' },
    range: { doc: 'random.range(start, end) -> Number\n\nReturns a random integer in range [start, end).', sig: 'random.range(start, end)' }
  },
  math: {
    sqrt: { doc: 'math.sqrt(x) -> Number\n\nSquare root of x.', sig: 'math.sqrt(x)' },
    pow: { doc: 'math.pow(base, exp) -> Number\n\nBase raised to the power exp.', sig: 'math.pow(base, exp)' },
    floor: { doc: 'math.floor(x) -> Number\n\nLargest integer less than or equal to x.', sig: 'math.floor(x)' },
    ceil: { doc: 'math.ceil(x) -> Number\n\nSmallest integer greater than or equal to x.', sig: 'math.ceil(x)' },
    round: { doc: 'math.round(x) -> Number\n\nNearest integer to x.', sig: 'math.round(x)' },
    abs: { doc: 'math.abs(x) -> Number\n\nAbsolute value of x.', sig: 'math.abs(x)' },
    sin: { doc: 'math.sin(rad) -> Number\n\nSine of angle in radians.', sig: 'math.sin(rad)' },
    cos: { doc: 'math.cos(rad) -> Number\n\nCosine of angle in radians.', sig: 'math.cos(rad)' },
    tan: { doc: 'math.tan(rad) -> Number\n\nTangent of angle in radians.', sig: 'math.tan(rad)' },
    min: { doc: 'math.min(a, b) -> Number\n\nSmaller of two numbers.', sig: 'math.min(a, b)' },
    max: { doc: 'math.max(a, b) -> Number\n\nLarger of two numbers.', sig: 'math.max(a, b)' },
    pi: { doc: 'math.pi -> Number\n\nThe mathematical constant π (3.141592653589793).', sig: 'math.pi' },
    e: { doc: 'math.e -> Number\n\nEuler’s number e (2.718281828459045).', sig: 'math.e' }
  },
  string: {
    to_upper: { doc: 'string.to_upper(s) -> String\n\nConverts string to uppercase.', sig: 'string.to_upper(s)' },
    to_lower: { doc: 'string.to_lower(s) -> String\n\nConverts string to lowercase.', sig: 'string.to_lower(s)' },
    trim: { doc: 'string.trim(s) -> String\n\nRemoves leading and trailing whitespace.', sig: 'string.trim(s)' },
    starts_with: { doc: 'string.starts_with(s, prefix) -> Boolean\n\nChecks if string begins with prefix.', sig: 'string.starts_with(s, prefix)' },
    ends_with: { doc: 'string.ends_with(s, suffix) -> Boolean\n\nChecks if string ends with suffix.', sig: 'string.ends_with(s, suffix)' },
    contains: { doc: 'string.contains(s, sub) -> Boolean\n\nChecks if substring exists in string.', sig: 'string.contains(s, sub)' },
    join: { doc: 'string.join(list, delim) -> String\n\nJoins list of strings by delimiter.', sig: 'string.join(list, delim="")' }
  },
  list: {
    contains: { doc: 'list.contains(list, item) -> Boolean\n\nChecks if item is present in list.', sig: 'list.contains(list, item)' },
    index_of: { doc: 'list.index_of(list, item) -> Number\n\nReturns 0-based index or -1 if not found.', sig: 'list.index_of(list, item)' },
    reverse: { doc: 'list.reverse(list) -> List\n\nReturns a reversed copy of list.', sig: 'list.reverse(list)' }
  },
  dict: {
    has_key: { doc: 'dict.has_key(dict, key) -> Boolean\n\nChecks if key exists in dictionary.', sig: 'dict.has_key(dict, key)' }
  }
};

const ALL_KNOWN_CALLS = [
  ...Object.keys(BUILTIN_FUNCTIONS),
  ...Object.values(MODULES).flatMap(mod => Object.keys(mod))
];

function editDistance(a, b) {
  const row = Array.from({ length: b.length + 1 }, (_, i) => i);
  for (let i = 1; i <= a.length; i++) {
    let previous = row[0];
    row[0] = i;
    for (let j = 1; j <= b.length; j++) {
      const saved = row[j];
      row[j] = Math.min(row[j] + 1, row[j - 1] + 1, previous + (a[i - 1] === b[j - 1] ? 0 : 1));
      previous = saved;
    }
  }
  return row[b.length];
}

const KEYWORD_COMPLETIONS = RESERVED_KEYWORDS.map(kw => {
  const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Keyword);
  item.detail = 'NOVA reserved keyword';
  return item;
});

// ---------------------------------------------------------------------------
// Document Cleaner / Tokenizer for Diagnostics
// ---------------------------------------------------------------------------

const diagnosticCollection = vscode.languages.createDiagnosticCollection('nova');

function makeDiagnostic(lineNum, startCol, endCol, message, category, severity) {
  const range = new vscode.Range(lineNum, Math.max(0, startCol), lineNum, Math.max(startCol + 1, endCol));
  const diag = new vscode.Diagnostic(range, `${category}: ${message}`, severity);
  diag.source = 'nova';
  diag.code = category;
  return diag;
}

function lintDocument(document) {
  const config = vscode.workspace.getConfiguration('nova');
  if (!config.get('diagnostics.enable', true)) {
    diagnosticCollection.delete(document.uri);
    return;
  }

  const diagnostics = [];
  const lineCount = document.lineCount;

  const constants = new Map(); // name -> line
  const blockStack = [];       // { keyword, line, isLoop }
  const delimiters = [];
  const reservedPattern = new RegExp(`^(${RESERVED_KEYWORDS.join('|')})$`);

  let inTripleString = false;

  for (let lineNum = 0; lineNum < lineCount; lineNum++) {
    const rawLine = document.lineAt(lineNum).text;

    // Handle multiline triple-quoted strings """..."""
    let code = '';
    let inSingleString = false;
    let i = 0;

    while (i < rawLine.length) {
      if (inTripleString) {
        if (rawLine.substr(i, 3) === '"""') {
          inTripleString = false;
          code += '   ';
          i += 3;
          continue;
        }
        code += ' ';
        i++;
        continue;
      }

      if (rawLine.substr(i, 3) === '"""') {
        inTripleString = true;
        code += '   ';
        i += 3;
        continue;
      }

      const ch = rawLine[i];
      if (ch === '#' && !inSingleString) {
        break; // comment
      }

      if (ch === '"') {
        if (i > 0 && rawLine[i - 1] === '\\' && inSingleString) {
          code += ' ';
          i++;
          continue;
        }
        inSingleString = !inSingleString;
        code += '"';
        i++;
        continue;
      }

      code += inSingleString ? ' ' : ch;
      i++;
    }

    // Check single-line unterminated string
    if (inSingleString && !inTripleString) {
      const col = rawLine.lastIndexOf('"');
      diagnostics.push(makeDiagnostic(
        lineNum, col >= 0 ? col : 0, rawLine.length,
        'Unterminated string literal',
        'LexerError',
        vscode.DiagnosticSeverity.Error
      ));
    }

    const trimmed = code.trim();
    if (trimmed.length === 0) continue;

    // Illegal character check outside strings
    const illegal = /[^A-Za-z0-9_\s+\-*/%=!<>&|().,:[\]{}#"\\]/g;
    let illegalMatch;
    while ((illegalMatch = illegal.exec(code)) !== null) {
      diagnostics.push(makeDiagnostic(lineNum, illegalMatch.index, illegalMatch.index + 1,
        `Invalid character '${illegalMatch[0]}'`, 'LexerError', vscode.DiagnosticSeverity.Error));
    }

    // Delimiter balance check
    for (let col = 0; col < code.length; col++) {
      const ch = code[col];
      if ('([{'.includes(ch)) delimiters.push({ ch, line: lineNum, col });
      if (')]}'.includes(ch)) {
        const expected = { ')': '(', ']': '[', '}': '{' }[ch];
        const open = delimiters[delimiters.length - 1];
        if (!open || open.ch !== expected) {
          diagnostics.push(makeDiagnostic(lineNum, col, col + 1,
            `Unexpected '${ch}'`, 'SyntaxError', vscode.DiagnosticSeverity.Error));
        } else {
          delimiters.pop();
        }
      }
    }

    // Block open/close tracking
    const leadingWordMatch = trimmed.match(/^([A-Za-z_][A-Za-z0-9_]*)/);
    const leadingWord = leadingWordMatch ? leadingWordMatch[1] : null;

    if (leadingWord && BLOCK_OPENERS.includes(leadingWord)) {
      blockStack.push({ keyword: leadingWord, line: lineNum, isLoop: LOOP_OPENERS.includes(leadingWord) });
    } else if (leadingWord === 'end') {
      if (blockStack.length === 0) {
        const col = rawLine.indexOf('end');
        diagnostics.push(makeDiagnostic(
          lineNum, col >= 0 ? col : 0, (col >= 0 ? col : 0) + 3,
          "Unexpected 'end' — no matching block opener",
          'SyntaxError',
          vscode.DiagnosticSeverity.Error
        ));
      } else {
        blockStack.pop();
      }
    }

    // Break / Continue outside of loop check
    if (leadingWord === 'break' || leadingWord === 'continue') {
      const insideLoop = blockStack.some(b => b.isLoop);
      if (!insideLoop) {
        const col = rawLine.indexOf(leadingWord);
        diagnostics.push(makeDiagnostic(
          lineNum, col >= 0 ? col : 0, (col >= 0 ? col : 0) + leadingWord.length,
          `'${leadingWord}' cannot be used outside of a loop (while / for)`,
          'SyntaxError',
          vscode.DiagnosticSeverity.Error
        ));
      }
    }

    // Reserved Name check
    const assignMatch = trimmed.match(/^(?:let\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)/);
    if (assignMatch && reservedPattern.test(assignMatch[1])) {
      const name = assignMatch[1];
      const col = rawLine.indexOf(name);
      diagnostics.push(makeDiagnostic(
        lineNum, col >= 0 ? col : 0, (col >= 0 ? col : 0) + name.length,
        `Cannot use reserved word '${name}' as a variable name`,
        'ReservedNameError',
        vscode.DiagnosticSeverity.Error
      ));
    }

    const fnNameMatch = trimmed.match(/^(?:static\s+)?fn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(/);
    if (fnNameMatch && reservedPattern.test(fnNameMatch[1]) && fnNameMatch[1] !== 'init') {
      const name = fnNameMatch[1];
      const col = rawLine.indexOf(name, rawLine.indexOf('fn'));
      diagnostics.push(makeDiagnostic(
        lineNum, col >= 0 ? col : 0, (col >= 0 ? col : 0) + name.length,
        `Cannot use reserved word '${name}' as a function name`,
        'ReservedNameError',
        vscode.DiagnosticSeverity.Error
      ));
    }

    // Constant Reassignment check
    const letMatch = trimmed.match(/^let\s+([A-Za-z_][A-Za-z0-9_]*)\s*=/);
    if (letMatch) {
      constants.set(letMatch[1], lineNum);
    } else if (assignMatch && !reservedPattern.test(assignMatch[1])) {
      const name = assignMatch[1];
      if (constants.has(name)) {
        const col = rawLine.indexOf(name);
        diagnostics.push(makeDiagnostic(
          lineNum, col >= 0 ? col : 0, (col >= 0 ? col : 0) + name.length,
          `Cannot reassign constant '${name}'`,
          'ConstantError',
          vscode.DiagnosticSeverity.Error
        ));
      }
    }

    // Syntax validation rules
    const conditionMatch = trimmed.match(/^(if|elif|while)\b\s*(.*)$/);
    if (conditionMatch && conditionMatch[2].trim() === '') {
      const col = rawLine.indexOf(conditionMatch[1]);
      diagnostics.push(makeDiagnostic(lineNum, col, rawLine.length,
        `'${conditionMatch[1]}' requires a condition expression`, 'SyntaxError', vscode.DiagnosticSeverity.Error));
    }

    const forMatch = trimmed.match(/^for\s+([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$/);
    if (forMatch && !/^in\s+\S/.test(forMatch[2])) {
      diagnostics.push(makeDiagnostic(lineNum, rawLine.indexOf('for'), rawLine.length,
        "Expected 'in <iterable>' after for variable", 'SyntaxError', vscode.DiagnosticSeverity.Error));
    }

    if (/^(?:static\s+)?fn\b/.test(trimmed) && !/^(?:static\s+)?fn\s+[A-Za-z_][A-Za-z0-9_]*\s*\(/.test(trimmed)) {
      diagnostics.push(makeDiagnostic(lineNum, rawLine.indexOf('fn'), rawLine.length,
        "Expected function name and parameter list '(...)' after 'fn'", 'SyntaxError', vscode.DiagnosticSeverity.Error));
    }

    if (/^class\b/.test(trimmed) && !/^class\s+[A-Za-z_][A-Za-z0-9_]*(?:\s+(?:extends|:)\s+[A-Za-z_][A-Za-z0-9_]*)?$/.test(trimmed)) {
      diagnostics.push(makeDiagnostic(lineNum, rawLine.indexOf('class'), rawLine.length,
        "Expected class name (optionally 'extends Parent')", 'SyntaxError', vscode.DiagnosticSeverity.Error));
    }

    if (/^enum\b/.test(trimmed) && !/^enum\s+[A-Za-z_][A-Za-z0-9_]*$/.test(trimmed)) {
      diagnostics.push(makeDiagnostic(lineNum, rawLine.indexOf('enum'), rawLine.length,
        "Expected enum name after 'enum'", 'SyntaxError', vscode.DiagnosticSeverity.Error));
    }

    // Call suggestion check for typos
    const callMatch = trimmed.match(/^([A-Za-z_][A-Za-z0-9_]*)\s*\(/);
    if (callMatch && !ALL_KNOWN_CALLS.includes(callMatch[1])) {
      const nearest = ALL_KNOWN_CALLS.reduce((best, item) =>
        editDistance(callMatch[1], item) < editDistance(callMatch[1], best) ? item : best, ALL_KNOWN_CALLS[0]);
      if (editDistance(callMatch[1], nearest) <= 2) {
        const col = rawLine.indexOf(callMatch[1]);
        diagnostics.push(makeDiagnostic(lineNum, col, col + callMatch[1].length,
          `Unknown call '${callMatch[1]}'. Did you mean '${nearest}'?`, 'NameError', vscode.DiagnosticSeverity.Warning));
      }
    }

    // Division by zero warning
    const divZeroRegex = /\/\s*0(?!\.\d|[0-9])/g;
    let m;
    while ((m = divZeroRegex.exec(code)) !== null) {
      diagnostics.push(makeDiagnostic(
        lineNum, m.index, m.index + m[0].length,
        'Division by zero',
        'RuntimeError',
        vscode.DiagnosticSeverity.Warning
      ));
    }
  }

  // Check unclosed triple quote
  if (inTripleString) {
    diagnostics.push(makeDiagnostic(
      lineCount - 1, 0, document.lineAt(lineCount - 1).text.length,
      'Unterminated triple-quoted string literal (""")',
      'LexerError',
      vscode.DiagnosticSeverity.Error
    ));
  }

  // Unclosed blocks
  for (const open of blockStack) {
    diagnostics.push(makeDiagnostic(
      open.line, 0, document.lineAt(open.line).text.length,
      `'${open.keyword}' block is never closed with 'end'`,
      'SyntaxError',
      vscode.DiagnosticSeverity.Error
    ));
  }

  for (const open of delimiters) {
    diagnostics.push(makeDiagnostic(open.line, open.col, open.col + 1,
      `Unclosed '${open.ch}'`, 'SyntaxError', vscode.DiagnosticSeverity.Error));
  }

  diagnosticCollection.set(document.uri, diagnostics);
}

// ---------------------------------------------------------------------------
// Symbol Table & Type Inference
// ---------------------------------------------------------------------------

function inferType(expression, symbols) {
  const value = expression.trim();
  if (/^"/.test(value) || /^f"/.test(value) || /^"""/.test(value)) return 'String';
  if (/^(true|false)$/.test(value)) return 'Boolean';
  if (/^null$/.test(value)) return 'Null';
  if (/^[+-]?[0-9]+(?:\.[0-9]+)?$/.test(value)) return 'Number';
  if (/^\[/.test(value)) return 'List';
  if (/^\{/.test(value)) return 'Dictionary';
  const construct = value.match(/^([A-Za-z_][A-Za-z0-9_]*)\s*\(/);
  if (construct && symbols.classes.has(construct[1])) return `Instance (${construct[1]})`;
  if (symbols.variables.has(value)) return symbols.variables.get(value).type;
  if (symbols.enums.has(value)) return `Enum (${value})`;
  return 'Unknown';
}

function buildSymbols(document) {
  const symbols = {
    variables: new Map(),
    functions: new Map(),
    classes: new Map(),
    enums: new Map()
  };
  const blocks = [];
  const currentClass = () => [...blocks].reverse().find(block => block.kind === 'class');
  const currentEnum = () => [...blocks].reverse().find(block => block.kind === 'enum');

  for (let line = 0; line < document.lineCount; line++) {
    const rawLine = document.lineAt(line).text;
    const code = rawLine.replace(/#.*$/, '').trim();
    if (!code) continue;

    // Enum definition
    const enumMatch = code.match(/^enum\s+([A-Za-z_][A-Za-z0-9_]*)$/);
    if (enumMatch) {
      const info = { name: enumMatch[1], line, members: [] };
      symbols.enums.set(info.name, info);
      blocks.push({ kind: 'enum', info });
      continue;
    }

    // Enum member
    if (currentEnum()) {
      const memberMatch = code.match(/^([A-Za-z_][A-Za-z0-9_]*)$/);
      if (memberMatch && memberMatch[1] !== 'end') {
        currentEnum().info.members.push(memberMatch[1]);
      }
    }

    // Class definition
    const classMatch = code.match(/^class\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s+(?:extends|:)\s*([A-Za-z_][A-Za-z0-9_]*))?/);
    if (classMatch) {
      const info = { name: classMatch[1], parent: classMatch[2] || null, line, methods: new Map(), properties: new Map() };
      symbols.classes.set(info.name, info);
      blocks.push({ kind: 'class', info });
      continue;
    }

    // Function definition
    const fn = code.match(/^(static\s+)?fn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)/);
    if (fn) {
      const info = { name: fn[2], params: fn[3].split(',').map(p => p.trim()).filter(Boolean), line, static: Boolean(fn[1]), returnType: 'Unknown' };
      const owner = currentClass();
      if (owner) {
        owner.info.methods.set(info.name, info);
      } else {
        symbols.functions.set(info.name, info);
      }
      blocks.push({ kind: 'fn', info });
      continue;
    }

    // Variable declaration
    const variableMatch = code.match(/^(?:let\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$/);
    if (variableMatch && !RESERVED_KEYWORDS.includes(variableMatch[1])) {
      symbols.variables.set(variableMatch[1], { name: variableMatch[1], line, type: inferType(variableMatch[2], symbols) });
    }

    // Class Property
    const propertyMatch = code.match(/^self\.([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$/);
    if (propertyMatch && currentClass()) {
      currentClass().info.properties.set(propertyMatch[1], { name: propertyMatch[1], line, type: inferType(propertyMatch[2], symbols) });
    }

    // Loop / catch bindings
    const forMatch = code.match(/^for\s+([A-Za-z_][A-Za-z0-9_]*)\s+in\b/);
    if (forMatch) symbols.variables.set(forMatch[1], { name: forMatch[1], line, type: 'Any' });

    const catchMatch = code.match(/^catch\s+([A-Za-z_][A-Za-z0-9_]*)\b/);
    if (catchMatch) symbols.variables.set(catchMatch[1], { name: catchMatch[1], line, type: 'Dictionary (Error)' });

    // Block open / close
    const opener = code.match(/^(if|while|for|try)\b/);
    if (opener) blocks.push({ kind: opener[1] });
    if (/^end\b/.test(code)) blocks.pop();
  }

  return symbols;
}

// ---------------------------------------------------------------------------
// Autocomplete Provider
// ---------------------------------------------------------------------------

function buildBuiltinCompletions() {
  return Object.entries(BUILTIN_FUNCTIONS).map(([name, item]) => {
    const comp = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
    comp.detail = item.sig;
    comp.documentation = new vscode.MarkdownString(item.doc);
    comp.insertText = new vscode.SnippetString(`${name}(\${0})`);
    return comp;
  });
}

function buildVariableCompletions(symbols) {
  return [...symbols.variables.values()].sort((a, b) => a.name.localeCompare(b.name)).map(variable => {
    const item = new vscode.CompletionItem(variable.name, vscode.CompletionItemKind.Variable);
    item.detail = `${variable.type} (line ${variable.line + 1})`;
    item.sortText = `0_${variable.name}`;
    return item;
  });
}

const completionProvider = {
  provideCompletionItems(document, position) {
    const linePrefix = document.lineAt(position).text.slice(0, position.character);
    const symbols = buildSymbols(document);

    // Module or Enum dot member completion (e.g. `file.`, `json.`, `Color.`)
    const dotMatch = linePrefix.match(/([A-Za-z_][A-Za-z0-9_]*)\.$/);
    if (dotMatch) {
      const target = dotMatch[1];

      // Standard Library Module
      if (MODULES[target]) {
        return Object.entries(MODULES[target]).map(([member, item]) => {
          const comp = new vscode.CompletionItem(member, typeof item === 'object' && item.sig ? vscode.CompletionItemKind.Method : vscode.CompletionItemKind.Field);
          comp.detail = item.sig || `${target}.${member}`;
          if (item.doc) comp.documentation = new vscode.MarkdownString(item.doc);
          if (typeof item === 'object' && item.sig && item.sig.includes('(')) {
            comp.insertText = new vscode.SnippetString(`${member}(\${0})`);
          }
          return comp;
        });
      }

      // Enum Members
      if (symbols.enums.has(target)) {
        const enumInfo = symbols.enums.get(target);
        return enumInfo.members.map(member => {
          const comp = new vscode.CompletionItem(member, vscode.CompletionItemKind.EnumMember);
          comp.detail = `enum ${target}.${member}`;
          return comp;
        });
      }

      // Class Static Members
      if (symbols.classes.has(target)) {
        const classInfo = symbols.classes.get(target);
        return [...classInfo.methods.values()].filter(m => m.static).map(method => {
          const comp = new vscode.CompletionItem(method.name, vscode.CompletionItemKind.Method);
          comp.detail = `static ${target}.${method.name}(${method.params.join(', ')})`;
          comp.insertText = new vscode.SnippetString(`${method.name}(\${0})`);
          return comp;
        });
      }

      // Instance Members (e.g. `self.`)
      if (target === 'self') {
        let currentClass = null;
        for (const info of symbols.classes.values()) {
          if (info.line <= position.line && (!currentClass || info.line > currentClass.line)) currentClass = info;
        }
        if (currentClass) {
          const methods = [...currentClass.methods.values()].filter(m => !m.static).map(m => {
            const p = m.params.filter(param => param !== 'self');
            const comp = new vscode.CompletionItem(m.name, vscode.CompletionItemKind.Method);
            comp.detail = `${currentClass.name}.${m.name}(${p.join(', ')})`;
            comp.insertText = new vscode.SnippetString(`${m.name}(\${0})`);
            return comp;
          });
          const props = [...currentClass.properties.values()].map(p => {
            const comp = new vscode.CompletionItem(p.name, vscode.CompletionItemKind.Field);
            comp.detail = `${currentClass.name}.${p.name}: ${p.type}`;
            return comp;
          });
          return [...methods, ...props];
        }
      }
    }

    // Top-level completions
    const functions = [...symbols.functions.values()].map(info => {
      const comp = new vscode.CompletionItem(info.name, vscode.CompletionItemKind.Function);
      comp.detail = `fn ${info.name}(${info.params.join(', ')})`;
      comp.insertText = new vscode.SnippetString(`${info.name}(\${0})`);
      return comp;
    });

    const classes = [...symbols.classes.values()].map(info => {
      const comp = new vscode.CompletionItem(info.name, vscode.CompletionItemKind.Class);
      comp.detail = `class ${info.name}${info.parent ? ` extends ${info.parent}` : ''}`;
      return comp;
    });

    const enums = [...symbols.enums.values()].map(info => {
      const comp = new vscode.CompletionItem(info.name, vscode.CompletionItemKind.Enum);
      comp.detail = `enum ${info.name} { ${info.members.join(', ')} }`;
      return comp;
    });

    const moduleCompletions = Object.keys(MODULES).map(mod => {
      const comp = new vscode.CompletionItem(mod, vscode.CompletionItemKind.Module);
      comp.detail = `NOVA Standard Module: ${mod}`;
      return comp;
    });

    return [
      ...buildVariableCompletions(symbols),
      ...functions,
      ...classes,
      ...enums,
      ...moduleCompletions,
      ...KEYWORD_COMPLETIONS,
      ...buildBuiltinCompletions()
    ];
  }
};

// ---------------------------------------------------------------------------
// Hover Provider
// ---------------------------------------------------------------------------

const hoverProvider = {
  provideHover(document, position) {
    const range = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
    if (!range) return null;
    const word = document.getText(range);
    const linePrefix = document.lineAt(position).text.slice(0, range.end.character);

    // Module method hover (e.g. `file.read`)
    const moduleMemberMatch = linePrefix.match(/([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)$/);
    if (moduleMemberMatch) {
      const mod = moduleMemberMatch[1];
      const member = moduleMemberMatch[2];
      if (MODULES[mod] && MODULES[mod][member]) {
        return new vscode.Hover(new vscode.MarkdownString(`\`\`\`nova\n${MODULES[mod][member].sig}\n\`\`\`\n\n${MODULES[mod][member].doc}`));
      }
    }

    const symbols = buildSymbols(document);

    if (MODULES[word]) {
      const members = Object.keys(MODULES[word]).map(m => `\`${m}\``).join(', ');
      return new vscode.Hover(new vscode.MarkdownString(`**Standard Module: \`${word}\`**\n\nMembers: ${members}`));
    }

    if (symbols.enums.has(word)) {
      const info = symbols.enums.get(word);
      return new vscode.Hover(new vscode.MarkdownString(`**enum \`${word}\`**\n\nMembers:\n${info.members.map(m => `- \`${m}\``).join('\n')}\n\nDeclared at line ${info.line + 1}`));
    }

    if (symbols.variables.has(word)) {
      const info = symbols.variables.get(word);
      return new vscode.Hover(new vscode.MarkdownString(`**let \`${word}\`** — \`${info.type}\`\n\nDeclared at line ${info.line + 1}`));
    }

    if (symbols.functions.has(word)) {
      const info = symbols.functions.get(word);
      return new vscode.Hover(new vscode.MarkdownString(`\`\`\`nova\nfn ${word}(${info.params.join(', ')})\n\`\`\`\n\nDeclared at line ${info.line + 1}`));
    }

    if (symbols.classes.has(word)) {
      const info = symbols.classes.get(word);
      const methods = [...info.methods.values()].map(m => `- \`${m.static ? 'static ' : ''}${m.name}(${m.params.join(', ')})\``).join('\n') || '- No methods';
      return new vscode.Hover(new vscode.MarkdownString(`**class \`${word}\`**${info.parent ? ` extends \`${info.parent}\`` : ''}\n\nMethods:\n${methods}\n\nDeclared at line ${info.line + 1}`));
    }

    if (BUILTIN_FUNCTIONS[word]) {
      const b = BUILTIN_FUNCTIONS[word];
      return new vscode.Hover(new vscode.MarkdownString(`\`\`\`nova\n${b.sig}\n\`\`\`\n\n${b.doc}`));
    }

    if (RESERVED_KEYWORDS.includes(word)) {
      return new vscode.Hover(new vscode.MarkdownString(`**\`${word}\`** — NOVA reserved keyword.`));
    }

    return null;
  }
};

// ---------------------------------------------------------------------------
// Definition Provider (Go to Definition)
// ---------------------------------------------------------------------------

const definitionProvider = {
  provideDefinition(document, position) {
    const range = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
    if (!range) return null;
    const word = document.getText(range);
    const symbols = buildSymbols(document);

    const found = symbols.variables.get(word) ||
      symbols.functions.get(word) ||
      symbols.classes.get(word) ||
      symbols.enums.get(word) ||
      [...symbols.classes.values()].map(info => info.methods.get(word)).find(Boolean);

    return found ? new vscode.Location(document.uri, new vscode.Position(found.line, 0)) : null;
  }
};

// ---------------------------------------------------------------------------
// Document Symbol Provider (Outline View)
// ---------------------------------------------------------------------------

const documentSymbolProvider = {
  provideDocumentSymbols(document) {
    const symbols = buildSymbols(document);
    const docSymbols = [];

    for (const [name, info] of symbols.enums.entries()) {
      const range = new vscode.Range(info.line, 0, info.line, 100);
      const sym = new vscode.DocumentSymbol(name, `enum (${info.members.length} members)`, vscode.SymbolKind.Enum, range, range);
      docSymbols.push(sym);
    }

    for (const [name, info] of symbols.classes.entries()) {
      const range = new vscode.Range(info.line, 0, info.line, 100);
      const sym = new vscode.DocumentSymbol(name, info.parent ? `extends ${info.parent}` : 'class', vscode.SymbolKind.Class, range, range);
      for (const method of info.methods.values()) {
        const mRange = new vscode.Range(method.line, 0, method.line, 100);
        sym.children.push(new vscode.DocumentSymbol(method.name, `(${method.params.join(', ')})`, vscode.SymbolKind.Method, mRange, mRange));
      }
      docSymbols.push(sym);
    }

    for (const [name, info] of symbols.functions.entries()) {
      const range = new vscode.Range(info.line, 0, info.line, 100);
      const sym = new vscode.DocumentSymbol(name, `(${info.params.join(', ')})`, vscode.SymbolKind.Function, range, range);
      docSymbols.push(sym);
    }

    return docSymbols;
  }
};

// ---------------------------------------------------------------------------
// Signature Help Provider (Parameter Hints)
// ---------------------------------------------------------------------------

const signatureHelpProvider = {
  provideSignatureHelp(document, position) {
    const line = document.lineAt(position).text.slice(0, position.character);
    const match = line.match(/([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)?)\s*\(([^)]*)$/);
    if (!match) return null;

    const callName = match[1];
    const argsString = match[2];
    const activeParameter = (argsString.match(/,/g) || []).length;

    let sigInfo = null;

    if (callName.includes('.')) {
      const [mod, member] = callName.split('.');
      if (MODULES[mod] && MODULES[mod][member]) {
        sigInfo = MODULES[mod][member];
      }
    } else if (BUILTIN_FUNCTIONS[callName]) {
      sigInfo = BUILTIN_FUNCTIONS[callName];
    } else {
      const symbols = buildSymbols(document);
      if (symbols.functions.has(callName)) {
        const fn = symbols.functions.get(callName);
        sigInfo = { sig: `fn ${fn.name}(${fn.params.join(', ')})`, doc: `User-defined function declared at line ${fn.line + 1}` };
      }
    }

    if (!sigInfo) return null;

    const help = new vscode.SignatureHelp();
    const signature = new vscode.SignatureInformation(sigInfo.sig, new vscode.MarkdownString(sigInfo.doc));
    help.signatures = [signature];
    help.activeSignature = 0;
    help.activeParameter = activeParameter;

    return help;
  }
};

// ---------------------------------------------------------------------------
// Activation & Runtime Runner
// ---------------------------------------------------------------------------

function findNovaRuntime(config) {
  const custom = config.get('runtime.path');
  if (custom && fs.existsSync(custom)) return custom;

  const candidates = [
    path.join(process.env.LOCALAPPDATA || '', 'Programs', 'Nova', 'nova.exe'),
    'C:\\Nova\\build\\nova.exe',
    'C:\\Nova\\language\\nova.exe'
  ];

  for (const p of candidates) {
    if (p && fs.existsSync(p)) return p;
  }

  return 'nova'; // Fallback to PATH
}

function activate(context) {
  const NOVA = { language: 'nova' };

  context.subscriptions.push(diagnosticCollection);
  context.subscriptions.push(vscode.languages.registerCompletionItemProvider(NOVA, completionProvider, '.', '(', ' '));
  context.subscriptions.push(vscode.languages.registerHoverProvider(NOVA, hoverProvider));
  context.subscriptions.push(vscode.languages.registerDefinitionProvider(NOVA, definitionProvider));
  context.subscriptions.push(vscode.languages.registerDocumentSymbolProvider(NOVA, documentSymbolProvider));
  context.subscriptions.push(vscode.languages.registerSignatureHelpProvider(NOVA, signatureHelpProvider, '(', ','));

  let novaTerminal;
  function getActiveNovaDocument() {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== 'nova') {
      vscode.window.showErrorMessage('NOVA: Please open a .nova file first.');
      return null;
    }
    return editor.document;
  }

  async function runActiveFile() {
    const document = getActiveNovaDocument();
    if (!document) return;
    if (document.isUntitled) {
      const saved = await document.save();
      if (!saved) return;
    }
    if (document.isDirty && !(await document.save())) return;

    lintDocument(document);

    const config = vscode.workspace.getConfiguration('nova');
    const executable = findNovaRuntime(config);

    if (!novaTerminal || novaTerminal.exitStatus !== undefined) {
      novaTerminal = vscode.window.createTerminal({
        name: 'NOVA Run',
        cwd: path.dirname(document.uri.fsPath)
      });
      context.subscriptions.push(novaTerminal);
    }

    if (config.get('runtime.clearTerminal', false)) {
      novaTerminal.sendText('Clear-Host', true);
    }

    novaTerminal.show(true);
    novaTerminal.sendText(`& "${executable}" "${document.uri.fsPath}"`, true);
  }

  context.subscriptions.push(vscode.commands.registerCommand('nova.runFile', runActiveFile));
  context.subscriptions.push(vscode.commands.registerCommand('nova.configureRuntime', async () => {
    const config = vscode.workspace.getConfiguration('nova');
    const value = await vscode.window.showInputBox({
      prompt: 'Path to nova.exe runtime',
      value: config.get('runtime.path') || 'C:\\Users\\ASUS\\AppData\\Local\\Programs\\Nova\\nova.exe'
    });
    if (value) {
      await config.update('runtime.path', value, vscode.ConfigurationTarget.Global);
      vscode.window.showInformationMessage(`NOVA: Runtime path set to ${value}`);
    }
  }));

  let timeout;
  const scheduleLint = (document) => {
    if (document.languageId !== 'nova') return;
    const config = vscode.workspace.getConfiguration('nova');
    const delay = config.get('diagnostics.delay', 300);
    clearTimeout(timeout);
    timeout = setTimeout(() => lintDocument(document), delay);
  };

  vscode.workspace.textDocuments.forEach(doc => {
    if (doc.languageId === 'nova') lintDocument(doc);
  });

  context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(scheduleLint));
  context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(e => scheduleLint(e.document)));
  context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(lintDocument));
  context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(doc => diagnosticCollection.delete(doc.uri)));
}

function deactivate() {
  diagnosticCollection.clear();
  diagnosticCollection.dispose();
}

module.exports = { activate, deactivate };
