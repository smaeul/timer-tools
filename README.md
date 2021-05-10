# ARM Architectural Timer Tools

This is a cleaned-up version of some of the tests used to investigate the
Allwinner A64 timer bug. Usage:

```
usage: timer_test -CcTt [-d DURATION] [-p] [-s]

  -C  run parallel counter test
  -c  run random counter test
  -T  run parallel timer test
  -t  run random timer test

  -d  test duration (seconds)
  -p  use physical counter
  -s  skip errors caught by kernel workaround
```

The virtual counter test can be run without any special permissions. The
physical counter test (`-p`) and the timer tests (`-Tt`) require enabling
userspace access to the CNTPCT or CVAL/TVAL registers, respectively, using the
included kernel module (see below).

Note that there is no option to run a physical timer test, because that timer
is actively used by the Linux kernel, and modifying it would cause instability.

## Building

To build the test program, just run `make`. If you are cross-compiling, define
`CROSS_COMPILE` with the toolchain prefix as usual.

To build the out-of-tree kernel module, run `make -C /path/to/linux/src
M=$PWD/module` (i.e. run `make` from your Linux source directory, and point it
to the `module` directory). You can load the module with `insmod
arch_timer_control.ko`.

## Unlocking register access

Access to various timer registers from EL0 is controlled by the `CNTKCTL_EL1`
system register. The relevant bits are listed below (taken from the ARMv8 ARM):

| Bit | Meaning                                  |
|-----|------------------------------------------|
|   0 | Allow userspace access to `CNTPCT_EL0`   |
|   1 | Allow userspace access to `CNTVCT_EL0`   |
|   8 | Allow userspace access to `CNTV_[CT]VAL` |
|   9 | Allow userspace access to `CNTP_[CT]VAL` |

The other bits control the event stream, and have no effect on the tests.

To modify these bits, use the file provided by the kernel module:

```
# insmod arch_timer_control.ko
[ 7870.050674] arch_timer_control: loading out-of-tree module taints kernel.
[ 7870.067838] Current workaround: Allwinner erratum UNKNOWN1 (ool_workarounds+0x0/0x50)
# cat /sys/kernel/arch_timer/cntkctl_el1
00000000000000a4
# ./timer_test -cpd1
Running random counter test...
Illegal instruction
# echo 3a7 > /sys/kernel/arch_timer/cntkctl_el1
# cat /sys/kernel/arch_timer/cntkctl_el1
00000000000003a7
# ./timer_test -cpd1
Running random counter test...
0: Failed after 16003218 reads (0.482697 s)
0: 0x0000002d01cdffff → 0x0000002d01cdc000 → 0x0000002d01ce0000 (    -0.683 ms)
0: 0x0000002d01dc7fff → 0x0000002d01dc4000 → 0x0000002d01dc8000 (    -0.683 ms)
0: 0x0000002d01dcffff → 0x0000002d01dcc000 → 0x0000002d01dd0000 (    -0.683 ms)
0: 0x0000002d01deffff → 0x0000002d01dec000 → 0x0000002d01df0000 (    -0.683 ms)
0: 0x0000002d01e4ffff → 0x0000002d01e4c000 → 0x0000002d01e50000 (    -0.683 ms)
0: 0x0000002d01e97fff → 0x0000002d01e94000 → 0x0000002d01e98000 (    -0.683 ms)
0: 0x0000002d01f77fff → 0x0000002d01f74000 → 0x0000002d01f78000 (    -0.683 ms)
0: 0x0000002d01f87fff → 0x0000002d01f84000 → 0x0000002d01f88000 (    -0.683 ms)
0: 0x0000002d01f97fff → 0x0000002d01f94000 → 0x0000002d01f98000 (    -0.683 ms)
0: 0x0000002d01fd7fff → 0x0000002d01fd4000 → 0x0000002d01fd8000 (    -0.683 ms)
0: Finished. 31870355 tries (31870355/s), 10 fails, 0 skips
```

Bit 1 is cleared when a timer workaround is in effect, allowing the kernel to
trap and emulate reads from `CNTVCT_EL0`. Setting this bit thus disables the
workaround for direct timer reads. However, it does not affect the workaround
for `clock_gettime` and kernel-internal timekeeping.

When the workaround is disabled, use the `-s` option to perform the same
filtering that the kernel would do.
