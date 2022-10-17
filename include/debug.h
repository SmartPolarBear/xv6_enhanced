//
// Created by bear on 10/17/2022.
//

#pragma once

#define KDEBUG_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            panic("kernel assertion failed: " #cond); \
        } \
    } while (0)
