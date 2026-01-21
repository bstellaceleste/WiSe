#!/usr/bin/env python3
"""
Adaptive Binary Instrumentation Engine - Results Analyzer
Analyzes the JSON output and generates visualizations
"""

import json
import sys
import argparse
from pathlib import Path


def load_results(filename):
    """Load results from JSON file"""
    try:
        with open(filename, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in '{filename}': {e}")
        sys.exit(1)


def print_summary(data):
    """Print summary of results"""
    print("=" * 60)
    print("  ANALYSIS SUMMARY")
    print("=" * 60)
    print()

    print(f"Engine Version:        {data.get('engine_version', 'N/A')}")
    print(f"Functions Analyzed:    {data.get('total_functions_analyzed', 0)}")
    print(f"Total Calls:           {data.get('total_calls', 0)}")
    print(f"Total Memory Ops:      {data.get('total_memory_ops', 0)}")
    print()

    # Statistics
    if 'statistics' in data:
        stats = data['statistics']
        print("Statistical Analysis:")
        print(f"  Call Mean:           {stats.get('call_mean', 0):.2f}")
        print(f"  Call StdDev:         {stats.get('call_stddev', 0):.2f}")
        print(f"  Call P95:            {stats.get('call_p95', 0)}")
        print(f"  Call P99:            {stats.get('call_p99', 0)}")
        print(f"  Memory Mean:         {stats.get('mem_mean', 0):.2f}")
        print(f"  Memory StdDev:       {stats.get('mem_stddev', 0):.2f}")
        print(f"  Memory P95:          {stats.get('mem_p95', 0)}")
        print(f"  Memory P99:          {stats.get('mem_p99', 0)}")
        print()


def print_top_functions(data, n=10):
    """Print top N most called functions"""
    if 'function_profiles' not in data:
        print("No function profiles found")
        return

    profiles = data['function_profiles']
    sorted_profiles = sorted(profiles, key=lambda x: x['call_count'], reverse=True)

    print(f"Top {n} Most Called Functions:")
    print("-" * 60)

    for i, prof in enumerate(sorted_profiles[:n], 1):
        name = prof['name']
        addr = prof['address']
        calls = prof['call_count']
        is_hotspot = prof['is_hotspot']
        is_patched = prof['is_patched']

        status = ""
        if is_hotspot:
            status += " ⚡HOTSPOT"
        if is_patched:
            status += " 🔧PATCHED"

        print(f"{i:2d}. {name:30s} @ {addr}")
        print(f"    Calls: {calls:10d}{status}")

        # Memory stats
        stack_ops = prof['stack_reads'] + prof['stack_writes']
        heap_ops = prof['heap_reads'] + prof['heap_writes']
        if stack_ops > 0 or heap_ops > 0:
            print(f"    Memory: Stack={stack_ops}, Heap={heap_ops}")

    print()


def print_hotspots(data):
    """Print all detected hotspots"""
    if 'function_profiles' not in data:
        return

    profiles = data['function_profiles']
    hotspots = [p for p in profiles if p['is_hotspot']]

    print(f"Detected Hotspots: {len(hotspots)}")
    print("-" * 60)

    if not hotspots:
        print("No hotspots detected")
        return

    for i, prof in enumerate(hotspots, 1):
        print(f"{i}. {prof['name']} @ {prof['address']}")
        print(f"   Calls: {prof['call_count']}")
        print(f"   Patched: {'Yes' if prof['is_patched'] else 'No'}")
        print()


def generate_histogram(data, output_file=None):
    """Generate call count histogram"""
    try:
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        print("matplotlib not available, skipping histogram generation")
        return

    if 'function_profiles' not in data:
        return

    profiles = data['function_profiles']
    call_counts = [p['call_count'] for p in profiles if p['call_count'] > 0]

    if not call_counts:
        print("No data for histogram")
        return

    plt.figure(figsize=(12, 6))
    plt.hist(call_counts, bins=50, edgecolor='black', alpha=0.7)
    plt.xlabel('Call Count')
    plt.ylabel('Number of Functions')
    plt.title('Distribution of Function Call Frequencies')
    plt.yscale('log')
    plt.grid(True, alpha=0.3)

    if output_file:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Histogram saved to: {output_file}")
    else:
        plt.show()


def generate_report(data, output_file):
    """Generate detailed text report"""
    with open(output_file, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("  ADAPTIVE BINARY INSTRUMENTATION ENGINE - DETAILED REPORT\n")
        f.write("=" * 80 + "\n\n")

        # Summary
        f.write("SUMMARY\n")
        f.write("-" * 80 + "\n")
        f.write(f"Engine Version:        {data.get('engine_version', 'N/A')}\n")
        f.write(f"Functions Analyzed:    {data.get('total_functions_analyzed', 0)}\n")
        f.write(f"Total Calls:           {data.get('total_calls', 0)}\n")
        f.write(f"Total Memory Ops:      {data.get('total_memory_ops', 0)}\n\n")

        # Statistics
        if 'statistics' in data:
            stats = data['statistics']
            f.write("STATISTICAL ANALYSIS\n")
            f.write("-" * 80 + "\n")
            f.write(f"Call Mean:             {stats.get('call_mean', 0):.2f}\n")
            f.write(f"Call Standard Dev:     {stats.get('call_stddev', 0):.2f}\n")
            f.write(f"Call 95th Percentile:  {stats.get('call_p95', 0)}\n")
            f.write(f"Call 99th Percentile:  {stats.get('call_p99', 0)}\n")
            f.write(f"Memory Mean:           {stats.get('mem_mean', 0):.2f}\n")
            f.write(f"Memory Standard Dev:   {stats.get('mem_stddev', 0):.2f}\n")
            f.write(f"Memory 95th Percentile:{stats.get('mem_p95', 0)}\n")
            f.write(f"Memory 99th Percentile:{stats.get('mem_p99', 0)}\n\n")

        # All functions
        if 'function_profiles' in data:
            profiles = data['function_profiles']
            sorted_profiles = sorted(profiles, key=lambda x: x['call_count'], reverse=True)

            f.write("ALL FUNCTION PROFILES\n")
            f.write("-" * 80 + "\n")

            for prof in sorted_profiles:
                f.write(f"\nFunction: {prof['name']}\n")
                f.write(f"  Address:      {prof['address']}\n")
                f.write(f"  Call Count:   {prof['call_count']}\n")
                f.write(f"  Stack Reads:  {prof['stack_reads']}\n")
                f.write(f"  Stack Writes: {prof['stack_writes']}\n")
                f.write(f"  Heap Reads:   {prof['heap_reads']}\n")
                f.write(f"  Heap Writes:  {prof['heap_writes']}\n")
                f.write(f"  Is Hotspot:   {prof['is_hotspot']}\n")
                f.write(f"  Is Patched:   {prof['is_patched']}\n")

    print(f"Detailed report saved to: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description='Analyze Adaptive Engine results'
    )
    parser.add_argument('json_file', help='JSON results file to analyze')
    parser.add_argument('--report', help='Generate detailed text report',
                        metavar='FILE')
    parser.add_argument('--histogram', help='Generate histogram image',
                        metavar='FILE')
    parser.add_argument('--top', type=int, default=10,
                        help='Number of top functions to display (default: 10)')

    args = parser.parse_args()

    # Load results
    data = load_results(args.json_file)

    # Print summary
    print_summary(data)

    # Print top functions
    print_top_functions(data, args.top)

    # Print hotspots
    print_hotspots(data)

    # Generate report if requested
    if args.report:
        generate_report(data, args.report)

    # Generate histogram if requested
    if args.histogram:
        generate_histogram(data, args.histogram)


if __name__ == '__main__':
    main()
