# TIS-100-CXX
A TIS-100 simulator and save validator.

This is a validator for saves for the game
[TIS-100](https://zachtronics.com/tis-100/), designed to be used for the 
[community-run leaderboard](https://www.reddit.com/r/tis100/wiki/index). Unlike
other simulators, TIS-100-CXX includes definitions for the base game's levels
and can generate suitable random tests for them. This simulator aims for exact
parity with the game in both execution and test generation.

## Build instructions:

Ensure you have a C++23 compliant compiler (clang 19+, gcc 14+).

1. clone repository
2. update submodules: `git submodule update --init --recursive`
2. run `cmake -B "path/to/some/build/dir" -S .`
3. run `cmake --build "path/to/some/build/dir"`

TIS-100-CXX only has two dependencies, which are header-only and will be found
and used automatically, so no further management is needed beyond the above
steps.

## Run instructions:

Invocation:
`TIS-100-CXX [options] path/to/solution1.txt path/to/solution2.txt ...`
The level is autodeduced from the filename prefix, if this is not possible
use an option to give it explicitely.

By default, it will run the simulation and if it passes, it will print
"validation successful" and a score, in the format 
`cycles/nodes/instructions/flags`, where flags may be `a` for achievement,
`c` for cheat, i.e. a solution which does not pass all random tests,
and `h` for a solution that passes < 5% of the random tests
(meaning that it made no real effort to pass any). "Passing" for a random test
means doing it in less than 5x as many cycles as the slowest fixed test (see
--limit-multiplier below).

If it does not pass, it will instead print the inputs and outputs
(expected and received) for the failed test, then
"validation failed" with some more information, and finally a score in which
the cycles count is replaced by `-` to denote a failure. The return value is `0`
on a validation, including a cheated solution, `1` on a validation failure,
and `2` on an exception.

For options `--limit`, `--total-limit`, `--random`, `--seed`, `--seeds`,
and `--T30_size`, integer arguments can be specified with a scale suffix,
either K, M, or B (case-insensitive) for thousand, million, or billion
respectively.

The most useful options are:
- `-l segment` is the name of a segment as it appears in game, either the
  numeric ID or the human-readable name (case-sensitive). For example, `"00150"`
  and `"SELF-TEST DIAGNOSTIC"` are equivalent. There is a complete list of these
  in the `--help` output, but you will most likely find it easier to get them
  from the game or from the solution's filename. 
- `--limit N`: set the timeout limit for the simulation. Default `100500`
  (enough for BUSY_LOOP with a little slack).
- `--seeds L..H`: a comma-separated list of integer ranges, such as `0..99`.
  Ranges are inclusive on both sides. Can also specify an individual integer,
  meaning a range of just that integer. Can be specified multiple times, which
  is equivalent to concatenating the arguments with a comma.
- `-r N` and `--seed S`: an alternate way to specify random tests, cannot be
  combined with `--seeds`. If `-r` is specified by itself, a starting seed will
  be selected at random. In any case, a contiguous range of N seeds starting at
  S will be used for random tests, except for EXPOSURE MASK VIEWER which may
  skip some seeds.
- `--loglevel LEVEL`, `--debug`, `--trace`, `--info`: set the amount of
  information logged to stderr. The default log level is "notice", which
  corresponds to only important information. "info" includes information that
  may be useful but is not always important, and notably the amount of
  information logged at level "info" is bounded. "trace" includes a printout
  of the board state at each cycle. "debug" includes a full trace of the execution
  in the log and will often produce multiple MB of data.
- `-j N`: run random tests with N worker threads. With `-j 0`, the number of
  hardware threads is detected and used.
- `--fixed 0`: disable fixed tests, run only random tests. This affects scoring,
  as normally random tests do not contribute to scoring except for /c and /h
  flags, but with this flag, the reported score will be the worst observed
  score.
- `-q`, `--quiet`: reduce the amount of human-readable text printed around the
  information. May be specified twice to remove almost all supplemental text.
  
Other options:
- `--T21_size N` and `--T30_size M`: override the default size limits on
  instructions in any particular T21 node and values in any particular T30 node
  respectively.
- `--cheat-rate C`: change the threshold between /c and /h to any proportion in
  the range 0-1. The default value is .05, or 5%.
- `-k N`, `--limit-multiplier N`: change the scale factor for dynamic timeouts for random
  tests (will not exceed --limit). Higher values are more lenient toward slow
  random tests. The default value is 5, meaning that if a solution takes 100
  cycles to pass the slowest fixed test, it will time out after 500 cycles on
  random tests (even when that is less than --limit).
- `-c`, `--color`: force color for important info even when redirecting output
- `-C`, `--log-color`: force color for logs even when redirecting stderr
- `-S`, `--stats`: run all requested random tests and report the pass rate at
  the end. Without this flag, the sim will quit as soon as it can label a
  solution /c (that is, at least 5% of requested tests passed and at least one
  failed).
- `-L ID`, `--level ID`: As an alternative to specifying the name or segment as
  described above, the numeric ID (0..50) can be used instead.
- `--dry-run`: Mainly useful for debugging the command-line parser and initial
  setup. Checks the command line as normal and quits before running any tests.
  
There are two shell scripts distributed with the sim, mainly intended for
testing the sim itself, test_saves_single.sh and test_saves_lb.sh, both of
which require [fish](https://fishshell.com/) to run. test_saves_single.sh can
be used to consume an entire folder of solutions, such as the saves folder used
by the official game, and simulate each of them. test_saves_lb.sh can be used
to consume the hierarchical folder structure used by the leaderboard.

In addition to all options accepted by the sim itself, these scripts both
accept options -d, which identifies the top-level folder to consume (required);
-s, -w, and -f, which identify files to write reports to, for success (score
and flags correct), wrong scores, and failed validations (including wrong
scores if -w is not separately specified), respectively; -a, which identifies a
file to receive a report of each score (like -s and -f pointing to the same
file, but fully independent of those flags); -n, which is an abbreviation of
--fixed 0; and -i, which prompts for input after each test. Note that fish's
argparse does not always accept spaces between flags and option values.
