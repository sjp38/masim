#!/usr/bin/env python3

'''
Receive ascii art as input and convert it into masim access config file, so
that 'damo report heats --heatmap' like command could show a colorized version
of the ascii art.

Input is assumed to be a matrix having 0-9 as values.  e.g.,

    01100
    00100
    02100
    00100
    00100
'''

import argparse
import os

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('ascii_art', metavar='<string or file>',
            help='the ascii art to convert')
    parser.add_argument('space', metavar='<bytes>', type=int,
            help='the total memory size of the access pattern (length)')
    parser.add_argument('time', metavar='<seconds>', type=int,
            help='the total time for the accesses (height)')
    parser.add_argument('config_file', metavar='<file>',
            help='the path for the output masim config file')
    args = parser.parse_args()

    if os.path.isfile(args.ascii_art):
        with open(args.ascii_art, 'r') as f:
            ascii_art = f.read().strip()
    else:
        ascii_art = args.ascii_art

    rows = ascii_art.split('\n')
    nr_cols = len(rows[0])

    config_lines = []
    # each column becomes region; each row becomes phase
    region_size = args.space / nr_cols
    for i in range(nr_cols):
        config_lines.append('region-%d, %d' % (i, region_size))
    config_lines.append('')

    phase_time = args.time / len(rows)
    for row_idx, row in enumerate(rows):
        config_lines.append('row %d' % row_idx)
        config_lines.append('%d' % phase_time)
        for cell_idx, cell in enumerate(row):
            config_lines.append('region-%d, 0, 4096, %s' % (cell_idx, cell))
        config_lines.append('')

    with open(args.config_file, 'w') as f:
        f.write('\n'.join(config_lines))

if __name__ == '__main__':
    main()
