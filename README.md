# Indexer — Local File Indexing and Search Service

A local file indexing and search service written in modern C++.

Indexer recursively scans directories, builds searchable indexes for filenames and file contents, persists them to disk, and serves search requests through a daemon process.

---

## Features

- Recursive directory indexing
- Trie-based filename search — exact and prefix matching
- Full-text content search via an Inverted Index
- TF-IDF based ranking
- Binary persistence using a single `.indxr_db` file
- Incremental indexing on startup — detects added, modified, and deleted files
- Real-time filesystem monitoring via `inotify`
- Automatic index updates while the daemon is running
- Unix domain socket daemon
- HTTP API integration
- Thread-safe access using `std::shared_mutex`
- Interactive CLI
- Benchmark suite

---

## Architecture

```
                HTTP Client
                     |
                     v
              HTTP Server
                     |
                     v
             Unix Domain Socket
                     |
                     v
              Indexer Daemon
                     |
      +--------------+--------------+
      |                             |
      v                             v
   FsHook                      Search Engine
 (inotify)                           |
                                    / \
                                   /   \
                                  v     v
                          Prefix Tree  Inverted Index
```

The search engine is transport-independent. Queries can come from the CLI, Unix socket clients, or the HTTP API without modifying the indexing logic.

---

## How It Works

### File Registration

Each file is registered with metadata:

```cpp
struct FileMetaData {
    int id;
    std::string name;
    std::string path;
    uintmax_t file_size;
    int64_t last_modified_ticks;
    bool index_content;
};
```

### Filename Index

Filenames are tokenized and inserted into a Trie. For example, `math_notes.txt` becomes the tokens `math`, `notes`, and `txt`, allowing both exact and prefix searches.

### Content Index

Files with the following extensions are tokenized and stored in an inverted index:

```
.txt  .md  .csv  .log
```

The index maps each token to the files containing it and their frequency:

```
matrix
 ├── file 3 -> 5
 └── file 8 -> 2
```

### Search Ranking

Scores are accumulated per file across all index sources:

| Match Type | Score |
|---|---|
| Exact filename token | +10 |
| Prefix filename token | +5 |
| Content match | TF-IDF weighted |

Results are sorted by descending score.

### Persistence

All data is written to `.indxr_db` using a custom binary format with tagged sections:

```
[METADATA] [REGISTRY] [PREFIX__] [INVERTED]
```

On startup the database is loaded, the filesystem is scanned, and only changed files are re-indexed.

### Live Updates

The daemon watches the indexed directory via Linux `inotify` through `FsHook`. New, modified, and deleted files are handled automatically while the daemon is running. Hidden files are ignored to prevent indexing internal database files.

---

## Build

```bash
make            # CLI  →  bin/indxr
make daemon     # Daemon  →  bin/indxrd
make benchmark  # Benchmark  →  bin/benchmark
```

---

## Usage

### CLI

```bash
make run
make run ARGS="/path/to/dir"
```

| Command | Description |
|---|---|
| `<query>` | Search files |
| `:q` | Save and quit |
| `:help` | Show help |

### Daemon

```bash
make run-daemon
make run-daemon ARGS="/path/to/dir"
```

The daemon listens on `/tmp/indexer.sock`.

### Querying the Daemon

```bash
./bin/indxrd &
echo "math" | nc -U /tmp/indexer.sock
```

Response:

```json
{
  "results": [
    {
      "id": 0,
      "name": "math_notes.txt",
      "path": "/home/user/docs/math_notes.txt",
      "score": 15
    }
  ]
}
```

### Benchmarks

```bash
make run-bench ARGS="./benchmark_data"
```

---

## Project Structure

```
include/
├── file_registery.h
├── prefix_tree.h
├── inverted_index.h
├── search_engine.h
├── fs_hook.h
└── tokenizer.h
src/
├── main.cpp
├── daemon.cpp
├── benchmark.cpp
├── file_registery.cpp
├── prefix_tree.cpp
├── inverted_index.cpp
├── search_engine.cpp
├── fs_hook.cpp
└── tokenizer.cpp
Makefile
```

---

## Design Decisions

| Decision | Reason |
|---|---|
| Trie for filename indexing | Efficient exact and prefix search |
| Inverted index for content search | O(1) token lookup |
| TF-IDF scoring | Penalizes common words, rewards rare matches |
| Single binary database | Faster startup and simpler persistence |
| Incremental indexing | Avoids full rebuilds |
| `inotify`-based file watching | Real-time updates without polling |
| Unix domain sockets | Efficient local IPC, no port needed |
| Separate daemon process | Decouples indexing from clients |
| `shared_mutex` | Concurrent readers with exclusive writer |

---

## Requirements

- C++20
- Linux
- make

Linux-specific: `inotify`, Unix domain sockets.

---

## Future Work

Fuzzy search (Levenshtein distance / BK-tree).

---

## License

MIT