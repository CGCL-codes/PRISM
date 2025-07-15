# PRISM: Practical In-Memory Acceleration for Subgraph Matching at Scale

PRISM is a scalable subgraph counting framework designed for the UPMEM **Processing-In-Memory (PIM)** architecture. It currently supports efficient **triangle counting**, and is extensible to other subgraph patterns.

The goal of PRISM is to accelerate subgraph counting tasks on large-scale graphs using UPMEM DPUs. It integrates **asynchronous pipelining**, **bitmap-based set intersection**, and other optimizations tailored for near-data processing architecture.

## 🚀 Key Features

- Fast triangle counting for large-scale sparse graphs.
- skew-aware workload distribution strategy for balanced execution across thousands of DPU
- Asynchronous loader-worker pipeline using WRAM FIFO.
- Bitmap-based set intersection acceleration on DPUs.
- Lightweight performance profiling and cycle analysis tools.
- CSR (Compressed Sparse Row) binary input format for fast loading.

## 📁 Directory Structure

PRISM/  
├── host/ # Host-side logic (C)  
├── dpu/ # DPU-side programs (C for UPMEM)  
├── python_tool/ # Python scripts for preprocessing and profiling  
├── include/ # Shared headers  
├── makefile # Compilation rules  
└── README.md # Project description  

## 🛠 Requirements

- Linux environment.
- UPMEM SDK: **upmem-v2025.1.0.**.
- GNU Make, C compiler (e.g., `gcc`).
- Python ≥ 3.8 (for analysis scripts).

## ⚙️ Build and Run Instructions

To match a pattern within a graph, run the following command:

```bash
make clean
GRAPH=<graph_name> PATTERN=<pattern_name> make test
```

Example:

``` bash
GRAPH=AM0312 PATTERN=CLIQUE3 make test
```

The available values for GRAPH and PATTERN are defined in common.h. To add new graphs or patterns, modify common.h and recompile.

## 📥 Input Format

PRISM accepts input graphs in binary CSR (Compressed Sparse Row) format.

To convert Edge List to CSR Binary, use python_tool/adjtsv2csrbin.py to convert TSV/TXT edge lists into binary CSR. Run the following command:

``` bash
python  python_tool/adjtsv2csrbin.py input.tsv --output graph.bin --header 1
```

Features:

- Skips header line (if specified).
- Ignores edge weights (only node pairs are used).
- Outputs node_num(4bytes), edge_num(4bytes), row_ptr[](node_num*4bytes), col_idx[](edge_num*4bytes) in binary.

Example:

```bash
mkdir -p ./data 
wget https://graphchallenge.s3.amazonaws.com/snap/amazon0312/amazon0312_adj.tsv 
python3 python_tool/adjtsv2csrbin.py amazon0312_adj.tsv
mv amazon0312_adj.bin ./data
GRAPH=AM0312 PATTERN=CLIQUE3 make test
```

## 📊 Profiling & Visualization Tools

### python_tool/analyze_csr_graph.py

Analyze CSR binary and generate graph statistics:

``` bash
python3 python_tool/analyze_csr_graph.py input/graph.bin
```

Outputs:

- Number of nodes and edges.
- Degree statistics (avg/max/min).

### python_tool/show_cycle.py

Visualize DPU cycle usage and workload distribution:

``` bash
python3 python_tool/show_cycle.py result.txt
```

- Left: Max cycle per DPU.
- Right: Root task count per DPU.
