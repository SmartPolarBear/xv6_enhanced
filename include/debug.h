//
// Created by bear on 10/17/2022.
//

#pragma once

extern void panic(char *) __attribute__((noreturn));

#define KDEBUG_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            panic("kernel assertion failed: " #cond); \
        } \
    } while (0)

#define KDEBUG_MSG_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            panic("kernel assertion failed: " #cond msg); \
        } \
    } while (0)

#define KDEBUG_UNREACHABLE  \
    KDEBUG_MSG_ASSERT(FALSE, "Unreachable")
