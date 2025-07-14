import re
import matplotlib
matplotlib.use('Agg')  # 无图形界面兼容
import matplotlib.pyplot as plt
import os
import struct
import sys

def check_csr_bin_structure(file_path):
    if not os.path.isfile(file_path):
        print(f"❌ 错误：文件 '{file_path}' 不存在。")
        return None, None, None

    file_size = os.path.getsize(file_path)
    print(f"📁 文件大小: {file_size} 字节")

    with open(file_path, "rb") as f:
        try:
            n, m = struct.unpack("II", f.read(8))
        except Exception as e:
            print("❌ 无法读取前8字节 (n 和 m)：", e)
            return None, None, None

        print(f"📌 读取到的节点数 n: {n}")
        print(f"📌 读取到的边数   m: {m}")

        expected_size = 8 + 4 * n + 4 * m
        expected_size_alt = 8 + 4 * (n + 1) + 4 * m

        if file_size == expected_size:
            print("✅ 文件结构匹配 CSR 格式（row_ptr 长度为 n）")
            row_ptr_len = n
        elif file_size == expected_size_alt:
            print("✅ 文件结构匹配 CSR 格式（row_ptr 长度为 n+1）")
            row_ptr_len = n + 1
        else:
            print("❌ 文件结构不匹配 CSR 格式。请检查 n/m 是否正确。")
            return None, None, None

    return n, m, row_ptr_len

def analyze_graph(file_path, n, m, row_ptr_len):
    with open(file_path, 'rb') as f:
        f.seek(8)  # 跳过 n 和 m
        row_ptr = list(struct.unpack(f'{row_ptr_len}I', f.read(4 * row_ptr_len)))

        if row_ptr_len == n:
            degrees = [row_ptr[i + 1] - row_ptr[i] for i in range(n - 1)]
            degrees.append(m - row_ptr[-1])
        else:
            degrees = [row_ptr[i + 1] - row_ptr[i] for i in range(n)]

        total_degree = sum(degrees)
        max_degree = max(degrees)
        min_degree = min(deg for deg in degrees if deg > 0)
        zero_degree_count = degrees.count(0)
        average_degree = total_degree / n

        print("\n📊 图统计分析：")
        print(f"  📎 节点总数         : {n}")
        print(f"  🔗 边总数           : {m}")
        print(f"  📈 平均出度         : {average_degree:.2f}")
        print(f"  🚀 最大出度         : {max_degree}")
        print(f"  🐜 最小非零出度     : {min_degree}")
        print(f"  ⚠️  出度为 0 的节点数 : {zero_degree_count}")

        # === 出度分布图 ===
        plt.figure(figsize=(8, 6))
        plt.hist(degrees, bins=1000, color='skyblue', edgecolor='black')
        plt.title("Degree Distribution")
        plt.xlabel("Out-Degree")
        plt.ylabel("Number of Nodes")
        plt.grid(axis='y', linestyle='--', alpha=0.7)

        output_path = os.path.join(os.path.dirname(file_path), "degree_histogram.png")
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"🖼️  出度分布图已保存为: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("用法: python analyze_csr_graph.py <文件路径>")
        sys.exit(1)

    file_path = sys.argv[1]
    n, m, row_ptr_len = check_csr_bin_structure(file_path)
    if n is not None:
        analyze_graph(file_path, n, m, row_ptr_len)
