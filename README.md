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

This repository contains several default configuration files.  Please refer to
those for usage and format of the file.  'configs/default' would be a good
starting point.


Author
------

SeongJae Park <sj38.park@gmail.com>
