# OUR IMPLEMENTATION

## BUILDING AND RUNNING XV6

- `make clean`: Remove all generated files
- `make qemu`: Run xv6 in QEMU with the default scheduler
- `make qemu-our`: Run xv6 with the custom scheduler implementation

## SCHEDULER IMPLEMENTATION

1. **Default Round-Robin Scheduler**: The default scheduler in proc.c implements a simple round-robin policy, where each runnable process gets a turn to execute.

2. **Lottery Scheduler**: An alternative implementation in proc_our.c uses a lottery scheduling algorithm where processes are assigned tickets, and the scheduler randomly selects a process to run based on the number of tickets it has.

## HOW TO CHANGE SCHEDULER

The scheduler can be changed in two ways:

1. **Using the Makefile Flag**:
   - To use the default scheduler: `make qemu`
   - To use the lottery scheduler: `make qemu-our`

The Makefile uses the `USE_OUR_PROC` flag to determine which scheduler implementation to use:

   ```makefile
   ifdef USE_OUR_PROC
   PROC_SOURCE = $K/proc_our.c
   else
   PROC_SOURCE = $K/proc.c
   endif
   ```

2. **Modifying the Scheduler Code**:
   - To modify the default scheduler, edit `kernel/proc.c`
   - To modify the lottery scheduler, edit `kernel/proc_our.c`
   - The scheduler implementation is in the `scheduler()` function in these files

## PROCESS PRIORITY

The system supports process priorities through the following system calls:

- `getPriority()`: Returns the priority of the current process
- `setPriority(pid, priority)`: Sets the priority of the specified process

Priorities range from 0 to 20, with higher values indicating higher priority. Only a parent process can set the priority of its child processes.

## TESTING

The system includes test programs to verify functionality:

- `priotest`: Tests the priority system calls
- `cstest`: Tests context switching between processes

Run these tests by executing the corresponding commands in the xv6 shell:

```bash
$ priotest
$ cstest
```

---

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Takahiro Aoyagi, Marcelo Arroyo, Silas Boyd-Wickizer, Anton Burtsev,
carlclone, Ian Chen, Dan Cross, Cody Cutler, Mike CAT, Tej Chajed,
Asami Doi,Wenyang Duan, eyalz800, Nelson Elhage, Saar Ettinger, Alice
Ferrazzi, Nathaniel Filardo, flespark, Peter Froehlich, Yakir Goaron,
Shivam Handa, Matt Harvey, Bryan Henry, jaichenhengjie, Jim Huang,
Matúš Jókay, John Jolly, Alexander Kapshuk, Anders Kaseorg, kehao95,
Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt, Eddie Kohler, Vadim
Kolontsov, Austin Liew, l0stman, Pavan Maddamsetti, Imbar Marinescu,
Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi Merimovich, Mark
Morrissey, mtasm, Joel Nider, Hayato Ohhashi, OptimisticSide,
phosphagos, Harry Porter, Greg Price, RayAndrew, Jude Rich, segfault,
Ayan Shafqat, Eldar Sehayek, Yongming Shen, Fumiya Shigemitsu, snoire,
Taojie, Cam Tenny, tyfkda, Warren Toomey, Stephen Tu, Alissa Tung,
Rafael Ubal, Amane Uehara, Pablo Ventura, Xi Wang, WaheedHafez,
Keiichi Watanabe, Lucas Wolf, Nicolas Wolovick, wxdao, Grant Wu, x653,
Jindong Zhang, Icenowy Zheng, ZhUyU1997, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2024 Frans Kaashoek, Robert Morris, and Rus``makefile
   ifdef USE_OUR_PROC
   PROC_SOURCE = $K/proc_our.c
   else
   PROC_SOURCE = $K/proc.c
   endif
   ```s Cox.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu).  The main purpose of xv6 is as a teaching
operating system for MIT's 6.1810, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu.  Once they are installed, and in your shell
search path, you can run "make qemu".

