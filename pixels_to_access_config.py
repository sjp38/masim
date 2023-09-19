#!/usr/bin/env python3

'''
Receive pixels data as input and convert it into masim access config file, so
that 'damo report heats --heatmap' like command could show a sort of pixel
arts.

Pixels is assumed to be a matrix having 0-9 as each cell.  e.g.,

    01100
    00100
    02100
    00100
    00100

Each row is for one time phase, and each cell is the memory region.  The values
on cells are the relative access frequency of the memory region.
'''

import argparse
import os

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('pixels', metavar='<string or file>',
            help='the pixels describing wanted access pattern')
    parser.add_argument('space', metavar='<bytes>', type=int,
            help='the total memory size of the access pattern (length)')
    parser.add_argument('time', metavar='<seconds>', type=int,
            help='the total time for the accesses (height)')
    parser.add_argument('config_file', metavar='<file>',
            help='the path for the output masim config file')
    args = parser.parse_args()

    if os.path.isfile(args.pixels):
        with open(args.pixels, 'r') as f:
            pixels = f.read().strip()
    else:
        pixels = args.pixels

    rows = pixels.split('\n')
    nr_cols = len(rows[0])

    config_lines = []
    # each column becomes region; each row becomes phase
    region_size = args.space / nr_cols
    for i in range(nr_cols):
        config_lines.append('region%d, %d' % (i, region_size))
    config_lines.append('')

    phase_time = args.time / len(rows)
    for row_idx, row in enumerate(rows):
        config_lines.append('row %d' % row_idx)
        config_lines.append('%d' % (phase_time * 1000))
        for cell_idx, cell in enumerate(row):
            if cell != '0':
                config_lines.append('region%d, 0, 4096, %s' % (cell_idx, cell))
        config_lines.append('')

    with open(args.config_file, 'w') as f:
        f.write('\n'.join(config_lines))

if __name__ == '__main__':
    main()
