# Indexor — Local File Search Engine

A from-scratch file search engine written in C++ that indexes files and
provides ranked full-text search through an interactive CLI.

## What It Does

- Walks a directory and indexes all files
- Tokenizes filenames and file contents
- Builds a filename index and an inverted index
- Ranks search results using weighted scoring
- Provides an interactive search interface

## Build & Run

```bash
make
./bin/indxr              # indexes ./data by default
./bin/indxr /path/to/dir # custom directory
```

## Project Structure

```
include/
├── file_registery.h      # File registry + metadata struct
├── file_name_index.h     # Filename token index
├── inverted_index.h      # Inverted index (content search)
├── search_engine.h       # Unified search + scoring
└── tokenizer.h           # Tokenization engine
src/
├── file_registery.cpp
├── file_name_index.cpp
├── inverted_index.cpp
├── search_engine.cpp
├── tokenizer.cpp
└── main.cpp              # Entry point + CLI loop
data/                     # Sample files for testing
Makefile
```

## How It Works

1. **Indexing** — Files are registered and tokenized. Filenames go into a
   lookup index. File contents go into an inverted index mapping each word
   to the files that contain it and how often.
2. **Searching** — A query is tokenized and searched across both indexes.
   Results are scored (filename matches weight higher) and ranked.
3. **Display** — Ranked results show file names and paths.

## Design Decisions

| Decision | Why |
|----------|-----|
| `unordered_map` for inverted index | O(1) token lookup |
| `unordered_map` → `vector` → sort | Fast accumulation, then sort once |
| Filename match ×10, content ×3 | Filename matches signal strong relevance |
| Single `tokenize()` with boolean flag | Cleaner than two separate functions |

## What's Next

- [x] Multi-token query support
- [ ] Prefix matching using Trie
- [ ] Stop words filtering
- [ ] Index persistence (save/load to disk)
- [ ] HTTP API via custom socket server (stretch)

## Why This Project

Built to deepen understanding of data structures, file I/O, and search
engine architecture in C++. Every component is written from scratch —
no external search libraries.
