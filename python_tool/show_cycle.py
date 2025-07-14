import re
import matplotlib
matplotlib.use('Agg')  # 无图形界面兼容
import matplotlib.pyplot as plt
import os
import sys

def parse_dpu_cycles(file_path):
    pattern = re.compile(r'DPU:\s*(\d+),\s*tasklet:\s*(\d+),\s*cycle:\s*(\d+),\s*root_num:\s*(\d+)')
    dpu_data = {}

    with open(file_path, 'r') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                dpu, tasklet, cycle, root_num = map(int, match.groups())
                if dpu not in dpu_data or cycle > dpu_data[dpu]['cycle']:
                    dpu_data[dpu] = {
                        'tasklet': tasklet,
                        'cycle': cycle,
                        'root_num': root_num
                    }

    return dpu_data

def plot_dpu_metrics_bar(dpu_data, output_path):
    dpus = sorted(dpu_data.keys())
    cycles = [dpu_data[dpu]['cycle'] for dpu in dpus]
    root_nums = [dpu_data[dpu]['root_num'] for dpu in dpus]

    max_cycle = max(cycles)
    max_dpu = dpus[cycles.index(max_cycle)]
    runtime_ms = max_cycle *2.85e-6  # 1 cycle = 2ns → 2e-3 ms

    plt.figure(figsize=(14, 6))

    # 柱状图1：Cycle
    plt.subplot(1, 2, 1)
    plt.bar(dpus, cycles, color='steelblue')
    plt.title("Max Cycle per DPU", fontsize=14)
    plt.xlabel("DPU ID", fontsize=12)
    plt.ylabel("Cycle Count", fontsize=12)
    plt.xticks(rotation=45, fontsize=8)

    # 注释最大值及其转换时间
    label_text = f"Max Cycle = {max_cycle} (DPU {max_dpu})\nEstimated Runtime ≈ {runtime_ms:.2f} ms"
    plt.text(0.05, 0.95, label_text, transform=plt.gca().transAxes,
             fontsize=10, verticalalignment='top', bbox=dict(facecolor='white', alpha=0.8))

    # 柱状图2：Root Num
    plt.subplot(1, 2, 2)
    plt.bar(dpus, root_nums, color='orange')
    plt.title("Root Num per DPU", fontsize=14)
    plt.xlabel("DPU ID", fontsize=12)
    plt.ylabel("Root Num", fontsize=12)
    plt.xticks(rotation=45, fontsize=8)

    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close()

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("用法: python show_cycle.py input_file.txt")
        sys.exit(1)

    input_file = sys.argv[1]
    if not os.path.isfile(input_file):
        print(f"错误：文件 '{input_file}' 不存在。")
        sys.exit(1)

    output_file = os.path.splitext(input_file)[0] + ".png"
    dpu_data = parse_dpu_cycles(input_file)
    plot_dpu_metrics_bar(dpu_data, output_file)
    print(f"图像已保存至: {output_file}")
