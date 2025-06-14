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

    def __init__(self, region_name, randomness, stride, access_probability,
                 rw_mode):
        self.region_name = region_name
        self.randomness = randomness
        self.stride = stride
        self.access_probability = access_probability
        self.rw_mode = rw_mode

class Phase:
    name = None
    runtime_ms = None
    patterns = None

    def __init__(self, name, runtime_ms, patterns):
        self.name = name
        self.runtime_ms = runtime_ms
        self.patterns = patterns

def pr_config(regions, phases):
    for region in regions:
        init_data_file = region.init_data_file
        if init_data_file is None:
            init_data_file = 'none'
        print('%s, %d, %s' % (region.name, region.sz_bytes, init_data_file))
    print()
    for idx, phase in enumerate(phases):
        print(phase.name)
        print(phase.runtime_ms)
        for pattern in phase.patterns:
            print('%s, %d, %d, %d, %s' % (
                pattern.region_name, 1 if pattern.randomness else 0,
                pattern.stride, pattern.access_probability, pattern.rw_mode))
        if idx < len(phases) - 1:
            print()
