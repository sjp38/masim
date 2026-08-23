#!/usr/bin/env python3

import argparse
import signal
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

def time_float_nr_suffix(text):
    suffix = text[-2:].lower()
    if suffix in ['us', 'ms']:
        return text[:-2], suffix
    suffix = text[-1:].lower()
    if suffix in ['s', 'm', 'h', 'd']:
        return text[:-1], suffix
    return text, 's'

def parse_seconds(text):
    float_nr, suffix = time_float_nr_suffix(text)
    try:
        float_nr = float(float_nr)
    except Exception as e:
        return None, 'number parsing fail (%s, %s)' % (text, e)
    suffix_scale = {
            'us': 1 / 1000000,
            'ms': 1 / 1000,
            's': 1,
            'm': 60,
            'h': 3600,
            'd': 3600 * 24,
            }
    return int(float_nr * suffix_scale[suffix]), None

def build_regions_phases(args):
    regions = []
    for fields in args.region:
        if not len(fields) in [2, 3]:
            print('wrong number of --region fields (%s)' % fields)
            exit(1)
        name = fields[0]
        sz_bytes = fields[1]
        sz_bytes, err = parse_bytes(sz_bytes)
        if err is not None:
            print('wrong region size (%s, %s)' % (sz_bytes, err))
            exit(1)
        if len(fields) > 2:
            data_file = fields[2]
        else:
            data_file = 'none'
        regions.append(masim_config.Region(name, sz_bytes, data_file))

    patterns_for_phase = {}
    for access_pattern in args.access_pattern:
        phase_name = access_pattern[0]
        region_name = access_pattern[1]
        randomness = access_pattern[2]
        stride = access_pattern[3]
        probability = access_pattern[4]
        rwmode = access_pattern[5]
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

        interval_sec, err = parse_seconds(access_pattern[6])
        if err is not None:
            print('interval_sec parse fail (%s, %s)' %
                  (access_pattern[6], err))
            exit(1)
        interval_us = int(interval_sec * 1000000)
        patterns_for_phase[phase_name].append(
                masim_config.AccessPattern(
                    region_name, randomness, stride, probability, rwmode,
                    interval_us))

    phases = []
    for name, runtime_sec in args.phase:
        runtime_sec, err = parse_seconds(runtime_sec)
        if err is not None:
            print('phase runtime parse fail (%s, %s)' % (args.phase[1], e))
            exit(1)
        runtime_ms = int(runtime_sec * 1000)
        phases.append(masim_config.Phase(
            name, runtime_ms, patterns_for_phase[name]))

    return regions, phases

def add_config_arguments(parser):
    parser.add_argument(
            '--region', nargs='+', action='append', metavar='<field>',
            help='memory region of the access pattern; '
            '<name> <sz_bytes> [inital data file]')
    parser.add_argument('--phase', nargs=2, action='append',
                        metavar=('<name>', '<runtime_sec>'),
                        help='access pattern execution phase')
    parser.add_argument(
            '--access_pattern', nargs=7, action='append',
            metavar=('<phase name>', '<region name>', '<randomness>',
                     '<stride>', '<access_probability>', '<rw_mode>',
                     '<interval_sec>'),
            help='per-phase per-region access pattern')

def add_run_arguments(parser):
    parser.add_argument('--masim_bin', metavar='<file>', default='./masim',
                            help='path to masim executable file')
    parser.add_argument('--config_file', metavar='<file>',
                            default='masim_py_runconfig',
                            help='config file to save or load')
    parser.add_argument(
            '--log_interval', metavar='<seconds>', type=float,
            help='periodic access speed logging interval')
    parser.add_argument(
            '--nr_accesses_per_region', type=int, metavar='<int>',
            help='number of accesses to make per region selection')
    parser.add_argument('--quiet', action='store_true',
                            help='Suppress normal output')
    parser.add_argument('--repeat', type=int, metavar='<int>',
                            help='Repeat run')

def do_pr_config(args):
    if args.region is None or args.phase is None or \
            args.access_pattern is None:
        print('config is not given')
        exit(1)

    regions, phases = build_regions_phases(args)
    masim_config.pr_config(regions, phases)

def run_sighandler(signum, frame):
    print('\nsignal %s received' % signum)

def do_run(args):
    build_save_config = True
    if args.region is None or args.phase is None or \
            args.access_pattern is None:
        if args.config_file is not None:
            build_save_config = False
        else:
            print('config is not given')
            exit(1)

    if build_save_config is True:
        regions, phases = build_regions_phases(args)
        with open(args.config_file, 'w') as f:
            f.write(masim_config.fmt_config(regions, phases) + '\n')
    cmd = [args.masim_bin, args.config_file]
    if args.log_interval is not None:
        interval_ms = int(args.log_interval * 1000)
        cmd += ['--log_interval=%d' % interval_ms]
    if args.nr_accesses_per_region is not None:
        cmd.append('--nr_accesses_per_region=%d' %
                   args.accesses_per_region_selection)
    if args.quiet is True:
        cmd.append('--quiet')
    if args.repeat is not None:
        cmd.append('--repeat', '%d' % args.repeat)
    masim_proc = subprocess.Popen(cmd)
    print('masim is started: pid %d' % masim_proc.pid)
    # Ctrl-C will pass the signal to masim, kill it.
    print('Kill masim or press Ctrl-C to stop')
    signal.signal(signal.SIGINT, run_sighandler)
    signal.signal(signal.SIGTERM, run_sighandler)
    masim_proc.wait()

def main():
    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(
            title='action', dest='action', metavar='<action>')

    parser_config = subparsers.add_parser('pr_config', help='print config')
    add_config_arguments(parser_config)

    parser_run = subparsers.add_parser('run', help='run masim')
    add_config_arguments(parser_run)
    add_run_arguments(parser_run)
    args = parser.parse_args()

    if args.action == 'pr_config':
        do_pr_config(args)
    elif args.action == 'run':
        do_run(args)

if __name__ == '__main__':
    main()
