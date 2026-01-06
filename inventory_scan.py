#!/usr/bin/env python3
import os
import sys
import json
import argparse
from pathlib import Path

WINDOWS_INDICATORS = [
    "#include <windows.h>", "#include <winsock", "WinMain(", "CreateWindow",
    "RegOpenKey", "RegCreateKey", "RegSetValue", "CoInitialize", "CoCreateInstance",
    "afx", "Afx", "MFC", "CWnd", "CDialog", "Service", "CreateService",
    "DeviceIoControl", "SetupDi", "SUSI", "HANDLE ", "HWND ", "HINSTANCE", "ws2_",
    "\.dll", "\.lib", "LoadLibrary", "GetProcAddress"
]

SKIP_DIRS = {'.git', 'Debug', 'Release', 'x64', 'Bin', '_Build', 'Debug_Remote', 'Debug_Remote', 'ipch', 'res'}
LARGE_FILE_THRESHOLD = 50 * 1024 * 1024  # 50 MB

EXT_SOURCE = ['.cpp', '.c', '.h', '.hpp', '.cc', '.rc', '.sln', '.vcxproj', '.vcproj', '.filters']


def scan(root):
    root = Path(root)
    summary = {
        'root': str(root),
        'counts': {},
        'solutions': [],
        'vcxproj': [],
        'source_files': 0,
        'header_files': 0,
        'resource_files': 0,
        'other_files': 0,
        'windows_indicator_matches': {},
        'large_files': [],
        'external_libs': set(),
    }

    for p in root.rglob('*'):
        try:
            rel = p.relative_to(root)
        except Exception:
            rel = p
        if any(part in SKIP_DIRS for part in rel.parts):
            continue
        if p.is_file():
            ext = p.suffix.lower()
            summary['counts'].setdefault(ext, 0)
            summary['counts'][ext] += 1
            if ext in ['.sln']:
                summary['solutions'].append(str(rel))
            if ext in ['.vcxproj', '.vcproj']:
                summary['vcxproj'].append(str(rel))
            if ext in ['.cpp', '.c', '.cc']:
                summary['source_files'] += 1
            elif ext in ['.h', '.hpp']:
                summary['header_files'] += 1
            elif ext in ['.rc']:
                summary['resource_files'] += 1
            else:
                summary['other_files'] += 1

            try:
                size = p.stat().st_size
            except Exception:
                size = 0
            if size >= LARGE_FILE_THRESHOLD:
                summary['large_files'].append({'path': str(rel), 'size_bytes': size})

            # quick scan for indicators in text files
            if ext in ['.cpp', '.c', '.h', '.hpp', '.rc', '.txt', '.xml', '.ini', '.props', '.vcxproj']:
                try:
                    text = p.read_text(errors='ignore')
                except Exception:
                    text = ''
                for pat in WINDOWS_INDICATORS:
                    if pat in text:
                        summary['windows_indicator_matches'].setdefault(pat, []).append(str(rel))
                # find references to .lib/.dll
                if '.lib' in text or '.dll' in text:
                    for token in text.split():
                        if token.lower().endswith('.lib') or token.lower().endswith('.dll'):
                            summary['external_libs'].add(token.strip('"<>:,;'))

    # finalize
    summary['external_libs'] = sorted(list(summary['external_libs']))
    summary['windows_indicator_counts'] = {k: len(v) for k, v in summary['windows_indicator_matches'].items()}
    return summary


def write_reports(summary, out_prefix):
    json_path = out_prefix + '.json'
    txt_path = out_prefix + '.txt'
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)
    # human readable
    with open(txt_path, 'w', encoding='utf-8') as f:
        f.write(f"Inventory report for: {summary['root']}\n\n")
        f.write("Summary counts (by extension):\n")
        for k, v in sorted(summary['counts'].items(), key=lambda x: -x[1]):
            f.write(f"  {k or '[noext]'}: {v}\n")
        f.write(f"\nSolutions: {len(summary['solutions'])}\n")
        for s in summary['solutions'][:50]:
            f.write(f"  {s}\n")
        f.write(f"\nvcxproj files: {len(summary['vcxproj'])}\n")
        for s in summary['vcxproj'][:50]:
            f.write(f"  {s}\n")
        f.write(f"\nSource files: {summary['source_files']}, Header files: {summary['header_files']}, Resource files: {summary['resource_files']}\n")
        f.write(f"\nLarge files (>{LARGE_FILE_THRESHOLD//1024//1024}MB): {len(summary['large_files'])}\n")
        for lf in summary['large_files'][:50]:
            f.write(f"  {lf['path']} ({lf['size_bytes']//1024//1024} MB)\n")
        f.write('\n\nWindows-specific indicator counts:\n')
        for k, v in sorted(summary['windows_indicator_counts'].items(), key=lambda x: -x[1]):
            f.write(f"  {k}: {v}\n")
        f.write('\nExternal libs/dlls referenced (sample):\n')
        for lib in summary['external_libs'][:200]:
            f.write(f"  {lib}\n")
    return json_path, txt_path


def main():
    parser = argparse.ArgumentParser(description='Inventory scan for porting analysis')
    parser.add_argument('root', nargs='?', default='.', help='Root directory to scan')
    parser.add_argument('-o', '--output', default='ANDROID_PORT_INVENTORY', help='Output file prefix (no ext)')
    args = parser.parse_args()
    root = args.root
    print(f"Scanning {root} ...")
    summary = scan(root)
    json_path, txt_path = write_reports(summary, args.output)
    print(f"Wrote: {json_path}, {txt_path}")

if __name__ == '__main__':
    main()
