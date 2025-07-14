import re
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import sys
from datetime import datetime

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

def get_unique_filename(base_path):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base_name, ext = os.path.splitext(base_path)
    counter = 1
    while True:
        if counter == 1:
            new_path = f"{base_name}_{timestamp}{ext}"
        else:
            new_path = f"{base_name}_{timestamp}_{counter}{ext}"
        if not os.path.exists(new_path):
            return new_path
        counter += 1

def plot_dpu_comparison(dpu_data_orig, dpu_data_opt, output_path):
    import numpy as np

    all_dpus = sorted(set(dpu_data_orig.keys()) | set(dpu_data_opt.keys()))
    x = np.arange(len(all_dpus))

    orig_cycles = [dpu_data_orig.get(dpu, {'cycle': 0})['cycle'] for dpu in all_dpus]
    opt_cycles = [dpu_data_opt.get(dpu, {'cycle': 0})['cycle'] for dpu in all_dpus]
    reduced_cycles = [max(o - n, 0) for o, n in zip(orig_cycles, opt_cycles)]
    root_nums = [dpu_data_orig.get(dpu, {'root_num': 0})['root_num'] for dpu in all_dpus]


    max_cycle = max(orig_cycles)
    avg_opt_cycles = sum(opt_cycles) / len(opt_cycles) if opt_cycles else 0
    avg_reduced_cycles = sum(reduced_cycles) / len(reduced_cycles) if reduced_cycles else 0
    max_dpu = all_dpus[orig_cycles.index(max_cycle)]
    runtime_ms = max_cycle * 2.85e-6
    opt_runtime_ms = avg_opt_cycles * 2.85e-6
    reduced_runtime_ms = avg_reduced_cycles * 2.85e-6


    plt.figure(figsize=(14, 6))

    # 左图 堆叠柱状图
    plt.subplot(1, 2, 1)
    plt.bar(x, reduced_cycles, color='salmon', label='Operation Cycle')
    plt.bar(x, opt_cycles, bottom=reduced_cycles, color='skyblue', label='Memory Access Cycle')

    plt.title("DPU Cycles: Operation vs Memory Access", fontsize=14)
    plt.xlabel("DPU ID", fontsize=12)
    plt.ylabel("Cycle Count", fontsize=12)

    # 控制 x 轴标签数量，显示6-7个左右
    n_labels = 7
    step = max(1, len(all_dpus)//n_labels)
    xticks_labels = [str(all_dpus[i]) if i % step == 0 else "" for i in range(len(all_dpus))]
    plt.xticks(x, xticks_labels, rotation=45, fontsize=8)

    # 去掉x轴底部黑线（spines）
    ax = plt.gca()
    ax.spines['bottom'].set_visible(False)

    plt.legend(fontsize=10)
    label_text = f"Max Original Cycle = {max_cycle} (DPU {max_dpu})\nEstimated Runtime ≈ {runtime_ms:.2f} ms \nargv memory access Runtime ≈ {opt_runtime_ms:.2f} ms \nargv intersection Runtime ≈ {reduced_runtime_ms:.2f} ms"
    plt.text(0.05, 0.95, label_text, transform=ax.transAxes,
             fontsize=10, verticalalignment='top', bbox=dict(facecolor='white', alpha=0.8))

    # 右图 Root Num
    plt.subplot(1, 2, 2)
    plt.bar(all_dpus, root_nums, color='orange')
    plt.title("Root Num per DPU", fontsize=14)
    plt.xlabel("DPU ID", fontsize=12)
    plt.ylabel("Root Num", fontsize=12)
    plt.xticks(x, xticks_labels, rotation=45, fontsize=8)

    ax2 = plt.gca()
    ax2.spines['bottom'].set_visible(False)

    plt.tight_layout()
    unique_path = get_unique_filename(output_path)
    plt.savefig(unique_path, dpi=300)
    plt.close()
    return unique_path

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python compare_dpu_cycles.py original.txt optimized.txt")
        sys.exit(1)

    file_orig = sys.argv[1]
    file_opt = sys.argv[2]

    if not os.path.isfile(file_orig) or not os.path.isfile(file_opt):
        print("Error: One of the input files does not exist.")
        sys.exit(1)

    dpu_data_orig = parse_dpu_cycles(file_orig)
    dpu_data_opt = parse_dpu_cycles(file_opt)

    base_output = os.path.splitext(file_orig)[0] + "_compare.png"
    saved_path = plot_dpu_comparison(dpu_data_orig, dpu_data_opt, base_output)
    print(f"Comparison image saved to: {saved_path}")
