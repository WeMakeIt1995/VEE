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

#ifndef HW_VEE_SPI_H
#define HW_VEE_SPI_H

#include "qemu/main-loop.h"
#include "qom/object.h"
#include "hw/misc/vee_pin.h"

#define TYPE_VEE_SPI_STATE "vee-spi"
OBJECT_DECLARE_SIMPLE_TYPE(VeeSpiState, VEE_SPI_STATE)

typedef struct VeeSpiState {
    DeviceState     parent;

    uint32_t        pin_voltage_mv;
    VeePinState     pin_cs, pin_sck, pin_miso, pin_mosi;

    struct {
        uint32_t    bits_remain;

        uint32_t    tx_data, rx_data;
        uint32_t    tx_shift_buffer, rx_shift_buffer;
    } transmitStatus;

    QEMUBH     *transmitByteBH,
               *transmitStartBH,
               *transmitStopBH;
} VeeSpiState;

#endif
