import struct
import argparse
import os
from collections import defaultdict

def convert_edge_list_to_custom_csr(input_file: str, output_file: str, header_lines=1):
    """
    将边表文件转换为定制的CSR格式二进制文件，忽略每行的第三列（权重值）
    """
    graph = defaultdict(list)
    max_node_id = 0

    # 读取边表，构建邻接表
    with open(input_file, 'r') as fin:
        for _ in range(header_lines):
            next(fin)
        for line in fin:
            try:
                parts = line.strip().split()
                if len(parts) < 2:
                    raise ValueError("行数据不足")
                src, dst = int(parts[0]), int(parts[1])  # 忽略 value 列
                graph[src].append(dst)
                max_node_id = max(max_node_id, src, dst)
            except ValueError:
                print(f"跳过无效行: {line.strip()}")
                continue

    n = max_node_id + 1
    row_ptr = [0] * n
    col_idx = []

    for node in range(n):
        neighbors = sorted(graph[node])
        row_ptr[node] = len(col_idx)
        col_idx.extend(neighbors)

    m = len(col_idx)
    avg_degree = m / n if n != 0 else 0.0

    with open(output_file, 'wb') as fout:
        fout.write(struct.pack('I', n))
        fout.write(struct.pack('I', m))
        fout.write(struct.pack(f'{n}I', *row_ptr))
        fout.write(struct.pack(f'{m}I', *col_idx))

    print(f"✅ 转换完成！输出文件: {output_file}")
    print(f"📌 节点数 (n): {n}")
    print(f"📌 边数 (m): {m}")
    print(f"📎 row_ptr (len={len(row_ptr)}): {row_ptr[:10]} ...")
    print(f"📎 col_idx (len={len(col_idx)}): {col_idx[:10]} ...")
    print(f"📊 平均节点度数: {avg_degree:.2f}")

def main():
    parser = argparse.ArgumentParser(description='将边表文件转换为定制CSR格式')
    parser.add_argument('input', help='输入文本文件路径')
    parser.add_argument('--output', help='输出二进制文件路径（可选，默认为输入路径替换.tsv为.bin）')
    parser.add_argument('--header', type=int, default=1, help='需要跳过的文件头行数(默认=1)')
    args = parser.parse_args()

    # 自动推导输出路径（如果未指定）
    if args.output is None:
        if args.input.endswith('.tsv') or args.input.endswith('.txt'):
            args.output = args.input[:-4] + '.bin'
        else:
            args.output = args.input + '.bin'

    convert_edge_list_to_custom_csr(args.input, args.output, args.header)

if __name__ == '__main__':
    main()
