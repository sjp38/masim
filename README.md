Memory Access Simulator
=======================

'masim' is a program for generating memory acccesses of artificial patterns.
You can use it to test the performance or behavior of your memory system.


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
repeat accessing first, fifth, and ninth bytes of the region in the order.

The fourth field is the probability of the access pattern to be selected for
execution by `masim` during the given phase.  For example, if the phase has two
access pattern lines with this probability value `2` and `1`, `masim` will
execute the first access pattern two times more frequently than the second
access pattern.  The probability is relative to those of other patterns, so any
number can be given.

The fifth field specifies whether to do read only (`ro`), write only (`wo`), or
both read and write (`rw`) access.

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

Author
------

SeongJae Park <sj38.park@gmail.com>
