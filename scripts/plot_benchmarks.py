#!/usr/bin/env python3
"""
Benchmark visualization and reporting script for dynamicArray.
Parses Google Benchmark JSON output and generates:
1. Native SVG charts (zero-dependency, renders crystal-clear on GitHub)
2. Formatted Markdown summary tables with speedup metrics
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
    for bench in data.get("benchmarks", []):
        raw_name = bench["name"]
        # Skip aggregates like mean/median if present unless specified
        if "_mean" in raw_name or "_median" in raw_name or "_stddev" in raw_name:
            continue
        parts = raw_name.split("/")
        suite_name = parts[0]
        arg = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else 0
        cpu_time = bench.get("cpu_time", bench.get("real_time", 0))

        if suite_name not in results:
            results[suite_name] = []
        results[suite_name].append((arg, cpu_time))

    for k in results:
        results[k].sort(key=lambda x: x[0])
    return results

def make_svg_line_chart(title, x_label, y_label, series_dict, filename):
    """Generate a clean, standalone SVG line chart with a dark/light compatible modern palette."""
    width, height = 800, 480
    margin = {"top": 60, "right": 160, "bottom": 60, "left": 80}
    plot_w = width - margin["left"] - margin["right"]
    plot_h = height - margin["top"] - margin["bottom"]

    # Gather bounds
    all_x = []
    all_y = []
    for s_name, points in series_dict.items():
        for x, y in points:
            if x > 0 and y > 0:
                all_x.append(x)
                all_y.append(y)

    if not all_x or not all_y:
        return

    min_x, max_x = min(all_x), max(all_x)
    min_y, max_y = min(all_y), max(all_y)

    log_x = (max_x / min_x >= 16)
    log_y = (max_y / min_y >= 16)

    def scale_x(val):
        if log_x:
            t = (math.log10(val) - math.log10(min_x)) / (math.log10(max_x) - math.log10(min_x) or 1)
        else:
            t = (val - min_x) / (max_x - min_x or 1)
        return margin["left"] + t * plot_w

    def scale_y(val):
        if log_y:
            t = (math.log10(val) - math.log10(min_y)) / (math.log10(max_y) - math.log10(min_y) or 1)
        else:
            t = (val - min_y) / (max_y - min_y or 1)
        return margin["top"] + (1.0 - t) * plot_h

    colors = ["#2563eb", "#dc2626", "#16a34a", "#9333ea", "#ea580c"]
    
    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%">',
        f'<rect width="{width}" height="{height}" fill="#0d1117" rx="8" />',
        f'<text x="{margin["left"]}" y="35" fill="#f0f6fc" font-family="system-ui, -apple-system, sans-serif" font-size="18" font-weight="600">{title}</text>',
        f'<text x="{margin["left"] + plot_w/2}" y="{height - 15}" fill="#8b949e" font-family="system-ui, sans-serif" font-size="12" text-anchor="middle">{x_label}</text>',
        f'<text x="25" y="{margin["top"] + plot_h/2}" fill="#8b949e" font-family="system-ui, sans-serif" font-size="12" text-anchor="middle" transform="rotate(-90 25 {margin["top"] + plot_h/2})">{y_label}</text>',
        # Grid lines
        f'<line x1="{margin["left"]}" y1="{margin["top"] + plot_h}" x2="{margin["left"] + plot_w}" y2="{margin["top"] + plot_h}" stroke="#30363d" stroke-width="1"/>',
        f'<line x1="{margin["left"]}" y1="{margin["top"]}" x2="{margin["left"]}" y2="{margin["top"] + plot_h}" stroke="#30363d" stroke-width="1"/>',
    ]

    # Legend & Plot Series
    legend_y = margin["top"] + 10
    color_idx = 0
    for s_name, points in series_dict.items():
        color = colors[color_idx % len(colors)]
        color_idx += 1

        poly_pts = []
        for x, y in points:
            px = scale_x(x)
            py = scale_y(y)
            poly_pts.append(f"{px:.1f},{py:.1f}")

        # Draw line
        svg.append(f'<polyline points="{" ".join(poly_pts)}" fill="none" stroke="{color}" stroke-width="2.5" stroke-linejoin="round"/>')

        # Draw dots
        for x, y in points:
            px = scale_x(x)
            py = scale_y(y)
            svg.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="4" fill="{color}"/>')

        # Legend item
        svg.append(f'<circle cx="{margin["left"] + plot_w + 20}" cy="{legend_y}" r="5" fill="{color}"/>')
        svg.append(f'<text x="{margin["left"] + plot_w + 32}" y="{legend_y + 4}" fill="#c9d1d9" font-family="system-ui, sans-serif" font-size="11">{s_name}</text>')
        legend_y += 24

    svg.append('</svg>')

    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "w") as f:
        f.write("\n".join(svg))
    print(f"[Chart Generated] {filename}")

def generate_markdown_summary(results, output_path):
    lines = [
        "# Benchmark Results Summary\n",
        "Performance benchmarks compiled with `-O3 -DNDEBUG` comparing `dynamicArray` against `std::vector` and allocator variants.\n",
        "## 1. Container PushBack Throughput (Time in ns)\n",
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
        ratio = f"{s_g / d_g:.2f}x" if d_g > 0 else "N/A"
        lines.append(f"| {n:,} | {d_g:,.1f} ns | {s_g:,.1f} ns | **{ratio}** | {d_r:,.1f} ns | {s_r:,.1f} ns |")

    lines.extend([
        "\n## 2. Allocator Impact: std::allocator (Heap malloc) vs ArenaAllocator (0-Syscall Bump)\n",
        "| Cycle Elements | std::allocator (malloc/free) | ArenaAllocator (Bump + Reset) | Latency Reduction |",
        "| :--- | :--- | :--- | :--- |"
    ] )

    h_data = dict(results.get("BM_Allocator_StdHeap", []))
    a_data = dict(results.get("BM_Allocator_MonotonicArena", []))
    for n in sorted(h_data.keys()):
        h_time = h_data.get(n, 0)
        a_time = a_data.get(n, 0)
        speedup = f"{h_time / a_time:.2f}x faster" if a_time > 0 else "N/A"
        lines.append(f"| {n:,} pushes | {h_time:,.1f} ns | {a_time:,.1f} ns | **{speedup}** |")

    lines.extend([
        "\n## 3. The Boolean Specialization Crossover (Scan Latency)\n",
        "| Total Booleans | dynamicArray<uint8_t> (Byte) | dynamicArray<bool> (1-bit Packed) | std::vector<bool> (STL) | Memory Bandwidth Advantage |",
        "| :--- | :--- | :--- | :--- | :--- |"
    ])

    b_byte = dict(results.get("BM_Bool_ByteArray_Scan", []))
    b_bit = dict(results.get("BM_Bool_BitPacked_Scan", []))
    s_bit = dict(results.get("BM_StdVector_Bool_Scan", []))
    for n in sorted(b_byte.keys()):
        by = b_byte.get(n, 0)
        bi = b_bit.get(n, 0)
        sb = s_bit.get(n, 0)
        ratio = f"{by / bi:.2f}x" if bi > 0 else "N/A"
        lines.append(f"| {n:,} | {by:,.1f} ns | {bi:,.1f} ns | {sb:,.1f} ns | **{ratio}** |")

    with open(output_path, "w") as f:
        f.write("\n".join(lines))
    print(f"[Summary Generated] {output_path}")

def main():
    json_path = sys.argv[1] if len(sys.argv) > 1 else "benchmark_results.json"
    if not os.path.exists(json_path):
        print(f"Error: {json_path} not found. Run benchmark_suite first.")
        sys.exit(1)

    data = load_benchmark_data(json_path)
    results = extract_benchmarks(data)

    chart_dir = "benchmarks/charts"
    os.makedirs(chart_dir, exist_ok=True)

    # 1. Throughput Chart
    make_svg_line_chart(
        "PushBack Throughput: dynamicArray vs std::vector",
        "Number of Elements (N)",
        "CPU Time (ns, log scale)",
        {
            "dynamicArray (Geometric)": results.get("BM_DynamicArray_PushBack_Geometric", []),
            "std::vector (Geometric)": results.get("BM_StdVector_PushBack_Geometric", []),
            "dynamicArray (Reserved)": results.get("BM_DynamicArray_PushBack_Reserved", []),
            "std::vector (Reserved)": results.get("BM_StdVector_PushBack_Reserved", []),
        },
        f"{chart_dir}/throughput_comparison.svg"
    )

    # 2. Allocator Latency Chart
    make_svg_line_chart(
        "Memory Model: Heap Allocator vs Monotonic Arena",
        "Elements Allocated Per Cycle",
        "Cycle Time (ns)",
        {
            "std::allocator (Heap)": results.get("BM_Allocator_StdHeap", []),
            "ArenaAllocator (0-Syscall Bump)": results.get("BM_Allocator_MonotonicArena", []),
        },
        f"{chart_dir}/allocator_latency.svg"
    )

    # 3. Boolean Crossover Chart
    make_svg_line_chart(
        "The Boolean Crossover: Byte Array vs Bit-Packed Specialization",
        "Total Elements (N)",
        "Scan Time (ns, log scale)",
        {
            "dynamicArray<uint8_t> (1 Byte/Bool)": results.get("BM_Bool_ByteArray_Scan", []),
            "dynamicArray<bool> (Bit-Packed)": results.get("BM_Bool_BitPacked_Scan", []),
            "std::vector<bool> (STL Baseline)": results.get("BM_StdVector_Bool_Scan", []),
        },
        f"{chart_dir}/bool_crossover.svg"
    )

    # Generate Markdown Summary
    generate_markdown_summary(results, "benchmarks/BENCHMARK_RESULTS.md")

if __name__ == "__main__":
    main()
