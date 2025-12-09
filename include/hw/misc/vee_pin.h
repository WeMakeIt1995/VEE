/*
 * VEE PIN
 *
 * Copyright (c) 2025 Jiehan Guo <1003425554@qq.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_VEE_PIN_H
#define HW_VEE_PIN_H

#include "qemu/main-loop.h"
#include "qom/object.h"

typedef enum {
    VeePinHighImpedance,
    VeePinOut,
    VeePinIn,
} VeePinDirection;

#define TYPE_VEE_PIN_STATE "vee-pin-state"
OBJECT_DECLARE_SIMPLE_TYPE(VeePinState, VEE_PIN_STATE)

typedef struct VeePinState {
    ObjectClass     parent;

    struct {
        VeePinDirection     direction;

        long        out_voltage_mv;
        long        out_current_ma;

        long        in_voltage_mv;
    } status;

    bool        has_extern_circuit;

    QEMUBH     *outStateChangeBH, *inStateChangeBH;
} VeePinState;

typedef struct VeePinStateListItem {
    QSIMPLEQ_ENTRY(VeePinStateListItem)     list_entry;

    VeePinState    *state;
} VeePinStateListItem;

typedef QSIMPLEQ_HEAD(VeePinStateList, VeePinStateListItem) VeePinStateList;

#endif
