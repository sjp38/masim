#!/usr/bin/env python3

import argparse
import subprocess

import masim_config

def parse_bytes(text):
    suffix_unit = {
            'b': 1,
            'k': 1024,
            'm': 1024**2,
            'g': 1024**3,
            't': 1024**4,
            }
    suffix = text[-1].lower()
    if suffix in suffix_unit:
        numbers = text[:-1]
    else:
        numbers = text
    try:
        numbers = int(numbers)
    except Exception as e:
        return None, 'number parsing fail (%s)' % e

    if suffix in suffix_unit:
        numbers = numbers * suffix_unit[suffix]
    return numbers, None

def build_regions_phases(args):
    regions = []
    for name, sz_bytes, data_file in args.region:
        sz_bytes, err = parse_bytes(sz_bytes)
        if err is not None:
            print('wrong region size (%s, %s)' % (sz_bytes, err))
            exit(1)
        regions.append(masim_config.Region(name, sz_bytes, data_file))

    patterns_for_phase = {}
    for access_pattern in args.access_pattern:
        phase_name = access_pattern[0]
        region_name = access_pattern[1]
        randomness = access_pattern[2]
        stride = access_pattern[3]
        probability = access_pattern[4]
        rwmode = access_pattern[5]
        interval_us = access_pattern[6]
        try:
            randomness = int(randomness)
        except Exception as e:
            print('wrong randomness (%s, %s)' % (randomness, e))
            exit(1)
        if not randomness in [0, 1]:
            print('randomness not in [0, 1]: %s' % randomness)
            exit(1)
        stride, err = parse_bytes(stride)
        if err is not None:
            print('wrong stride (%s, %s)' % (stride, err))
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
        try:
            interval_us = int(interval_us)
        except Exception as e:
            print('wrong interval_us (%s, %s)' % (interval_us, e))
        patterns_for_phase[phase_name].append(
                masim_config.AccessPattern(
                    region_name, randomness, stride, probability, rwmode,
                    interval_us))

    phases = []
    for name, runtime_ms in args.phase:
        try:
            runtime_ms = int(runtime_ms)
        except Exception as e:
            print('wrong phase runtime (%s, %s)' % (runtime_ms, e))
            exit(1)
        phases.append(masim_config.Phase(
            name, runtime_ms, patterns_for_phase[name]))

    return regions, phases

def add_config_arguments(parser):
    parser.add_argument(
            '--region', nargs=3, action='append',
            metavar=('<name>', '<sz_bytes>', '<initial data file>'),
            help='memory region of the access pattern')
    parser.add_argument('--phase', nargs=2, action='append',
                        metavar=('<name>', '<runtime_ms>'),
                        help='access pattern execution phase')
    parser.add_argument(
            '--access_pattern', nargs=7, action='append',
            metavar=('<phase name>', '<region name>', '<randomness>',
                     '<stride>', '<access_probability>', '<rw_mode>',
                     '<interval_us>'),
            help='per-phase per-region access pattern')

def do_pr_config(args):
    if args.region is None or args.phase is None or \
            args.access_pattern is None:
        print('config is not given')
        exit(1)

    regions, phases = build_regions_phases(args)
    masim_config.pr_config(regions, phases)

def do_run(args):
    if args.region is None or args.phase is None or \
            args.access_pattern is None:
        print('config is not given')
        exit(1)

    regions, phases = build_regions_phases(args)
    with open(args.config_file, 'w') as f:
        f.write(masim_config.fmt_config(regions, phases) + '\n')
    cmd = [args.masim_bin, args.config_file]
    if args.log_interval is not None:
        cmd += ['--log_interval=%d' % args.log_interval]
    if args.accesses_per_region_selection is not None:
        cmd.append('--nr_accesses_per_region=%d' %
                   args.accesses_per_region_selection)
    subprocess.run(cmd)

def main():
    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(
            title='action', dest='action', metavar='<action>')

    parser_config = subparsers.add_parser('pr_config', help='print config')
    add_config_arguments(parser_config)

    parser_run = subparsers.add_parser('run', help='run masim')
    add_config_arguments(parser_run)
    parser_run.add_argument('--masim_bin', metavar='<file>', default='./masim',
                            help='path to masim executable file')
    parser_run.add_argument('--config_file', metavar='<file>',
                            default='masim_py_runconfig',
                            help='"masim.py run" saves config file here')
    parser_run.add_argument(
            '--log_interval', metavar='<milliseconds>', type=int,
            help='periodic access speed logging interval')
    parser_run.add_argument(
            '--accesses_per_region_selection', type=int,
            help='number of accesses to make per region selection')
    args = parser.parse_args()

    if args.action == 'pr_config':
        do_pr_config(args)
    elif args.action == 'run':
        do_run(args)

if __name__ == '__main__':
    main()
