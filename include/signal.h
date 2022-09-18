//
// Created by bear on 9/18/2022.
//
#pragma once

#define    NSIGNALS (32)

#define    SIGHUP    1    /* hangup */
#define    SIGINT    2    /* interrupt */
#define    SIGQUIT   3    /* quit */
#define    SIGILL    4    /* illegal instruction (not reset when caught) */

#define    SIGABRT   6    /* abort() */

#define    SIGFPE    8    /* floating point exception */
#define    SIGKILL    9    /* kill (cannot be caught or ignored) */

#define    SIGSEGV    11    /* segmentation violation */

#define    SIGPIPE    13    /* write on a pipe with no one to read it */
#define    SIGALRM    14    /* alarm clock */
#define    SIGTERM    15    /* software termination signal from kill */

#define    SIGSTOP    17    /* sendable stop signal not from tty */
#define    SIGTSTP    18    /* stop signal from tty */
#define    SIGCONT    19    /* continue a stopped process */
#define    SIGCHLD    20    /* to parent on child stop or exit */
#define    SIGTTIN    21    /* to readers pgrp upon background tty read */
#define    SIGTTOU    22    /* like TTIN for output if (tp->t_local&LTOSTOP) */

#define    SIGUSR1    30    /* user defined signal 1 */
#define    SIGUSR2    31    /* user defined signal 2 */

