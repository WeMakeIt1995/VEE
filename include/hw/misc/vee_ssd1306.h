/*
 * VEE SSD1306
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

#ifndef HW_VEE_SSD1306_H
#define HW_VEE_SSD1306_H

#include "qom/object.h"
#include "qemu/queue.h"

#include "hw/misc/vee_spi.h"

#define TYPE_VEE_SSD1306_STATE "vee-ssd1306"
OBJECT_DECLARE_SIMPLE_TYPE(VeeSSD1306State, VEE_SSD1306_STATE)

typedef uint32_t VeeSSD1306Pixel_t;

#define VeeSsd1306DisplayWidth              128
#define VeeSsd1306DisplayPage               8
#define VeeSsd1306DisplayPageHeight         8

typedef struct VeeSSD1306State {
    DeviceState     parent;

    char           *spi_path;
    VeeSpiState    *spi_state;

    uint8_t         cmd_buffer[4096];
    int             cmd_buffer_idx;

    VeePinState     pin_rst,
                    pin_dc;

    struct {
        int     memory_addressing_mode,
                lower_column_start,
                higher_column_start,
                page_start,
                page_end,
                page_select,
                column_start,
                column_end,
                display_on,
                display_gddram,
                display_inverse;
    } regs;

    VeeSSD1306Pixel_t       gddram[VeeSsd1306DisplayPage * VeeSsd1306DisplayPageHeight * VeeSsd1306DisplayWidth];
} VeeSSD1306State;

typedef struct {
    int     width;
    int     height;

    uint32_t       *pixel;
} VeeSSD1306PixelData_t;

VeeSSD1306PixelData_t *vee_ssd1306_get_pixel(VeeSSD1306State *s);

void vee_ssd1306_free_pixel(void *ptr);

#endif
