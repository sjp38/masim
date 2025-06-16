#!/usr/bin/env python3

import argparse

import masim_config

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
            '--region', nargs=3, action='append',
            metavar=('<name>', '<sz_bytes>', '<initial data file>'),
            help='memory region of the access pattern')
    parser.add_argument('--phase', nargs=2, action='append',
                        metavar=('<name>', '<runtime_ms>'),
                        help='access pattern execution phase')
    parser.add_argument(
            '--access_pattern', nargs=6, action='append',
            metavar=('<phase name>', '<region name>', '<randomness>',
                     '<stride>', '<access_probability>', '<rw_mode>'),
            help='per-phase per-region access pattern')
    args = parser.parse_args()

    if args.region is None or args.phase is None or \
            args.access_pattern is None:
        print('config is not given')
        exit(1)

    regions = []
    for name, sz_bytes, data_file in args.region:
        try:
            sz_bytes = int(sz_bytes)
        except Exception as e:
            print('wrong region size (%s, %s)' % (sz_bytes, e))
            exit(1)
        regions.append(masim_config.Region(name, sz_bytes, data_file))

    patterns_for_phase = {}
    for access_pattern in args.access_pattern:
        phase_name, region_name, randomness, stride, probability, rwmode = \
                access_pattern
        try:
            randomness = int(randomness)
        except Exception as e:
            print('wrong randomness (%s, %s)' % (randomness, e))
            exit(1)
        if not randomness in [0, 1]:
            print('randomness not in [0, 1]: %s' % randomness)
            exit(1)
        try:
            stride = int(stride)
        except Exception as e:
            print('wrong stride (%s, %s)' % (stride, e))
            exit(1)
        try:
            probability = int(probability)
        except Exception as e:
            print('wrong pribability (%s, %s)' % (probability, e))
            exit(1)
        if not rwmode in ['wo', 'ro', 'rw']:
            print('wrong rwmode (%s)' % rwmode)
            exit(1)
        if not phase_name in patterns_for_phase:
            patterns_for_phase[phase_name] = []
        patterns_for_phase[phase_name].append(
                masim_config.AccessPattern(
                    region_name, randomness, stride, probability, rwmode))

    phases = []
    for name, runtime_ms in args.phase:
        try:
            runtime_ms = int(runtime_ms)
        except Exception as e:
            print('wrong phase runtime (%s, %s)' % (runtime_ms, e))
            exit(1)
        phases.append(masim_config.Phase(
            name, runtime_ms, patterns_for_phase[name]))

    masim_config.pr_config(regions, phases)

if __name__ == '__main__':
    main()
