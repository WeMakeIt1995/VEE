/*
 * VEE LINE
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
#include "hw/qdev-properties.h"
#include "hw/misc/vee_line.h"

#include "vee/common.h"

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

static void vee_line_init(Object *obj)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);
}

static void vee_pin_out_state_change_handler(void *opaque)
{
    // qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    VeeLineState *ls = (VeeLineState *) opaque;

    long min_mv = 9e6;
    VeePinStateListItem *item = NULL;
    QSIMPLEQ_FOREACH(item, &ls->veePinStateList, list_entry) {
        // if (item->state->status.direction != VeePinOut) {
        //     continue;
        // }
        if (item->state->status.out_voltage_mv < min_mv) {
            min_mv = item->state->status.out_voltage_mv;
        }
    }
    QSIMPLEQ_FOREACH(item, &ls->veePinStateList, list_entry) {
        if (min_mv == item->state->status.in_voltage_mv) {
            continue;
        }

        item->state->status.in_voltage_mv = min_mv;

        if (item->state->inStateChangeBH) {
            aio_bh_call(item->state->inStateChangeBH);
        }
    }

    char *id = object_get_canonical_path(OBJECT(ls));
    qemu_log("%s: line out changed: id=%s, min_mv=%d \r\n",
        __FILE__, id, (int) min_mv
    );
    if (id) {
        g_free(id);
    }
}

static void dev_realize(DeviceState *dev, Error **errp) {
    VeeLineState *s = VEE_LINE_STATE(dev);

    qemu_log("%s %s() \r\n", __FILE__, __FUNCTION__);

    QSIMPLEQ_INIT(&s->veePinStateList);

    s->vee_pins_path = vee_str_trim(s->vee_pins_path, "'");
    s->vee_pins_path = vee_str_trim(s->vee_pins_path, "\"");

    char **paths = g_strsplit(s->vee_pins_path, ",", 256);

    for (int i = 0; paths[i] != NULL; i ++) {
        qemu_log("%s %s(): vee_pin_path=%s \r\n", __FILE__, __FUNCTION__, paths[i]);

        bool ambiguous;
        Object *obj = object_resolve_path_type(paths[i], TYPE_VEE_PIN_STATE, &ambiguous);
        if (! obj) {
            qemu_log("object_resolve_path_type error");
            continue;
        }

        qemu_log("%s %s(): object resolve ok \r\n", __FILE__, __FUNCTION__);

        VeePinStateListItem *item = g_malloc0(sizeof(VeePinStateListItem));
        item->state = (VeePinState *) object_dynamic_cast(obj, TYPE_VEE_PIN_STATE);
        item->state->has_extern_circuit = true;

        item->state->outStateChangeBH = qemu_bh_new(vee_pin_out_state_change_handler, s);

        QSIMPLEQ_INSERT_TAIL(&s->veePinStateList, item, list_entry);

        qemu_log("%s %s(): object inserted \r\n", __FILE__, __FUNCTION__);
    }

    g_strfreev(paths);
}

static Property dev_properties[] = {
    DEFINE_PROP_STRING("vee-pins-path", VeeLineState, vee_pins_path),
    DEFINE_PROP_END_OF_LIST(),
};

static void vee_line_class_init(ObjectClass *klass, void *data)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = dev_realize;

    device_class_set_props(dc, dev_properties);
}

static const TypeInfo vee_line_info = {
    .name          = TYPE_VEE_LINE_STATE,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(VeeLineState),
    .instance_init = vee_line_init,
    .class_init    = vee_line_class_init,
};

static void vee_line_register_types(void)
{
    type_register_static(&vee_line_info);
}

type_init(vee_line_register_types)
