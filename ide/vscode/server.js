/* 
 * PyLua Advanced Language Server with Deep Introspection
 */

const {
    createConnection,
    TextDocuments,
    ProposedFeatures,
    CompletionItemKind,
    TextDocumentSyncKind,
} = require('vscode-languageserver/node');

const {
    TextDocument
} = require('vscode-languageserver-textdocument');

const { execSync } = require('child_process');

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

// CONFIG
const PYLUA_PATH = 'C:\\Users\\s28\\Documents\\lua\\cmake-build-debug\\bin\\pylua.exe';
const INTRO_SCRIPT = 'C:\\Users\\s28\\Documents\\lua\\ide\\introspection.lua';

// Caches
const libraryCache = new Map();
const documentMetadata = new Map(); // Store metadata (aliases, classes, etc.)

connection.onInitialize((params) => {
    return {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Incremental,
            completionProvider: {
                resolveProvider: true,
                triggerCharacters: ['.', ':', '\"', '\'']
            }
        }
    };
});

/**
 * Runs the Pylua interpreter to introspect a library or expression
 */
function introspect(expr) {
    if (libraryCache.has(expr)) return libraryCache.get(expr);

    try {
        const cmd = `"${PYLUA_PATH}" "${INTRO_SCRIPT}" "${expr || ''}"`;
        const output = execSync(cmd, { encoding: 'utf8', timeout: 3000 });
        const symbols = output.split(/\r?\n/).map(line => {
            const parts = line.trim().split(':');
            if (parts.length < 2) return null;
            const [label, type] = parts;
            let kind = CompletionItemKind.Field;
            if (type === 'function') kind = CompletionItemKind.Method;
            if (type === 'table') kind = CompletionItemKind.Module;
            return { label, kind, detail: `(${type})` };
        }).filter(s => s !== null);

        // Cache small results
        if (symbols.length > 0) libraryCache.set(expr, symbols);
        return symbols;
    } catch (e) {
        return [];
    }
}

documents.onDidChangeContent(change => {
    analyzeDocument(change.document);
});

/**
 * Deep Analysis: Track aliases and symbols
 */
function analyzeDocument(textDocument) {
    const text = textDocument.getText();
    const metadata = {
        symbols: [],
        aliases: new Map() // varName -> target (e.g. req -> request)
    };

    // Track Classes
    let match;
    const classRegex = /\bclass\s+(\w+)/g;
    while ((match = classRegex.exec(text))) {
        metadata.symbols.push({ label: match[1], kind: CompletionItemKind.Class });
    }

    // Track Enums
    const emuRegex = /\bemu\s+(\w+)/g;
    while ((match = emuRegex.exec(text))) {
        metadata.symbols.push({ label: match[1], kind: CompletionItemKind.Enum });
    }

    // Track Variables and Aliases
    // Pattern: local x = y
    const aliasRegex = /\b(?:local\s+)?(\w+)\s*=\s*([\w.]+)/g;
    while ((match = aliasRegex.exec(text))) {
        const varName = match[1];
        const valName = match[2];
        if (!['class', 'emu', 'function', 'local', 'if', 'while', 'for', 'return', 'then', 'do', 'end'].includes(varName)) {
            metadata.symbols.push({ label: varName, kind: CompletionItemKind.Variable });
            // Record alias
            metadata.aliases.set(varName, valName);
        }
    }

    // Track instance creations: local v = Vec2:new()
    const instanceRegex = /\b(?:local\s+)?(\w+)\s*=\s*(\w+)[.:]new/g;
    while ((match = instanceRegex.exec(text))) {
        metadata.aliases.set(match[1], match[2]);
    }

    documentMetadata.set(textDocument.uri, metadata);
}

connection.onCompletion((params) => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return [];

    const text = doc.getText();
    const offset = doc.offsetAt(params.position);
    const beforeText = text.substring(0, offset);
    const metadata = documentMetadata.get(params.textDocument.uri);

    // Contextual Indexing: Track chains (e.g. a.b.c)
    const chainMatch = beforeText.match(/([\w.:]+)[.:]\s*$/);
    if (chainMatch) {
        let chain = chainMatch[1];

        // Resolve aliases in the chain
        // E.g. if 'req' is 'request', resolve 'req.get' -> 'request.get'
        const parts = chain.split(/[.:]/);
        if (metadata && metadata.aliases.has(parts[0])) {
            parts[0] = metadata.aliases.get(parts[0]);
            chain = parts.join('.');
        }

        // Query the interpreter for the resolved chain
        return introspect(chain);
    }

    // require support
    if (beforeText.match(/require\s*\(?\s*["']$/)) {
        return introspect('').filter(s => s.kind === CompletionItemKind.Module);
    }

    const localSymbols = (metadata ? metadata.symbols : []) || [];
    const staticCompletions = [
        { label: 'class', kind: CompletionItemKind.Keyword },
        { label: 'emu', kind: CompletionItemKind.Keyword },
        { label: 'request', kind: CompletionItemKind.Module },
        { label: 'math', kind: CompletionItemKind.Module }
    ];

    return [...staticCompletions, ...localSymbols];
});

documents.listen(connection);
connection.listen();
