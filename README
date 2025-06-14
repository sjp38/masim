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

The first paragraph specifies memory regions to construct for.  Each line
specifies each region.  Each line should have three fields split by `, `.  The
three fields specifies the name of the region, the size of the region, and the
path to a file that contains data to be loaded to the region at the
initialization phase.  If you don't want to load a data to the region, you can
put `none` for the file path.

The second and all remaining paragraphs specify phases of access patterns to
execute.

First line of a phase paragraph specifies the name of the phase.  Second line
of a phase paragraph specifies how long the phase should executed, in
milliseconds.

Remaining lines of a phase paragraph specifies access pattern of memory regions
in the phase.  The line is constructed with five fields separated by `, `.  The
first field is the name of the region to set access pattern during the given
phase.  The second field specifies whether the access to the region during the
phase should be random or sequential.  `1` means random, `0` means sequential.
The third field specifies access stride size in bytes.  The access stride size
is the interval between access when the access is sequential.  The fourth field
is the probability of the region to the access will be made.  The probability
is relative to other regions, so any number can be given.  The fifth field
specifies whether to do read only (`ro`), write only (`wo`), or both read and
write (`rw`).

This repository contains several default configuration files.  Please refer to
those for usage and format of the file.  'configs/default' would be a good
starting point.


Author
------

SeongJae Park <sj38.park@gmail.com>
