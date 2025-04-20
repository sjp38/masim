#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0

prog_description = '''
Generate masim config that can be used for tiered memory system performance
testing.

The configuration will have multiple memory regions of same size.  It will also
have multiple access phases of same runtime.  Each phase will access each
memory region in different hotness.  The hotness of each region per phase is
randomly decided.

The size of each regions, the number of regions, the runtime of each phase, and
the number of phases can be set by the user.
'''

import argparse

import masim_config

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--memsize', metavar='<bytes>', type=int,
                        help='total size of memory to make stress')
    parser.add_argument('--nr_regions', type=int,
                        help='number of regions having different hotness')
    parser.add_argument('--nr_phases', type=int,
                        help='number of phases')
    parser.add_argument('--phase_runtime_ms', metavar='<milliseconds>',
                        type=int, help='runtime of each phase')
    parser.description = prog_description
    args = parser.parse_args()

if __name__ == '__main__':
    main()
