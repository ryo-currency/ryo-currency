# Debugging

This section contains general instructions for debugging failed installs or problems encountered with Ryo. First ensure you are running the latest version built from the github repo.

### Obtaining stack traces and core dumps on Unix systems

We generally use the tool `gdb` (GNU debugger) to provide stack trace functionality, and `ulimit` to provide core dumps in builds which crash or segfault.

* To use gdb in order to obtain a stack trace for a build that has stalled:

Run the build.

Once it stalls, enter the following command:

```
gdb /path/to/ryod `pidof ryod`
```

Type `thread apply all bt` within gdb in order to obtain the stack trace

* If however the core dumps or segfaults:

Enter `ulimit -c unlimited` on the command line to enable unlimited filesizes for core dumps

Enter `echo core | sudo tee /proc/sys/kernel/core_pattern` to stop cores from being hijacked by other tools

Run the build.

When it terminates with an output along the lines of "Segmentation fault (core dumped)", there should be a core dump file in the same directory as ryod. It may be named just `core`, or `core.xxxx` with numbers appended.

You can now analyse this core dump with `gdb` as follows:

`gdb /path/to/ryod /path/to/dumpfile`

Print the stack trace with `bt`

* To run ryo within gdb:

Type `gdb /path/to/ryod`

Pass command-line options with `--args` followed by the relevant arguments

Type `run` to run ryod

### Analysing memory corruption

We use the tool `valgrind` for this.

Run with `valgrind /path/to/ryod`. It will be slow.