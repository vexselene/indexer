# Generate data for benchmarking
import os
import random

words = [
    "apple", "banana", "cherry", "date", "elderberry", "fig", "grape",
    "honeydew", "kiwi", "lemon", "mango", "nectarine", "orange", "papaya",
    "quince", "raspberry", "strawberry", "tangerine", "watermelon",
    "algorithm", "binary", "cache", "database", "encryption", "framework",
    "graph", "hash", "iterator", "json", "kernel", "linkedlist", "memory",
    "network", "operator", "pointer", "queue", "recursion", "stack", "thread"
]

prefixes = [
    "notes", "draft", "final", "backup", "todo", "summary", "report",
    "analysis", "review", "project", "research", "meeting", "budget",
    "proposal", "specification", "roadmap", "outline", "minutes", "slides"
]

subdirs = ["work", "personal", "archive", "drafts", "reference", "misc"]

os.makedirs("benchmark_data", exist_ok=True)
for subdir in subdirs:
    os.makedirs(f"benchmark_data/{subdir}", exist_ok=True)

for i in range(200):
    prefix = random.choice(prefixes)
    word1 = random.choice(words)
    word2 = random.choice(words)
    filename = f"{prefix}_{word1}_{word2}_{i}.txt"
    
    # 30% nested in subdirectory, 70% top-level
    if random.random() < 0.3:
        subdir = random.choice(subdirs)
        filepath = f"benchmark_data/{subdir}/{filename}"
    else:
        filepath = f"benchmark_data/{filename}"
    
    # 20-200 words per file, some words repeated for frequency testing
    content_words = random.choices(words, k=random.randint(20, 200))
    content = " ".join(content_words)
    
    with open(filepath, "w") as f:
        f.write(content)

print("Generated 200 files in benchmark_data/")
print(f"Top-level: {len(os.listdir('benchmark_data')) - len(subdirs)} files + {len(subdirs)} subdirs")
for subdir in subdirs:
    count = len(os.listdir(f"benchmark_data/{subdir}"))
    print(f"  {subdir}/: {count} files")