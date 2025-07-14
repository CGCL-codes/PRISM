# PRISM: Triangle and Subgraph Counting on PIM Platform

PRISM is a scalable subgraph counting framework designed for the UPMEM **Processing-In-Memory (PIM)** architecture. It currently supports efficient **triangle counting**, and is extensible to other subgraph patterns.

The goal of PRISM is to accelerate subgraph counting tasks on large-scale graphs using UPMEM DPUs. It integrates **asynchronous pipelining**, **bitmap-based set intersection**, and other optimizations tailored for near-data processing architecture.

## 🚀 Key Features

- Fast triangle counting for large-scale sparse graphs.
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

- Linux or WSL environment.
- UPMEM SDK: **upmem-2023.2.0**.
- GNU Make, C compiler (e.g., `gcc`).
- Python ≥ 3.6 (for analysis scripts).

## ⚙️ Build and Run Instructions

To match a pattern within a graph, run the following command:

```bash
make clean
GRAPH=<graph_name> PATTERN=<pattern_name> make test
```

Example:

``` bash
make GRAPH=AM0505 PATTERN=CLIQUE3
```

The available values for GRAPH and PATTERN are defined in common.h. To add new graphs or patterns, modify common.h and recompile.

## 📥 Input Format

PRISM accepts input graphs in binary CSR (Compressed Sparse Row) format.

To convert Edge List to CSR Binary, use python_tool/adjtsv2csrbin.py to convert TSV/TXT edge lists into binary CSR. Run the following command:

``` bash
python tools/adjtsv2csrbin.py input.tsv --output graph.bin --header 1
```

Features:

Skips header line (if specified).
Ignores edge weights (only node pairs are used).
Outputs node_num, edge_num, row_ptr[], col_idx[] in binary.

## 📊 Profiling & Visualization Tools

### python_tool/analyze_csr_graph.py

Analyze CSR binary and generate graph statistics:

``` bash
python tools/analyze_csr_graph.py input/graph.bin
```

Outputs:

Number of nodes and edges.
Degree statistics (avg/max/min).
Histogram saved as degree_histogram.png.

### python_tool/show_cycle.py

Visualize DPU cycle usage and workload distribution:

``` bash
python tools/show_cycle.py result.txt
```

Left: Max cycle per DPU.
Right: Root task count per DPU.
Output: PNG image.

### python_tool/show_run_cycle.py

Compare cycle usage before and after optimization:

``` bash
python tools/show_run_cycle.py original.txt optimized.txt
```

Left: Operation/memory cycle stack bars.
Right: Root task count.
Output: Named PNG image (auto-generated)
