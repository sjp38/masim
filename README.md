Memory Access Simulator
=======================

'masim' is a program for generating memory acccesses of artificial patterns.
You can use it to test the performance or behavior of your memory system.

Getting Started
---------------

```
$ # build masim
$ make
gcc -c -o masim.o masim.c -g -I -O3 -Wall -Werror -std=gnu99
gcc -c -o misc.o misc.c -g -I -O3 -Wall -Werror -std=gnu99
gcc -o masim masim.o misc.o
$
$ # build access pattern configuration with comments
$ ./masim.py pr_config --region r1 50M --region r2 100M /dev/random \
        --phase p1 5s \
        --access_pattern p1 r1 0 64 0 rw 5ms \
        --access_pattern p1 r2 1 128 0 ro 100ms \
        --phase p2 5s \
        --access_pattern p2 r1 1 64 10 wo 0 \
        --access_pattern p2 r2 0 64 90 wo 0
# allocate 50 MiB memory, call it r1
r1, 52428800, none
# allocate 100 MiB memory, call it r2, and initialize it with contents of /dev/random
r2, 104857600, /dev/random

# for next 5 seconds, do below.
p1
5000
# read and write bytes of region r1 in address order with 64 bytes stride once per 5 milliseconds
r1, 0, 64, 0, rw, 5000
# read random bytes of region r2 with 128 bytes stride once per 100 milliseconds
r2, 1, 128, 0, ro, 100000

# for next 5 seconds, do below.
p2
5000
# write random bytes of region r1 with 64 bytes stride with relative frequency of 10
r1, 1, 64, 10, wo, 0
# write bytes of region r2 in address order with 64 bytes stride with relative frequency of 90
r2, 0, 64, 90, wo, 0
$
$ # build the config and run it.
$ ./masim.py run \
        --region r1 50M --region r2 100M /dev/random \
        --phase p1 5s \
        --access_pattern p1 r1 0 64 0 rw 5ms \
        --access_pattern p1 r2 1 128 0 ro 100ms \
        --phase p2 5s \
        --access_pattern p2 r1 1 64 10 wo 0 \
        --access_pattern p2 r2 0 64 90 wo 0
masim is started: pid 68031
Kill masim or press Ctrl-C to stop
p1:                   73,400 accesses/msec, 5000 msecs run
p2:                  163,210 accesses/msec, 5000 msecs run
```

Usage
-----

Please do below:

```
$ make
$ ./masim <config file>
```

`<config file>` should be a path to a file containing the patterns of the
access you want to generate.  Read following section for how te config file can
be made.

Also, use `./masim --help` to see more options.


Configuration File
------------------

Data access pattern you want to generate can be specified via a configuration
file.  The path to the configuration file can be specified as the command line
argument.

The config file should be a plain text file.  The content should be multiple
paragraphs having one empty line between paragraphs.  Lines starting with `#`
are comments and ignored by `masim`.

### Regions

The first paragraph specifies memory regions (address ranges on `masim`'s
virtual address space) to construct for.  Each line specifies each region.
Each line should have three fields split by `, `.  The three fields specifies
the name of the region, the size of the region, and the path to a file that
contains data to be loaded to the region at the initialization phase.  If you
don't want to load a data to the region, you can put `none` for the file path.


### Phases

The second and all remaining paragraphs specify phases of access patterns to
execute.

The first line of a phase paragraph specifies the name of the phase.  The
second line of a phase paragraph specifies how long the phase should executed,
in milliseconds.

#### Access Pattern

Remaining lines of a phase paragraph specifies per-region access pattern for
the phase.  The line is constructed with five fields separated by `, `.  The
first field is the name of the region to set access pattern during the given
phase.

The second field specifies whether the access to the region during the phase
should be random or sequential.  `1` means random, `0` means sequential.  If
the access is sequential, the third field specifies the access stride size in
bytes.  For example, if the size of the region is `12 bytes` and the pattern
asks `masim` to do sequential access with `4 bytes` stride size, `masim` will
repeat accessing first, fifth, and ninth bytes of the region in the order.  If
the second field specifies the access should be random, the third field means
nothing.

The fourth field is the probability of the access pattern to be selected for
execution by `masim` during the given phase.  For example, if the phase has two
access pattern lines with this probability value `2` and `1`, `masim` will
execute the first access pattern two times more frequently than the second
access pattern.  The probability is relative to those of other patterns, so any
number can be given.

The fifth field specifies whether to do read only (`ro`), write only (`wo`), or
both read and write (`rw`) access.

The sixth field specifies time interval between access to the region in
microseconds.  This is ignored if the access probability (fourth field) is
not zero.  If this is not ignored (probability is zero), the probability is
ignored.  Instead, `masim` tries to access all memory of the region once per
the given time interval during the phase.

### Example

Let's see below config file content as an example.

```
#regions
# name, length, initial data file
a, 256, none
b, 64, /dev/zero
c, 128, /dev/urandom

# phase 1
# name of phase
example phase 1
# time in ms
1000
# access patterns
# name of region, randomness, stride, probability, read/write mode
a, 1, 64, 80, wo

# phase 2
example phase 2
# time in ms
1000
# access patterns
# name of region, randomness, stride, probability
c, 1, 64, 50, ro
b, 0, 4096, 50, rw
```

If this config is passed to `masim`, `masim` will allocate three memory regions
of size 256, 64, 128 bytes, respectively.  `masim` will internally name the
three regions as `a`, `b`, and `c`, respectively.  The second and third regions
will load 64 and 128 bytes of data that read from `/dev/zero` and
`/dev/urandom` files.

`masim` will then write random bytes of the first region (named `a`), for one
second.

After the one second, `masim` will read random bytes of the region named `c`
for one second.  `masim` will also sequentially read and write first bytes of 4
KiB sub-regions of the region named `b`, for the one second.  The access will
be repeatedly made during the one second phase.  For each of the access,
whether the access should be that for region `c` or region `b` will be decided
in 50:50 probability.
