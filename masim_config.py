# SPDX-License-Identifier: GPL-2.0

class Region:
    name = None
    sz_bytes = None
    init_data_file = None

    def __init__(self, name, sz_bytes, init_data_file=None):
        self.name = name
        self.sz_bytes = sz_bytes
        self.init_data_file = init_data_file

class AccessPattern:
    region_name = None
    randomness = None
    stride = None
    access_probability = None
    rw_mode = None  # wo, ro, rw
    interval_us = None

    def __init__(self, region_name, randomness, stride, access_probability,
                 rw_mode, interval_us):
        self.region_name = region_name
        self.randomness = randomness
        self.stride = stride
        self.access_probability = access_probability
        self.rw_mode = rw_mode
        self.interval_us = interval_us

class Phase:
    name = None
    runtime_ms = None
    patterns = None

    def __init__(self, name, runtime_ms, patterns):
        self.name = name
        self.runtime_ms = runtime_ms
        self.patterns = patterns

def fmt_bytes(bytes):
    if bytes < 1024:
        return '%d bytes' % bytes
    if bytes < 1024 * 1024:
        return '%d KiB' % (bytes / 1024)
    if bytes < 1024 * 1024 * 1024:
        return '%d MiB' % (bytes / 1024 / 1024)
    if bytes < 1024 * 1024 * 1024 * 1024:
        return '%d GiB' % (bytes / 1024 / 1024 / 1024)
    return '%d TiB' % (bytes / 1024 / 1024 / 1024 / 1024)

def fmt_seconds(seconds):
    if seconds < 1:
        ms = seconds * 1000
        if ms > 1:
            return '%d milliseconds' % ms
        else :
            us = seconds * 1000
            return '%.3f microseconds' % us
    if seconds > 60:
        minutes = seconds / 60
        if minutes < 60:
            return '%.3f minutes' % minutes
        hours = minutes / 60
        if hours < 24:
            return '%.3f hours' % hours
        return '%.3f dasys' % (hours / 24)
    return '%d seconds' % seconds

def fmt_config(regions, phases, add_comments=False):
    lines = []
    for region in regions:
        init_data_file = region.init_data_file
        if init_data_file is None:
            init_data_file = 'none'
        if add_comments:
            words = ['# allocate %s memory, call it %s' %
                     (fmt_bytes(region.sz_bytes), region.name)]
            if init_data_file != 'none':
                words.append(', and initialize it with contents of %s' %
                             init_data_file)
            lines.append(''.join(words))
        lines.append('%s, %d, %s' % (
            region.name, region.sz_bytes, init_data_file))
    lines.append('')
    for idx, phase in enumerate(phases):
        if add_comments:
            lines.append('# for next %s, do below.' %
                         fmt_seconds(phase.runtime_ms / 1000))
        lines.append(phase.name)
        lines.append('%s' % phase.runtime_ms)
        for pattern in phase.patterns:
            if add_comments:
                words = []
                if pattern.rw_mode == 'ro':
                    words.append('read')
                elif pattern.rw_mode == 'rw':
                    words.append('read and write')
                elif pattern.rw_mode == 'wo':
                    words.append('write')
                if pattern.randomness:
                    words.append('random bytes of region %s' %
                                 pattern.region_name)
                else:
                    words.append('bytes of region %s in address order' %
                                 pattern.region_name)
                words.append('with %s stride' % fmt_bytes(pattern.stride))
                if pattern.access_probability > 0:
                    words.append('with %d relative frequency' %
                                 pattern.access_probability)
                else:
                    words.append('once per %s' %
                                 fmt_seconds(pattern.interval_us / 1000000))
                comment_line = ' '.join(words)
                lines.append('# %s' % comment_line)

            lines.append('%s, %d, %d, %d, %s, %d' % (
                pattern.region_name, 1 if pattern.randomness else 0,
                pattern.stride, pattern.access_probability, pattern.rw_mode,
                pattern.interval_us))
        if idx < len(phases) - 1:
            lines.append('')
    return '\n'.join(lines)

def pr_config(regions, phases):
    print(fmt_config(regions, phases))
