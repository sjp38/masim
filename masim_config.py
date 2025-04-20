# SPDX-License-Identifier: GPL-2.0

class Region:
    name = None
    length = None

    def __init__(self, name, length):
        self.name = name
        self.length = length

class AccessPattern:
    region_name = None
    randomness = None
    stride = None
    access_probability = None

    def __init__(self, region_name, randomness, stride, access_probability):
        self.region_name = region_name
        self.randomness = randomness
        self.stride = stride
        self.access_probability = access_probability

class Phase:
    name = None
    runtime_ms = None
    patterns = None

    def __init__(self, name, runtime_ms, patterns):
        self.name = name
        self.runtime_ms = runtime_ms
        self.patterns = patterns
