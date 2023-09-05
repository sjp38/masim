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

if __name__ == '__main__':
    main()
