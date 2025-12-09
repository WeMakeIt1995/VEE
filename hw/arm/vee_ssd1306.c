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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "hw/misc/vee_ssd1306.h"

#include "vee/common.h"

#define __DEBUG     0
#if ! __DEBUG
#define qemu_log(...)
#endif

/**
 * supported memory addressing mode:
 * horizon, vertical, page
 *
 * supported cmd:
 * display off:         0xae
 * display on:          0xaf
 * display gddram:      0xa4
 * display all on:      0xa5
 * gddram 1 for show:   0xa6
 * gddram 0 for show:   0xa7
 */

enum {
    MemoryAddressingModeHorizontal,
    MemoryAddressingModeVertical,
    MemoryAddressingModePage,
};

#define CMD_LOWER_COLUMN_START                  0x00
#define CMD_LOWER_COLUMN_END                    0x0f
#define CMD_HIGHER_COLUMN_START                 0x10
#define CMD_HIGHER_COLUMN_END                   0x1f
#define CMD_MEMORY_ADDRESSING_MODE_START        0x20
#define CMD_MEMORY_ADDRESSING_MODE_END          0x20
#define CMD_COLUMN_ADDRESS_START                0x21
#define CMD_COLUMN_ADDRESS_END                  0x21
#define CMD_PAGE_ADDRESS_START                  0x22
#define CMD_PAGE_ADDRESS_END                    0x22
#define CMD_DISPLAY_START_LINE_START            0x40
#define CMD_DISPLAY_START_LINE_END              0x7f
#define CMD_CONTRAST_CONTROL_START              0x81
#define CMD_CONTRAST_CONTROL_END                0x81
#define CMD_SEGMENT_REMAP_START                 0xa0
#define CMD_SEGMENT_REMAP_END                   0xa1
#define CMD_ENTIRE_DISPLAY_ON_START             0xa4
#define CMD_ENTIRE_DISPLAY_ON_END               0xa5
#define CMD_NORMAL_DISPLAY_START                0xa6
#define CMD_NORMAL_DISPLAY_END                  0xa7
#define CMD_MULTIPLEX_RATIO_START               0xa8
#define CMD_MULTIPLEX_RATIO_END                 0xa8
#define CMD_DISPLAY_ON_START                    0xae
#define CMD_DISPLAY_ON_END                      0xaf
#define CMD_PAGE_START_ADDRESS_START            0xb0
#define CMD_PAGE_START_ADDRESS_END              0xb7
#define CMD_COM_OUTPUT_SCAN_DIRECTION_START     0xc0
#define CMD_COM_OUTPUT_SCAN_DIRECTION_END       0xc8
#define CMD_DISPLAY_OFFSET_START                0xd3
#define CMD_DISPLAY_OFFSET_END                  0xd3
#define CMD_DISPLAY_CLOCK_DIVIDE_RATIO_START    0xd5
#define CMD_DISPLAY_CLOCK_DIVIDE_RATIO_END      0xd5
#define CMD_PRE_CHARGE_PERIOD_START             0xd9
#define CMD_PRE_CHARGE_PERIOD_END               0xd9
#define CMD_COM_PINS_HARDWARE_CONFIGURATION_START       0xda
#define CMD_COM_PINS_HARDWARE_CONFIGURATION_END         0xda
#define CMD_VCOMH_DESELECT_LEVEL_START          0xdb
#define CMD_VCOMH_DESELECT_LEVEL_END            0xdb
#define CMD_NOP_START                           0xe3
#define CMD_NOP_END                             0xe3

#define CMD_HORIZONTAL_SCROLL_SETUP_START       0x26
#define CMD_HORIZONTAL_SCROLL_SETUP_END         0x27
#define CMD_CONTINUOUS_VERTIVAL_AND_HORIZONTAL_SCROLL_SETUP_START       0x29
#define CMD_CONTINUOUS_VERTIVAL_AND_HORIZONTAL_SCROLL_SETUP_END         0x2a
#define CMD_DEACTIVATE_SCROLL_START             0x2e
#define CMD_DEACTIVATE_SCROLL_END               0x2e
#define CMD_ACTIVATE_SCROLL_START               0x2f
#define CMD_ACTIVATE_SCROLL_END                 0x2f
#define CMD_VERTICAL_SCROLL_AREA_START          0xa3
#define CMD_VERTICAL_SCROLL_AREA_END            0xa3

static inline bool isInRange(int v, int min, int max)
{
    return v >= min && v <= max;
}

#define SSD1306_ON_COLOR        0x0000ffff

VeeSSD1306PixelData_t *vee_ssd1306_get_pixel(VeeSSD1306State *s)
{
    VeeSSD1306PixelData_t *dataStruct = g_malloc(VeeSsd1306DisplayPage * VeeSsd1306DisplayPageHeight * VeeSsd1306DisplayWidth * sizeof(uint32_t));
    dataStruct->width = VeeSsd1306DisplayWidth;
    dataStruct->height = VeeSsd1306DisplayPage * VeeSsd1306DisplayPageHeight;
    dataStruct->pixel = (uint32_t *) g_malloc0(dataStruct->width * dataStruct->height * sizeof(uint32_t));

    if (s->regs.display_on) {
        if (s->regs.display_gddram) {
            if (s->regs.display_inverse) {
                for (int i = 0; i < dataStruct->width * dataStruct->height; i ++) {
                    dataStruct->pixel[i] = s->gddram[i] ? 0 : s->gddram[i];
                }
            }
            else {
                memcpy(dataStruct->pixel, s->gddram, dataStruct->width * dataStruct->height * sizeof(uint32_t));
            }
        }
        else {
            memset(dataStruct->pixel, SSD1306_ON_COLOR, dataStruct->width * dataStruct->height * sizeof(uint32_t));
        }
    }

    return dataStruct;
}

void vee_ssd1306_free_pixel(void *ptr)
{
    g_free(((VeeSSD1306PixelData_t *) ptr)->pixel);
    g_free(ptr);
}

static void vee_ssd1306_rst(VeeSSD1306State *s)
{
    s->cmd_buffer_idx = 0;
    s->regs.column_start = 0;
    s->regs.column_end = 0;
    s->regs.display_gddram = true;
    s->regs.display_inverse = false;
    s->regs.display_on = false;
    s->regs.lower_column_start = 0;
    s->regs.higher_column_start = 0;
    s->regs.memory_addressing_mode = MemoryAddressingModePage;
    s->regs.page_select = 0;
    s->regs.page_start = 0;
    s->regs.page_end = 0;

    memset(s->gddram, 0, sizeof(s->gddram));
}

static void vee_ssd1306_init(Object *obj)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    VeeSSD1306State *s = VEE_SSD1306_STATE(obj);
    object_initialize_child(obj, "pin-rst", &s->pin_rst, TYPE_VEE_PIN_STATE);
    object_initialize_child(obj, "pin-dc", &s->pin_dc, TYPE_VEE_PIN_STATE);

    s->pin_dc.status.direction = VeePinIn;
    s->pin_dc.status.out_voltage_mv = 3300;
    s->pin_rst.status.direction = VeePinIn;
    s->pin_rst.status.out_voltage_mv = 3300;

    vee_ssd1306_rst(s);
}

static void pin_rst_handler(void *opaque)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    VeeSSD1306State *dev = (VeeSSD1306State *) opaque;

    if (! dev->pin_rst.status.in_voltage_mv) {
        vee_ssd1306_rst(dev);
    }
}

static void vee_spi_transmit_byte_handler(void *opaque)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    VeeSSD1306State *dev = (VeeSSD1306State *) opaque;
    VeeSpiState *ss = dev->spi_state;

    if (dev->cmd_buffer_idx < sizeof(dev->cmd_buffer) / sizeof(dev->cmd_buffer[0]) - 1) {
        dev->cmd_buffer[dev->cmd_buffer_idx ++] = ss->transmitStatus.rx_data;
    }
}

static void vee_spi_transmit_start_handler(void *opaque)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);
}

static void vee_spi_transmit_stop_handler(void *opaque)
{
    VeeSSD1306State *dev = (VeeSSD1306State *) opaque;

    qemu_log("qemu: %s %s(), dc vol: %d, recv len: %d \r\n", __FILE__, __FUNCTION__, (int) dev->pin_dc.status.in_voltage_mv, dev->cmd_buffer_idx);
    qemu_log("data: ");
    for (int i = 0; i < dev->cmd_buffer_idx; i ++) {
        qemu_log("%02x ", dev->cmd_buffer[i]);
    }
    qemu_log("\r\n");

    do {
        if (! dev->cmd_buffer_idx) {
            break;
        }

        if (dev->pin_dc.status.in_voltage_mv) {
            qemu_log("qemu: memory_addressing_mode=%d \r\n", dev->regs.memory_addressing_mode);

            switch (dev->regs.memory_addressing_mode) {
            case MemoryAddressingModeHorizontal: {
                int dataIdx = 0;
                for (int i = dev->regs.page_start; i < dev->regs.page_end; i ++) {
                    for (int j = dev->regs.column_start; j < dev->regs.column_end; j ++) {
                        for (int k = 0; k < VeeSsd1306DisplayPageHeight; k ++) {
                            int pos = i * VeeSsd1306DisplayPageHeight * VeeSsd1306DisplayWidth + j + k * VeeSsd1306DisplayWidth;
                            dev->gddram[pos] = (dev->cmd_buffer[dataIdx] >> k) & 1 ? SSD1306_ON_COLOR : 0;
                        }

                        dataIdx ++;
                        if (dataIdx == dev->cmd_buffer_idx) {
                            break;
                        }
                    }

                    if (dataIdx == dev->cmd_buffer_idx) {
                        break;
                    }
                }
            } break;

            case MemoryAddressingModeVertical: {
                int dataIdx = 0;
                for (int i = dev->regs.column_start; i < dev->regs.column_end; i ++) {
                    for (int j = dev->regs.page_start; j < dev->regs.page_end; j ++) {
                        for (int k = 0; k < VeeSsd1306DisplayPageHeight; k ++) {
                            int pos = j * VeeSsd1306DisplayPageHeight * VeeSsd1306DisplayWidth + i + k * VeeSsd1306DisplayWidth;
                            dev->gddram[pos] = (dev->cmd_buffer[dataIdx] >> k) & 1 ? SSD1306_ON_COLOR : 0;
                        }

                        dataIdx ++;
                        if (dataIdx == dev->cmd_buffer_idx) {
                            break;
                        }
                    }

                    if (dataIdx == dev->cmd_buffer_idx) {
                        break;
                    }
                }
            } break;

            case MemoryAddressingModePage: {
                int dataIdx = 0;
                int columnStart = ((dev->regs.higher_column_start & 0xf) << 4) | dev->regs.lower_column_start;
                for (int i = columnStart; i < VeeSsd1306DisplayWidth; i ++) {
                    for (int j = 0; j < VeeSsd1306DisplayPageHeight; j ++) {
                        int pos = dev->regs.page_select * VeeSsd1306DisplayPageHeight * VeeSsd1306DisplayWidth + i + j * VeeSsd1306DisplayWidth;
                        dev->gddram[pos] = (dev->cmd_buffer[dataIdx] >> j) & 1 ? SSD1306_ON_COLOR : 0;

                        // qemu_log("pos=%d, val=%u \r\n", pos, dev->gddram[pos]);
                    }

                    dataIdx ++;
                    if (dataIdx == dev->cmd_buffer_idx) {
                        break;
                    }
                }
            } break;
            }
        }
        else {
            if (isInRange(dev->cmd_buffer[0], CMD_LOWER_COLUMN_START, CMD_LOWER_COLUMN_END)) {
                dev->regs.lower_column_start = dev->cmd_buffer[0] & 0xf;
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_HIGHER_COLUMN_START, CMD_HIGHER_COLUMN_END)) {
                dev->regs.higher_column_start = dev->cmd_buffer[0] & 0x1f;
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_MEMORY_ADDRESSING_MODE_START, CMD_MEMORY_ADDRESSING_MODE_END) &&
                dev->cmd_buffer_idx >= 2) {
                dev->regs.memory_addressing_mode = dev->cmd_buffer[1] & 0x3;
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_PAGE_ADDRESS_START, CMD_PAGE_ADDRESS_END)) {
                if (dev->cmd_buffer_idx >= 2) {
                    dev->regs.page_start = dev->cmd_buffer[1] & 0x7;
                }
                if (dev->cmd_buffer_idx >= 3) {
                    dev->regs.page_end = dev->cmd_buffer[2] & 0x7;
                }
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_COLUMN_ADDRESS_START, CMD_COLUMN_ADDRESS_END)) {
                if (dev->cmd_buffer_idx >= 2) {
                    dev->regs.column_start = dev->cmd_buffer[1] & 0x7;
                }
                if (dev->cmd_buffer_idx >= 3) {
                    dev->regs.column_end = dev->cmd_buffer[2] & 0x7;
                }
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_DISPLAY_ON_START, CMD_DISPLAY_ON_END)) {
                dev->regs.display_on = dev->cmd_buffer[0] == CMD_DISPLAY_ON_START;
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_ENTIRE_DISPLAY_ON_START, CMD_ENTIRE_DISPLAY_ON_END)) {
                dev->regs.display_gddram = dev->cmd_buffer[0] == CMD_ENTIRE_DISPLAY_ON_START;
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_NORMAL_DISPLAY_START, CMD_NORMAL_DISPLAY_END)) {
                dev->regs.display_inverse = dev->cmd_buffer[0] != CMD_NORMAL_DISPLAY_START;
            }
            else if (isInRange(dev->cmd_buffer[0], CMD_PAGE_START_ADDRESS_START, CMD_PAGE_START_ADDRESS_END)) {
                dev->regs.page_select = dev->cmd_buffer[0] & 0x7;
            }
        }
    } while (0);

    dev->cmd_buffer_idx = 0;
}

static void dev_realize(DeviceState *dev, Error **errp) {
    VeeSSD1306State *s = VEE_SSD1306_STATE(dev);

    qemu_log("%s %s() \r\n", __FILE__, __FUNCTION__);

    s->spi_path = vee_str_trim(s->spi_path, "'");
    s->spi_path = vee_str_trim(s->spi_path, "\"");

    bool ambiguous;
    Object *obj = object_resolve_path_type(s->spi_path, TYPE_VEE_SPI_STATE, &ambiguous);
    if (! obj) {
        qemu_log("object_resolve_path_type error: %s \r\n", s->spi_path);
        return;
    }

    qemu_log("%s %s(): object resolve ok \r\n", __FILE__, __FUNCTION__);

    s->spi_state = (VeeSpiState *) object_dynamic_cast(obj, TYPE_VEE_SPI_STATE);

    s->pin_rst.inStateChangeBH = qemu_bh_new(pin_rst_handler, s);
    s->spi_state->transmitByteBH = qemu_bh_new(vee_spi_transmit_byte_handler, s);
    s->spi_state->transmitStartBH = qemu_bh_new(vee_spi_transmit_start_handler, s);
    s->spi_state->transmitStopBH = qemu_bh_new(vee_spi_transmit_stop_handler, s);
}

static Property dev_properties[] = {
    DEFINE_PROP_STRING("vee-spi", VeeSSD1306State, spi_path),
    DEFINE_PROP_END_OF_LIST(),
};

static void vee_ssd1306_class_init(ObjectClass *klass, void *data)
{
    qemu_log("qemu: %s %s() \r\n", __FILE__, __FUNCTION__);

    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = dev_realize;

    device_class_set_props(dc, dev_properties);
}

static const TypeInfo vee_ssd1306_info = {
    .name          = TYPE_VEE_SSD1306_STATE,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(VeeSSD1306State),
    .instance_init = vee_ssd1306_init,
    .class_init    = vee_ssd1306_class_init,
};

static void vee_ssd1306_register_types(void)
{
    type_register_static(&vee_ssd1306_info);
}

type_init(vee_ssd1306_register_types)
