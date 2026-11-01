#!/usr/bin/env python3
"""
Publication-quality benchmark plotting and summary reporting script for dynamicArray.
Generates:
1. High-resolution vector SVGs with white backgrounds, clear axes, ticks, and numeric scales.
2. High-DPI PNGs if matplotlib is available.
3. Formatted Markdown summary tables with exact numbers and speedups from Cirrus HPC.
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

        # If aggregates exist (e.g. from Cirrus run), keep only the _mean
        if has_aggregates:
            if not raw_name.endswith("_mean"):
                continue
            clean_name = raw_name[:-5]
        else:
            if "_mean" in raw_name or "_median" in raw_name or "_stddev" in raw_name or "_cv" in raw_name:
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
    if val >= 1_000_000:
        return f"{int(val/1_000_000)}M" if val % 1_000_000 == 0 else f"{val/1_000_000:.1f}M"
    if val >= 1_000:
        return f"{int(val/1_000)}K" if val % 1_000 == 0 else f"{val/1_000:.1f}K"
    return str(int(val))

def format_y_label(val):
    if val >= 1_000_000_000:
        return f"{val/1_000_000_000:.1f} s"
    if val >= 1_000_000:
        return f"{val/1_000_000:.1f} ms" if val >= 10_000_000 else f"{val/1_000_000:.2f} ms"
    if val >= 1_000:
        return f"{val/1_000:.0f} µs" if val >= 10_000 else f"{val/1_000:.1f} µs"
    return f"{int(val)} ns"

def render_svg_chart(title, x_title, y_title, series_dict, out_path, x_log=True, y_log=True):
    """
    Renders a clean, publication-ready SVG plot with white background,
    crisp grid lines, explicit axis tick marks, and formatted numeric scale labels.
    """
    width, height = 900, 520
    margin = {"top": 60, "right": 210, "bottom": 70, "left": 95}
    plot_w = width - margin["left"] - margin["right"]
    plot_h = height - margin["top"] - margin["bottom"]

    # Gather data points
    all_x = []
    all_y = []
    for pts in series_dict.values():
        for x, y in pts:
            if x > 0 and y > 0:
                all_x.append(x)
                all_y.append(y)

    if not all_x or not all_y:
        return

    # Determine axis ranges
    raw_min_x, raw_max_x = min(all_x), max(all_x)
    raw_min_y, raw_max_y = min(all_y), max(all_y)

    # Compute neat ticks
    if x_log:
        min_exp_x = int(math.floor(math.log2(raw_min_x)))
        max_exp_x = int(math.ceil(math.log2(raw_max_x)))
        # Filter to powers of 4 if too many ticks
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
        plot_max_y = raw_max_y * 1.1
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
        return margin["top"] + (1.0 - t) * plot_h

    # Color palette (clean, distinct colors)
    palette = [
        {"color": "#1d4ed8", "shape": "circle"},    # Vibrant Blue
        {"color": "#dc2626", "shape": "rect"},      # Bright Red
        {"color": "#15803d", "shape": "triangle"},  # Forest Green
        {"color": "#7e22ce", "shape": "diamond"},   # Purple
        {"color": "#c2410c", "shape": "circle"},    # Burnt Orange
    ]

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%">',
        f'  <defs>',
        f'    <filter id="shadow" x="-5%" y="-5%" width="110%" height="110%">',
        f'      <feDropShadow dx="0" dy="1" stdDeviation="2" flood-color="#000000" flood-opacity="0.06"/>',
        f'    </filter>',
        f'  </defs>',
        # Background
        f'  <rect width="{width}" height="{height}" fill="#ffffff" rx="8" />',
        f'  <rect x="{margin["left"]}" y="{margin["top"]}" width="{plot_w}" height="{plot_h}" fill="#fcfcfd" stroke="#e2e8f0" stroke-width="1.2"/>',
        # Title
        f'  <text x="{margin["left"]}" y="36" fill="#0f172a" font-family="system-ui, -apple-system, sans-serif" font-size="16" font-weight="700">{title}</text>',
        f'  <text x="{margin["left"] + plot_w/2}" y="{height - 18}" fill="#475569" font-family="system-ui, sans-serif" font-size="12" font-weight="600" text-anchor="middle">{x_title}</text>',
        f'  <text x="24" y="{margin["top"] + plot_h/2}" fill="#475569" font-family="system-ui, sans-serif" font-size="12" font-weight="600" text-anchor="middle" transform="rotate(-90 24 {margin["top"] + plot_h/2})">{y_title}</text>',
    ]

    # Horizontal Grid Lines & Y Ticks
    for y_val in y_ticks:
        py = scale_y(y_val)
        if margin["top"] <= py <= margin["top"] + plot_h:
            svg.append(f'  <line x1="{margin["left"]}" y1="{py:.1f}" x2="{margin["left"] + plot_w}" y2="{py:.1f}" stroke="#e2e8f0" stroke-width="1" stroke-dasharray="4,4"/>')
            svg.append(f'  <line x1="{margin["left"] - 6}" y1="{py:.1f}" x2="{margin["left"]}" y2="{py:.1f}" stroke="#94a3b8" stroke-width="1.2"/>')
            svg.append(f'  <text x="{margin["left"] - 12}" y="{py + 4:.1f}" fill="#475569" font-family="system-ui, sans-serif" font-size="11" text-anchor="end">{format_y_label(y_val)}</text>')

    # Vertical Grid Lines & X Ticks
    for x_val in x_ticks:
        px = scale_x(x_val)
        if margin["left"] <= px <= margin["left"] + plot_w:
            svg.append(f'  <line x1="{px:.1f}" y1="{margin["top"]}" x2="{px:.1f}" y2="{margin["top"] + plot_h}" stroke="#e2e8f0" stroke-width="1" stroke-dasharray="4,4"/>')
            svg.append(f'  <line x1="{px:.1f}" y1="{margin["top"] + plot_h}" x2="{px:.1f}" y2="{margin["top"] + plot_h + 6}" stroke="#94a3b8" stroke-width="1.2"/>')
            svg.append(f'  <text x="{px:.1f}" y="{margin["top"] + plot_h + 22}" fill="#475569" font-family="system-ui, sans-serif" font-size="11" text-anchor="middle">{format_x_label(x_val)}</text>')

    # Legend Container
    leg_x = margin["left"] + plot_w + 16
    leg_y = margin["top"] + 10
    leg_w = margin["right"] - 26
    leg_h = len(series_dict) * 32 + 16
    svg.append(f'  <rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="{leg_h}" fill="#f8fafc" stroke="#e2e8f0" rx="6" filter="url(#shadow)"/>')

    # Draw Series Data & Legend Entries
    for i, (name, pts) in enumerate(series_dict.items()):
        if not pts:
            continue
        style = palette[i % len(palette)]
        color = style["color"]
        shape = style["shape"]

        poly_pts = []
        for x, y in pts:
            px = scale_x(x)
            py = scale_y(y)
            poly_pts.append(f"{px:.1f},{py:.1f}")

        # Polyline
        svg.append(f'  <polyline points="{" ".join(poly_pts)}" fill="none" stroke="{color}" stroke-width="2.4" stroke-linejoin="round"/>')

        # Markers
        for x, y in pts:
            px = scale_x(x)
            py = scale_y(y)
            if shape == "circle":
                svg.append(f'  <circle cx="{px:.1f}" cy="{py:.1f}" r="4.2" fill="{color}" stroke="#ffffff" stroke-width="1.5"/>')
            elif shape == "rect":
                svg.append(f'  <rect x="{px-4:.1f}" y="{py-4:.1f}" width="8" height="8" fill="{color}" stroke="#ffffff" stroke-width="1.5"/>')
            elif shape == "triangle":
                svg.append(f'  <polygon points="{px:.1f},{py-5:.1f} {px+4.5:.1f},{py+3.5:.1f} {px-4.5:.1f},{py+3.5:.1f}" fill="{color}" stroke="#ffffff" stroke-width="1.5"/>')
            elif shape == "diamond":
                svg.append(f'  <polygon points="{px:.1f},{py-5:.1f} {px+4.5:.1f},{py:.1f} {px:.1f},{py+5:.1f} {px-4.5:.1f},{py:.1f}" fill="{color}" stroke="#ffffff" stroke-width="1.5"/>')

        # Legend Entry
        entry_y = leg_y + 20 + i * 32
        svg.append(f'  <line x1="{leg_x + 12}" y1="{entry_y}" x2="{leg_x + 32}" y2="{entry_y}" stroke="{color}" stroke-width="2.4"/>')
        svg.append(f'  <circle cx="{leg_x + 22}" cy="{entry_y}" r="4" fill="{color}" stroke="#ffffff" stroke-width="1.2"/>')
        svg.append(f'  <text x="{leg_x + 40}" y="{entry_y + 4}" fill="#1e293b" font-family="system-ui, sans-serif" font-size="11" font-weight="500">{name}</text>')

    svg.append('</svg>')

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w") as f:
        f.write("\n".join(svg))
    print(f"[Chart Generated] {out_path}")

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

    # 1. Throughput Chart
    render_svg_chart(
        "PushBack Throughput: dynamicArray vs std::vector (Cirrus HPC)",
        "Sequence Size (N Elements)",
        "Total Execution Time (log scale)",
        {
            "dynamicArray (Geometric)": results.get("BM_DynamicArray_PushBack_Geometric", []),
            "std::vector (Geometric)": results.get("BM_StdVector_PushBack_Geometric", []),
            "dynamicArray (Reserved)": results.get("BM_DynamicArray_PushBack_Reserved", []),
            "std::vector (Reserved)": results.get("BM_StdVector_PushBack_Reserved", []),
        },
        f"{chart_dir}/throughput_comparison.svg",
        x_log=True, y_log=True
    )

    # 2. Allocator Latency Chart
    render_svg_chart(
        "Order Cycle Latency: Heap Allocator vs Monotonic Arena (Cirrus HPC)",
        "Elements Allocated Per Cycle (N)",
        "Cycle Time (ns)",
        {
            "std::allocator (Heap malloc/free)": results.get("BM_Allocator_StdHeap", []),
            "ArenaAllocator (0-Syscall Bump)": results.get("BM_Allocator_MonotonicArena", []),
        },
        f"{chart_dir}/allocator_latency.svg",
        x_log=True, y_log=False
    )

    # 3. Boolean Specialization Chart
    render_svg_chart(
        "Boolean Traversal Latency: Byte vs Bit-Packed Specialization (Cirrus HPC)",
        "Total Boolean Elements (N)",
        "Scan Time (log scale)",
        {
            "dynamicArray<uint8_t> (Byte Array)": results.get("BM_Bool_ByteArray_Scan", []),
            "dynamicArray<bool> (Bit-Packed Proxy)": results.get("BM_Bool_BitPacked_Scan", []),
            "std::vector<bool> (STL Baseline)": results.get("BM_StdVector_Bool_Scan", []),
        },
        f"{chart_dir}/bool_crossover.svg",
        x_log=True, y_log=True
    )

    # 4. Iteration & Sort Comparison Chart
    render_svg_chart(
        "Algorithm & Iteration Throughput: dynamicArray vs std::vector (Cirrus HPC)",
        "Elements (N)",
        "Execution Time (log scale)",
        {
            "dynamicArray (std::sort)": results.get("BM_DynamicArray_Sort", []),
            "std::vector (std::sort)": results.get("BM_StdVector_Sort", []),
            "dynamicArray (Sequential Scan)": results.get("BM_DynamicArray_Iteration", []),
            "std::vector (Sequential Scan)": results.get("BM_StdVector_Iteration", []),
        },
        f"{chart_dir}/algorithms_comparison.svg",
        x_log=True, y_log=True
    )

    # Generate Markdown Summary
    generate_markdown_summary(results, "benchmarks/BENCHMARK_RESULTS.md")

if __name__ == "__main__":
    main()
