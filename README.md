# Indexer — Local File Search Engine

A from-scratch local file search engine written in C++ with no external search libraries.
Indexes filenames and file contents, ranks results using TF-IDF scoring, and persists
the index to disk across sessions.

## Features

- Walks a directory recursively and indexes all files
- Filename search via a **Trie** (exact + prefix matching)
- Full-text content search via an **Inverted Index**
- Ranked results using **TF-IDF** weighted scoring
- **Incremental updates** — detects added, modified, and deleted files on restart
- **Binary serialization** — index persists to `.indxr_db`, no full rebuild needed
- **Daemon mode** — runs as a background process, exposes search over a Unix domain socket
- Thread-safe reads via `std::shared_mutex`
- Interactive CLI with `:q` to save and quit

## Build & Run

```bash
make

./bin/indxr                  # indexes ./data by default
./bin/indxr /path/to/dir     # custom directory

./bin/indxrd /path/to/dir    # run as daemon (Unix socket at /tmp/indexer.sock)
./bin/benchmark /path/to/dir # run benchmarks
```

## CLI Commands

```
<query>     search for files
:q / :Q     quit and save index
:help       show help
```

## Project Structure

```
include/
├── file_registery.h      # File registry + metadata
├── prefix_tree.h         # Trie for filename indexing
├── inverted_index.h      # Inverted index for content search
├── search_engine.h       # Unified search, scoring, persistence
└── tokenizer.h           # Tokenizer
src/
├── file_registery.cpp
├── prefix_tree.cpp
├── inverted_index.cpp
├── search_engine.cpp
├── tokenizer.cpp
├── main.cpp              # Interactive CLI
├── daemon.cpp            # Unix socket daemon
└── benchmark.cpp         # Benchmark harness
data/                     # Sample files for testing
Makefile
```

## How It Works

**Indexing**

Files are registered with metadata (size, last modified). Filenames are tokenized
and inserted into a Trie. File contents (`.txt`, `.md`, `.csv`, `.log`) are tokenized
and inserted into an inverted index mapping each token to the files that contain it
and its frequency.

**Searching**

A query is tokenized and searched across both indexes. Scores are accumulated per file:

| Match type | Score |
|---|---|
| Exact filename token match | +10 |
| Prefix filename match | +5 |
| Content match | `freq × IDF × 10` |

Results are sorted by score descending.

**Persistence**

On quit, the index is serialized to `.indxr_db` in the indexed directory using a
custom binary format with tagged sections (`METADATA`, `REGISTRY`, `PREFIX__`, `INVERTED`).
On next run, the index is loaded and only changed files are re-indexed.

**Daemon mode**

`indxrd` builds the index then listens on `/tmp/indexer.sock` (Unix domain socket).
Clients send a query string and receive a JSON response. The companion HTTP server
connects to this socket and exposes search via a `/search` route.

## Design Decisions

| Decision | Why |
|---|---|
| Trie for filename index | O(m) lookup and natural prefix search |
| `unordered_map` for inverted index | O(1) token lookup |
| TF-IDF for content scoring | Penalizes common words, rewards rare matches |
| BFS order for Trie serialization | Root is always index 0, clean pointer reconstruction on load |
| Unix domain socket for daemon | Local IPC — faster than TCP, no port needed |
| `shared_mutex` | Multiple concurrent readers, exclusive writer |
| Single `tokenize()` with boolean flag | Reused for filenames (alpha only) and content (alphanumeric) |

## Future Work

Fuzzy search (edit distance / dynamic programming).
