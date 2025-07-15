# PRISM: Practical In-Memory Acceleration for Subgraph Matching at Scale

**PRISM** is a scalable subgraph counting framework designed for the UPMEM **Processing-In-Memory (PIM)** architecture. It currently supports efficient **triangle counting**, and is extensible to other subgraph patterns.

The goal of PRISM is to accelerate subgraph counting tasks on large-scale graphs using UPMEM DPUs. It integrates **asynchronous pipelining**, **bitmap-based set intersection**, and other optimizations tailored for near-data processing architecture.

---

## 🚀 Key Features

- Fast triangle counting for large-scale sparse graphs.
- Skew-aware workload distribution for balanced execution across thousands of DPUs.
- Asynchronous loader–worker pipeline using WRAM FIFO.
- Bitmap-based set intersection acceleration on DPUs.
- Lightweight performance profiling and cycle analysis tools.
- CSR (Compressed Sparse Row) binary input format for fast loading.

---

## 📁 Directory Structure

```
PRISM/
├── host/         # Host-side logic (C)
├── dpu/          # DPU-side programs (C for UPMEM)
├── python_tool/  # Python scripts for preprocessing and profiling
├── include/      # Shared headers
├── makefile      # Compilation rules
└── README.md     # Project description
```

---

## 🛠 Requirements

- **Linux environment**
- **UPMEM SDK v2025.1.0.**
- **GNU Make, C compiler (e.g., `gcc`)**
- **Python ≥ 3.8 (for analysis scripts)**

---

## ⚙️ Build and Run Instructions

To match a pattern within a graph, run:

```bash
make clean
GRAPH=<graph_name> PATTERN=<pattern_name> make test
```

Example:

```bash
GRAPH=AM0312 PATTERN=CLIQUE3 make test
```

> 💡 The available values for `GRAPH` and `PATTERN` are defined in `include/common.h`.  
> To add new graphs or patterns, modify `common.h` and recompile.

---

## 📥 Input Format

PRISM accepts input graphs in binary CSR (Compressed Sparse Row) format.

To convert an edge list into CSR binary format, use `python_tool/adjtsv2csrbin.py`:

```bash
python3 python_tool/adjtsv2csrbin.py input.tsv --output graph.bin --header 1
```

- Skips header line (if specified).
- Ignores edge weights (only node pairs are used).
- Outputs the following binary layout:
  - `node_num (4 bytes)`
  - `edge_num (4 bytes)`
  - `row_ptr[] (node_num × 4 bytes)`
  - `col_idx[] (edge_num × 4 bytes)`

Example:

```bash
mkdir -p ./data 
wget https://graphchallenge.s3.amazonaws.com/snap/amazon0312/amazon0312_adj.tsv 
python3 python_tool/adjtsv2csrbin.py amazon0312_adj.tsv
mv amazon0312_adj.bin ./data
GRAPH=AM0312 PATTERN=CLIQUE3 make test
```

---

## 🧩 Customized Graphs and Matching Patterns

PRISM supports flexible definitions of graph inputs and matching patterns.

All configuration entries are defined in `include/common.h`.

### ➕ Adding Custom Graphs

1. Place your input graph (in CSR binary format) into the `./data/` directory.
2. Add a macro definition in `include/common.h`:

    ```c
    #if defined(AM0312)
    #define DATA_NAME "amazon0312_adj"
    #define N (1<<20)
    #define M (1<<23)
    #endif
    ```

3. Build and test:

    ```bash
    GRAPH=AM0312 PATTERN=CLIQUE3 make test
    ```

### ➕ Adding Custom Patterns

1. Define a new macro for your pattern kernel in include/common.h

    ```c
    #elif defined(TELE5)
    #define KERNEL_FUNC tele5
    #define PATTERN_NAME "tele5"
    #endif
    ```

2. Implement the kernel function in dpu/ directory (e.g., in TELE5.c or new source file).
3. Build and run:

    ```bash
    GRAPH=AM0312 PATTERN=TELE5 make test
    ```

---

## 📈 Scalability Testing

PRISM is designed to scale from hundreds to tens of thousands of DPUs.

### 🔧 Custom DPU Count

To run PRISM on a specific number of DPUs:

```bash
GRAPH=AM0312 PATTERN=CLIQUE3 EXTRA_FLAGS="-DV_NR_DPUS=5120" make test
```

### 📊 Full Scalability Sweep

To automatically benchmark PRISM from **640** to **40,960** DPUs:

```bash
GRAPH=AM0312 PATTERN=CLIQUE3 make test_sc
```

This script:

- Compiles PRISM with various DPU counts.
- Runs the benchmark for each configuration.

---

## 📊 Profiling & Visualization Tools

### `analyze_csr_graph.py`

Analyzes CSR binary and outputs graph statistics:

```bash
python3 python_tool/analyze_csr_graph.py input/graph.bin
```

Outputs include:

- Number of nodes and edges
- Degree distribution (min/avg/max)

---

### `show_cycle.py`

Visualizes DPU-level workload distribution:

```bash
python3 python_tool/show_cycle.py result.txt
```

- Left plot: Max cycle per DPU
- Right plot: Task count per DPU

---
