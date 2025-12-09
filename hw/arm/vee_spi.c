/*
 * VEE SPI
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
#include "hw/misc/vee_spi.h"

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

static void cs_handler(void *opaque)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    VeeSpiState *ss = (VeeSpiState *) opaque;

    if (ss->pin_cs.status.in_voltage_mv) {
        ss->transmitStatus.bits_remain = 8;

        qemu_log("qemu: %s %s(), emit stop \r\n", __FILE__, __FUNCTION__);

        if (ss->transmitStopBH) {
            aio_bh_call(ss->transmitStopBH);
        }
    }
    else {
        ss->transmitStatus.tx_shift_buffer = ss->transmitStatus.tx_data;

        if (! ss->pin_sck.status.in_voltage_mv) {
            ss->pin_miso.status.out_voltage_mv = ss->transmitStatus.tx_data & (1 << (ss->transmitStatus.bits_remain - 1)) ?
                ss->pin_voltage_mv : 0;

            if (ss->pin_miso.outStateChangeBH) {
                aio_bh_call(ss->pin_miso.outStateChangeBH);
            }
        }

        qemu_log("qemu: %s %s(), emit start \r\n", __FILE__, __FUNCTION__);

        if (ss->transmitStartBH) {
            aio_bh_call(ss->transmitStartBH);
        }
    }
}

static void sck_handler(void *opaque)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    VeeSpiState *ss = (VeeSpiState *) opaque;

    if (ss->pin_cs.status.in_voltage_mv) {
        return;
    }

    if (ss->pin_sck.status.in_voltage_mv) {
        ss->transmitStatus.rx_shift_buffer |= (ss->pin_mosi.status.in_voltage_mv ? 1 : 0) << (ss->transmitStatus.bits_remain - 1);
        ss->transmitStatus.bits_remain --;

        if (! ss->transmitStatus.bits_remain) {
            ss->transmitStatus.bits_remain = 8;
            ss->transmitStatus.rx_data = ss->transmitStatus.rx_shift_buffer;
            ss->transmitStatus.rx_shift_buffer = 0;
            ss->transmitStatus.tx_shift_buffer = ss->transmitStatus.tx_data;

            if (ss->transmitByteBH) {
                aio_bh_call(ss->transmitByteBH);
            }
        }
    }
    else {
        ss->pin_miso.status.out_voltage_mv = ss->transmitStatus.tx_shift_buffer & (1 << (ss->transmitStatus.bits_remain - 1)) ?
            ss->pin_voltage_mv : 0;

        if (ss->pin_miso.outStateChangeBH) {
            aio_bh_call(ss->pin_miso.outStateChangeBH);
        }
    }
}

static void vee_spi_init(Object *obj)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    VeeSpiState *s = VEE_SPI_STATE(obj);

    object_initialize_child(obj, "pin-cs", &s->pin_cs, TYPE_VEE_PIN_STATE);
    object_initialize_child(obj, "pin-sck", &s->pin_sck, TYPE_VEE_PIN_STATE);
    object_initialize_child(obj, "pin-miso", &s->pin_miso, TYPE_VEE_PIN_STATE);
    object_initialize_child(obj, "pin-mosi", &s->pin_mosi, TYPE_VEE_PIN_STATE);

    s->pin_cs.status.direction = VeePinIn;
    s->pin_cs.status.out_voltage_mv = s->pin_voltage_mv;
    s->pin_sck.status.direction = VeePinIn;
    s->pin_sck.status.out_voltage_mv = s->pin_voltage_mv;
    s->pin_miso.status.direction = VeePinOut;
    s->pin_miso.status.out_voltage_mv = s->pin_voltage_mv;
    s->pin_mosi.status.direction = VeePinIn;
    s->pin_mosi.status.out_voltage_mv = s->pin_voltage_mv;

    s->pin_cs.inStateChangeBH = qemu_bh_new(cs_handler, s);
    s->pin_sck.inStateChangeBH = qemu_bh_new(sck_handler, s);
}

static Property dev_properties[] = {
    DEFINE_PROP_UINT32("vee-pin-voltage-mv", VeeSpiState, pin_voltage_mv, 3300),
    DEFINE_PROP_END_OF_LIST(),
};

static void vee_spi_class_init(ObjectClass *klass, void *data)
{
    qemu_log("qemu: %s() %d \r\n", __FUNCTION__, __LINE__);

    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, dev_properties);

    qemu_log("qemu: %s() %d \r\n", __FUNCTION__, __LINE__);
}

static const TypeInfo vee_spi_info = {
    .name          = TYPE_VEE_SPI_STATE,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(VeeSpiState),
    .instance_init = vee_spi_init,
    .class_init    = vee_spi_class_init,
};

static void vee_spi_register_types(void)
{
    type_register_static(&vee_spi_info);
}

type_init(vee_spi_register_types)
