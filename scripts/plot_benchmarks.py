#!/usr/bin/env python3
"""
Publication-quality PDF benchmark plotting and summary reporting script for dynamicArray.
Generates:
1. Vector PDF charts (*.pdf) with white backgrounds, clear axes, ticks, and numeric scales.
2. Formatted Markdown summary tables with exact numbers and speedups from Cirrus HPC.
"""

import json
import math
import os
import sys

def load_benchmark_data(filepath):
    with open(filepath, "r") as f:
        return json.load(f)

def extract_benchmarks(data):
    results = {}
    has_aggregates = any("_mean" in b["name"] for b in data.get("benchmarks", []))

    for bench in data.get("benchmarks", []):
        raw_name = bench["name"]

        # If aggregates exist (from Cirrus --benchmark_report_aggregates_only=true), extract _mean
        if has_aggregates:
            if not raw_name.endswith("_mean"):
                continue
            clean_name = raw_name[:-5]
        else:
            if any(s in raw_name for s in ("_mean", "_median", "_stddev", "_cv")):
                continue
            clean_name = raw_name

        parts = clean_name.split("/")
        suite_name = parts[0]
        arg = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else 0
        cpu_time = bench.get("cpu_time", bench.get("real_time", 0))

        if suite_name not in results:
            results[suite_name] = []
        results[suite_name].append((arg, cpu_time))

    for k in results:
        results[k].sort(key=lambda x: x[0])
    return results

def format_x_label(val):
    val = int(val)
    if val >= 1024 * 1024:
        mb = val / (1024 * 1024)
        return f"{int(mb)}M" if val % (1024 * 1024) == 0 else f"{mb:.1f}M"
    if val >= 1024:
        kb = val / 1024
        return f"{int(kb)}K" if val % 1024 == 0 else f"{kb:.1f}K"
    return str(val)

def format_y_label(val):
    if val >= 1_000_000_000:
        return f"{val/1_000_000_000:.1f} s"
    if val >= 1_000_000:
        return f"{val/1_000_000:.1f} ms" if val >= 10_000_000 else f"{val/1_000_000:.2f} ms"
    if val >= 1_000:
        return f"{val/1_000:.0f} us" if val >= 10_000 else f"{val/1_000:.1f} us"
    return f"{int(val)} ns"

def plot_with_matplotlib(title, x_title, y_title, series_dict, out_pdf_path, x_log=True, y_log=True):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker

    plt.figure(figsize=(9.5, 5.5))
    ax = plt.gca()
    ax.set_facecolor("#ffffff")
    plt.gcf().patch.set_facecolor("#ffffff")

    palette = [
        ("#1d4ed8", "o", "-"),   # Blue
        ("#dc2626", "s", "--"),  # Red
        ("#16a34a", "^", "-."),  # Green
        ("#9333ea", "D", ":"),   # Purple
        ("#ea580c", "v", "-"),   # Orange
    ]

    for i, (name, pts) in enumerate(series_dict.items()):
        if not pts:
            continue
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        color, marker, line_style = palette[i % len(palette)]
        ax.plot(xs, ys, label=name, color=color, marker=marker, linestyle=line_style,
                linewidth=2.2, markersize=6, alpha=0.95)

    if x_log:
        ax.set_xscale("log", base=2)
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, pos: format_x_label(x)))
    if y_log:
        ax.set_yscale("log")
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y, pos: format_y_label(y)))
    else:
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y, pos: format_y_label(y)))

    ax.grid(True, which="both", linestyle="--", linewidth=0.6, alpha=0.7, color="#d1d5db")
    ax.tick_params(axis="both", which="major", labelsize=10.5, color="#111827")

    plt.title(title, fontsize=13.5, fontweight="bold", pad=12, color="#111827")
    plt.xlabel(x_title, fontsize=11.5, labelpad=9, color="#111827", fontweight="600")
    plt.ylabel(y_title, fontsize=11.5, labelpad=9, color="#111827", fontweight="600")

    plt.legend(frameon=True, facecolor="#f9fafb", edgecolor="#e5e7eb", fontsize=9.5, loc="best")
    plt.tight_layout()

    os.makedirs(os.path.dirname(out_pdf_path), exist_ok=True)
    plt.savefig(out_pdf_path, format="pdf", bbox_inches="tight")
    plt.close()
    print(f"[PDF Generated via Matplotlib] {out_pdf_path}")

class VectorPDF:
    """Pure-Python vector PDF generator (zero-dependency fallback)."""
    def __init__(self, width=820, height=480):
        self.width = width
        self.height = height
        self.content = []

    def rect(self, x, y, w, h, fill_rgb=None, stroke_rgb=None, line_width=1):
        cmds = []
        if fill_rgb:
            cmds.append(f"{fill_rgb[0]:.3f} {fill_rgb[1]:.3f} {fill_rgb[2]:.3f} rg")
        if stroke_rgb:
            cmds.append(f"{stroke_rgb[0]:.3f} {stroke_rgb[1]:.3f} {stroke_rgb[2]:.3f} RG")
            cmds.append(f"{line_width:.2f} w")
        cmds.append(f"{x:.2f} {y:.2f} {w:.2f} {h:.2f} re")
        if fill_rgb and stroke_rgb:
            cmds.append("B")
        elif fill_rgb:
            cmds.append("f")
        elif stroke_rgb:
            cmds.append("S")
        self.content.append("\n".join(cmds))

    def line(self, x1, y1, x2, y2, stroke_rgb=(0,0,0), line_width=1, dash=None):
        cmds = [f"{stroke_rgb[0]:.3f} {stroke_rgb[1]:.3f} {stroke_rgb[2]:.3f} RG", f"{line_width:.2f} w"]
        if dash:
            cmds.append(f"[{dash[0]} {dash[1]}] 0 d")
        else:
            cmds.append("[] 0 d")
        cmds.append(f"{x1:.2f} {y1:.2f} m {x2:.2f} {y2:.2f} l S")
        self.content.append("\n".join(cmds))

    def polyline(self, points, stroke_rgb=(0,0,0), line_width=2):
        if not points:
            return
        cmds = [
            f"{stroke_rgb[0]:.3f} {stroke_rgb[1]:.3f} {stroke_rgb[2]:.3f} RG",
            f"{line_width:.2f} w",
            "[] 0 d",
            f"{points[0][0]:.2f} {points[0][1]:.2f} m"
        ]
        for p in points[1:]:
            cmds.append(f"{p[0]:.2f} {p[1]:.2f} l")
        cmds.append("S")
        self.content.append("\n".join(cmds))

    def circle(self, cx, cy, r, fill_rgb=(0,0,0), stroke_rgb=None):
        k = 0.5522847498 * r
        cmds = []
        if fill_rgb:
            cmds.append(f"{fill_rgb[0]:.3f} {fill_rgb[1]:.3f} {fill_rgb[2]:.3f} rg")
        if stroke_rgb:
            cmds.append(f"{stroke_rgb[0]:.3f} {stroke_rgb[1]:.3f} {stroke_rgb[2]:.3f} RG 1 w")
        cmds.append(f"{cx+r:.2f} {cy:.2f} m")
        cmds.append(f"{cx+r:.2f} {cy+k:.2f} {cx+k:.2f} {cy+r:.2f} {cx:.2f} {cy+r:.2f} c")
        cmds.append(f"{cx-k:.2f} {cy+r:.2f} {cx-r:.2f} {cy+k:.2f} {cx-r:.2f} {cy:.2f} c")
        cmds.append(f"{cx-r:.2f} {cy-k:.2f} {cx-k:.2f} {cy-r:.2f} {cx:.2f} {cy-r:.2f} c")
        cmds.append(f"{cx+k:.2f} {cy-r:.2f} {cx+r:.2f} {cy-k:.2f} {cx+r:.2f} {cy:.2f} c")
        if fill_rgb and stroke_rgb:
            cmds.append("B")
        elif fill_rgb:
            cmds.append("f")
        else:
            cmds.append("S")
        self.content.append("\n".join(cmds))

    def text(self, x, y, string, font_size=10, fill_rgb=(0,0,0), font="/F1", align="left"):
        escaped = string.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")
        approx_w = len(string) * font_size * 0.52
        tx = x
        if align == "center":
            tx = x - approx_w / 2
        elif align == "right":
            tx = x - approx_w
        cmd = f"BT {font} {font_size} Tf {fill_rgb[0]:.3f} {fill_rgb[1]:.3f} {fill_rgb[2]:.3f} rg 1 0 0 1 {tx:.2f} {y:.2f} Tm ({escaped}) Tj ET"
        self.content.append(cmd)

    def text_vertical(self, x, y, string, font_size=10, fill_rgb=(0,0,0), font="/F1", align="center"):
        escaped = string.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")
        approx_w = len(string) * font_size * 0.52
        offset = -approx_w / 2 if align == "center" else (-approx_w if align == "right" else 0)
        # Coordinate transform rotating 90 degrees CCW: [0 1 -1 0 tx ty]
        cmd = f"q 0 1 -1 0 {x:.2f} {y:.2f} cm BT {font} {font_size} Tf {fill_rgb[0]:.3f} {fill_rgb[1]:.3f} {fill_rgb[2]:.3f} rg 1 0 0 1 {offset:.2f} 0 Tm ({escaped}) Tj ET Q"
        self.content.append(cmd)

    def write_to_file(self, filepath):
        stream_data = "\n".join(self.content).encode("latin1")
        length = len(stream_data)

        pdf = bytearray()
        pdf.extend(b"%PDF-1.4\n")
        offsets = []

        def add_obj(data):
            offsets.append(len(pdf))
            obj_num = len(offsets)
            pdf.extend(f"{obj_num} 0 obj\n".encode("latin1"))
            pdf.extend(data)
            pdf.extend(b"\nendobj\n")
            return obj_num

        add_obj(b"<< /Type /Catalog /Pages 2 0 R >>")
        add_obj(b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>")
        add_obj(f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {self.width} {self.height}] /Contents 4 0 R /Resources << /Font << /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> /F2 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >> >> >> >>".encode("latin1"))
        add_obj(f"<< /Length {length} >>\nstream\n".encode("latin1") + stream_data + b"\nendstream")

        xref_pos = len(pdf)
        pdf.extend(f"xref\n0 {len(offsets)+1}\n0000000000 65535 f \n".encode("latin1"))
        for off in offsets:
            pdf.extend(f"{off:010d} 00000 n \n".encode("latin1"))
        pdf.extend(f"trailer\n<< /Size {len(offsets)+1} /Root 1 0 R >>\nstartxref\n{xref_pos}\n%%EOF\n".encode("latin1"))

        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        with open(filepath, "wb") as f:
            f.write(pdf)
        print(f"[PDF Generated via VectorPDF] {filepath}")

def plot_with_vector_pdf(title, x_title, y_title, series_dict, out_pdf_path, x_log=True, y_log=True):
    width, height = 820, 480
    margin = {"top": 55, "right": 240, "bottom": 60, "left": 85}
    plot_w = width - margin["left"] - margin["right"]
    plot_h = height - margin["top"] - margin["bottom"]

    all_x = []
    all_y = []
    for pts in series_dict.values():
        for x, y in pts:
            if x > 0 and y > 0:
                all_x.append(x)
                all_y.append(y)

    if not all_x or not all_y:
        return

    raw_min_x, raw_max_x = min(all_x), max(all_x)
    raw_min_y, raw_max_y = min(all_y), max(all_y)

    if x_log:
        min_exp_x = int(math.floor(math.log2(raw_min_x)))
        max_exp_x = int(math.ceil(math.log2(raw_max_x)))
        step = 2 if (max_exp_x - min_exp_x) >= 8 else 1
        x_ticks = [2**e for e in range(min_exp_x, max_exp_x + 1, step)]
        plot_min_x = x_ticks[0]
        plot_max_x = x_ticks[-1]
    else:
        x_ticks = sorted(list(set(all_x)))
        plot_min_x = min(x_ticks)
        plot_max_x = max(x_ticks)

    if y_log:
        min_exp_y = int(math.floor(math.log10(raw_min_y)))
        max_exp_y = int(math.ceil(math.log10(raw_max_y)))
        y_ticks = [10**e for e in range(min_exp_y, max_exp_y + 1)]
        plot_min_y = y_ticks[0]
        plot_max_y = y_ticks[-1]
    else:
        plot_min_y = 0
        plot_max_y = raw_max_y * 1.15
        y_ticks = [plot_max_y * (i / 5.0) for i in range(6)]

    def scale_x(val):
        if x_log:
            t = (math.log2(val) - math.log2(plot_min_x)) / (math.log2(plot_max_x) - math.log2(plot_min_x) or 1)
        else:
            t = (val - plot_min_x) / (plot_max_x - plot_min_x or 1)
        return margin["left"] + t * plot_w

    def scale_y(val):
        if y_log:
            t = (math.log10(val) - math.log10(plot_min_y)) / (math.log10(plot_max_y) - math.log10(plot_min_y) or 1)
        else:
            t = (val - plot_min_y) / (plot_max_y - plot_min_y or 1)
        return margin["bottom"] + t * plot_h

    pdf = VectorPDF(width, height)
    # White background
    pdf.rect(0, 0, width, height, fill_rgb=(1, 1, 1))
    # Plot frame
    pdf.rect(margin["left"], margin["bottom"], plot_w, plot_h, fill_rgb=(0.99, 0.99, 0.995), stroke_rgb=(0.85, 0.88, 0.91), line_width=1.2)

    # Title & Axis labels
    pdf.text(margin["left"], height - 35, title, font_size=13, font="/F2", fill_rgb=(0.08, 0.11, 0.16))
    pdf.text(margin["left"] + plot_w / 2, 22, x_title, font_size=10, font="/F2", fill_rgb=(0.28, 0.33, 0.41), align="center")
    
    # Vertically rotated Y-axis label centered on Y axis
    pdf.text_vertical(24, margin["bottom"] + plot_h / 2, y_title, font_size=10, font="/F2", fill_rgb=(0.28, 0.33, 0.41), align="center")

    # Grid & Y ticks
    for y_val in y_ticks:
        py = scale_y(y_val)
        if margin["bottom"] <= py <= margin["bottom"] + plot_h:
            pdf.line(margin["left"], py, margin["left"] + plot_w, py, stroke_rgb=(0.89, 0.91, 0.94), line_width=0.8, dash=(3, 3))
            pdf.line(margin["left"] - 5, py, margin["left"], py, stroke_rgb=(0.58, 0.64, 0.72), line_width=1)
            pdf.text(margin["left"] - 8, py - 3, format_y_label(y_val), font_size=9, font="/F1", fill_rgb=(0.28, 0.33, 0.41), align="right")

    # Grid & X ticks
    for x_val in x_ticks:
        px = scale_x(x_val)
        if margin["left"] <= px <= margin["left"] + plot_w:
            pdf.line(px, margin["bottom"], px, margin["bottom"] + plot_h, stroke_rgb=(0.89, 0.91, 0.94), line_width=0.8, dash=(3, 3))
            pdf.line(px, margin["bottom"] - 5, px, margin["bottom"], stroke_rgb=(0.58, 0.64, 0.72), line_width=1)
            pdf.text(px, margin["bottom"] - 16, format_x_label(x_val), font_size=9, font="/F1", fill_rgb=(0.28, 0.33, 0.41), align="center")

    colors = [
        (0.11, 0.31, 0.85),  # Blue
        (0.86, 0.15, 0.15),  # Red
        (0.08, 0.63, 0.24),  # Green
        (0.58, 0.20, 0.92),  # Purple
        (0.92, 0.35, 0.05),  # Orange
    ]

    leg_x = margin["left"] + plot_w + 14
    leg_y = margin["bottom"] + plot_h - 10
    leg_w = margin["right"] - 22
    leg_h = len(series_dict) * 26 + 14
    pdf.rect(leg_x, leg_y - leg_h, leg_w, leg_h, fill_rgb=(0.97, 0.98, 0.99), stroke_rgb=(0.88, 0.90, 0.93), line_width=1)

    for i, (name, pts) in enumerate(series_dict.items()):
        if not pts:
            continue
        c = colors[i % len(colors)]
        screen_pts = [(scale_x(x), scale_y(y)) for x, y in pts]

        pdf.polyline(screen_pts, stroke_rgb=c, line_width=2.2)
        for px, py in screen_pts:
            pdf.circle(px, py, 3.5, fill_rgb=c)

        entry_y = leg_y - 20 - i * 26
        pdf.line(leg_x + 8, entry_y + 3, leg_x + 24, entry_y + 3, stroke_rgb=c, line_width=2.2)
        pdf.circle(leg_x + 16, entry_y + 3, 3, fill_rgb=c)
        pdf.text(leg_x + 28, entry_y, name, font_size=8.5, font="/F1", fill_rgb=(0.12, 0.16, 0.23))

    pdf.write_to_file(out_pdf_path)

def render_plot_pdf(title, x_title, y_title, series_dict, out_pdf_path, x_log=True, y_log=True):
    try:
        plot_with_matplotlib(title, x_title, y_title, series_dict, out_pdf_path, x_log=x_log, y_log=y_log)
    except Exception as e:
        print(f"[Notice] Matplotlib not used ({e}), rendering via native VectorPDF engine...")
        plot_with_vector_pdf(title, x_title, y_title, series_dict, out_pdf_path, x_log=x_log, y_log=y_log)

def generate_markdown_summary(results, output_path):
    lines = [
        "# Cirrus HPC Benchmark Results & Performance Analysis\n",
        "Benchmarked on a dedicated compute node (`cs-n0004`, 576 CPUs, 3.75 GHz, 32 MB L3 Cache) on the **Cirrus Supercomputer (EPCC)**. Pinned to core 0 via `taskset` to eliminate thread scheduling jitter and frequency scaling noise.\n",
        "## 1. Container PushBack Throughput (Time per full sequence)\n",
        "| Elements (N) | dynamicArray (Geometric) | std::vector (Geometric) | Speedup vs STL | dynamicArray (Reserved) | std::vector (Reserved) |",
        "| :--- | :--- | :--- | :--- | :--- | :--- |"
    ]

    da_geo = dict(results.get("BM_DynamicArray_PushBack_Geometric", []))
    stl_geo = dict(results.get("BM_StdVector_PushBack_Geometric", []))
    da_res = dict(results.get("BM_DynamicArray_PushBack_Reserved", []))
    stl_res = dict(results.get("BM_StdVector_PushBack_Reserved", []))

    for n in sorted(da_geo.keys()):
        d_g = da_geo.get(n, 0)
        s_g = stl_geo.get(n, 0)
        d_r = da_res.get(n, 0)
        s_r = stl_res.get(n, 0)
        is_faster = f"**{s_g / d_g:.2f}x (FASTER)**" if d_g < s_g else f"{s_g / d_g:.2f}x"
        lines.append(f"| {n:,} | {d_g:,.1f} ns | {s_g:,.1f} ns | {is_faster} | {d_r:,.1f} ns | {s_r:,.1f} ns |")

    lines.extend([
        "\n## 2. Iteration & std::sort Performance (Validating 3-Pointer & Contiguous Iterators)\n",
        "| Elements (N) | dynamicArray Iteration | std::vector Iteration | dynamicArray std::sort | std::vector std::sort |",
        "| :--- | :--- | :--- | :--- | :--- |"
    ])

    da_iter = dict(results.get("BM_DynamicArray_Iteration", []))
    stl_iter = dict(results.get("BM_StdVector_Iteration", []))
    da_sort = dict(results.get("BM_DynamicArray_Sort", []))
    stl_sort = dict(results.get("BM_StdVector_Sort", []))

    for n in sorted(da_iter.keys()):
        d_i = da_iter.get(n, 0)
        s_i = stl_iter.get(n, 0)
        d_s = da_sort.get(n, 0)
        s_s = stl_sort.get(n, 0)
        sort_str = f"{d_s:,.1f} ns | {s_s:,.1f} ns" if d_s > 0 else "N/A | N/A"
        lines.append(f"| {n:,} | {d_i:,.1f} ns | {s_i:,.1f} ns | {sort_str} |")

    lines.extend([
        "\n## 3. Allocator Impact: std::allocator (Heap malloc) vs ArenaAllocator (0-Syscall Bump)\n",
        "| Cycle Elements | std::allocator (malloc/free) | ArenaAllocator (Bump + Reset) | Speedup (Latency Win) |",
        "| :--- | :--- | :--- | :--- |"
    ])

    h_data = dict(results.get("BM_Allocator_StdHeap", []))
    a_data = dict(results.get("BM_Allocator_MonotonicArena", []))
    for n in sorted(h_data.keys()):
        h_time = h_data.get(n, 0)
        a_time = a_data.get(n, 0)
        speedup = f"**{h_time / a_time:.2f}x faster**" if a_time > 0 else "N/A"
        lines.append(f"| {n:,} pushes | {h_time:,.1f} ns | {a_time:,.1f} ns | {speedup} |")

    lines.extend([
        "\n## 4. The Boolean Specialization (Full Traversal Scan)\n",
        "| Total Booleans | dynamicArray<uint8_t> (Byte) | dynamicArray<bool> (1-bit Packed) | std::vector<bool> (STL) |",
        "| :--- | :--- | :--- | :--- |"
    ])

    b_byte = dict(results.get("BM_Bool_ByteArray_Scan", []))
    b_bit = dict(results.get("BM_Bool_BitPacked_Scan", []))
    s_bit = dict(results.get("BM_StdVector_Bool_Scan", []))
    for n in sorted(b_byte.keys()):
        by = b_byte.get(n, 0)
        bi = b_bit.get(n, 0)
        sb = s_bit.get(n, 0)
        lines.append(f"| {n:,} | {by:,.1f} ns | {bi:,.1f} ns | {sb:,.1f} ns |")

    with open(output_path, "w") as f:
        f.write("\n".join(lines))
    print(f"[Summary Generated] {output_path}")

def main():
    json_path = sys.argv[1] if len(sys.argv) > 1 else "benchmark_results_cirrus.json"
    if not os.path.exists(json_path):
        print(f"Error: {json_path} not found.")
        sys.exit(1)

    data = load_benchmark_data(json_path)
    results = extract_benchmarks(data)

    chart_dir = "benchmarks/charts"
    os.makedirs(chart_dir, exist_ok=True)

    # 1. Throughput PDF Chart
    render_plot_pdf(
        "PushBack Throughput: dynamicArray vs std::vector (Cirrus HPC)",
        "Sequence Size (N Elements)",
        "Total Latency (log scale)",
        {
            "dynamicArray (Geometric)": results.get("BM_DynamicArray_PushBack_Geometric", []),
            "std::vector (Geometric)": results.get("BM_StdVector_PushBack_Geometric", []),
            "dynamicArray (Reserved)": results.get("BM_DynamicArray_PushBack_Reserved", []),
            "std::vector (Reserved)": results.get("BM_StdVector_PushBack_Reserved", []),
        },
        f"{chart_dir}/throughput_comparison.pdf",
        x_log=True, y_log=True
    )

    # 2. Allocator Latency PDF Chart
    render_plot_pdf(
        "Order Cycle Latency: Heap Allocator vs Monotonic Arena (Cirrus HPC)",
        "Elements Allocated Per Cycle (N)",
        "Cycle Time (ns)",
        {
            "std::allocator (Heap malloc/free)": results.get("BM_Allocator_StdHeap", []),
            "ArenaAllocator (0-Syscall Bump)": results.get("BM_Allocator_MonotonicArena", []),
        },
        f"{chart_dir}/allocator_latency.pdf",
        x_log=True, y_log=False
    )

    # 3. Boolean Specialization PDF Chart
    render_plot_pdf(
        "Boolean Traversal Latency: Byte vs Bit-Packed Specialization (Cirrus HPC)",
        "Total Boolean Elements (N)",
        "Scan Time (log scale)",
        {
            "dynamicArray<uint8_t> (Byte Array)": results.get("BM_Bool_ByteArray_Scan", []),
            "dynamicArray<bool> (Bit-Packed Proxy)": results.get("BM_Bool_BitPacked_Scan", []),
            "std::vector<bool> (STL Baseline)": results.get("BM_StdVector_Bool_Scan", []),
        },
        f"{chart_dir}/bool_crossover.pdf",
        x_log=True, y_log=True
    )

    # 4. Iteration & Sort Comparison PDF Chart
    render_plot_pdf(
        "Iteration & std::sort Throughput: dynamicArray vs std::vector (Cirrus HPC)",
        "Elements (N)",
        "Execution Time (log scale)",
        {
            "dynamicArray (std::sort)": results.get("BM_DynamicArray_Sort", []),
            "std::vector (std::sort)": results.get("BM_StdVector_Sort", []),
            "dynamicArray (Sequential Iteration)": results.get("BM_DynamicArray_Iteration", []),
            "std::vector (Sequential Iteration)": results.get("BM_StdVector_Iteration", []),
        },
        f"{chart_dir}/algorithms_comparison.pdf",
        x_log=True, y_log=True
    )

    # Generate Markdown Summary
    generate_markdown_summary(results, "benchmarks/BENCHMARK_RESULTS.md")

if __name__ == "__main__":
    main()
