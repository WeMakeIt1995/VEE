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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/misc/vee_pin.h"

#ifndef STM_ERR_DEBUG
#define STM_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#if ! STM_ERR_DEBUG
#define qemu_log(...)
#endif

static void vee_pin_init(Object *obj)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    VeePinState *s = VEE_PIN_STATE(obj);

    s->has_extern_circuit = false;
    s->status.direction = VeePinHighImpedance;
}

static void vee_pin_class_init(ObjectClass *klass, void *data)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);
}

static const TypeInfo vee_pin_info = {
    .name          = TYPE_VEE_PIN_STATE,
    .parent        = TYPE_OBJECT,
    .instance_size = sizeof(VeePinState),
    .instance_init = vee_pin_init,
    .class_init    = vee_pin_class_init,
};

static void vee_pin_register_types(void)
{
    type_register_static(&vee_pin_info);
}

type_init(vee_pin_register_types)
