/*
 * VEE STM32F4XX SPI
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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
#include "qapi/error.h"
#include "hw/misc/vee_stm32f4xx_spi.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"

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

static void clock_handler(void *opaque);

static void stm32f4xx_spi_reset(DeviceState *dev)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    VEE_STM32F4XXSPIState *s = (VEE_STM32F4XXSPIState *) dev;

    // s->spi_cr1 = 0x00000000;
    // s->spi_cr2 = 0x00000000;
    // s->spi_sr = 0x0000000A;
    // s->spi_dr = 0x0000000C;
    // s->spi_crcpr = 0x00000007;
    // s->spi_rxcrcr = 0x00000000;
    // s->spi_txcrcr = 0x00000000;
    // s->spi_i2scfgr = 0x00000000;
    // s->spi_i2spr = 0x00000002;

    s->regs[StmSpiCR2] =        0x00000000;
    s->regs[StmSpiCR1] =        0x00000000;
    s->regs[StmSpiSR] =         0x00000002;
    s->regs[StmSpiDR] =         0x00000000;
    s->regs[StmSpiCRCPR] =      0x00000007;
    s->regs[StmSpiRxCRCR] =     0x00000000;
    s->regs[StmSpiTxCRCR] =     0x00000000;
    s->regs[StmSpiI2SCFGR] =    0x00000000;
    s->regs[StmSpiI2SPR] =      0x00000002;
}

#pragma pack(1)

typedef struct {
    uint32_t    cpha: 1,
                cpol: 1,
                mstr: 1,
                br: 3,
                spe: 1,
                lsb_first: 1,
                ssi: 1,
                ssm: 1,
                rx_only: 1,
                dff: 1,
                crc_next: 1,
                crc_en: 1,
                bidi_oe: 1,
                bidi_mode: 1;
} CR1Fields;

typedef struct {
    uint32_t    rx_dma_en: 1,
                tx_dma_en: 1,
                ssoe: 1,
                : 1,
                frf: 1,
                errie: 1,
                rxneie: 1,
                txeie: 1;
} CR2Fields;

typedef struct {
    uint32_t    rxne: 1,
                txe: 1,
                chside: 1,
                udr: 1,
                crc_err: 1,
                modf: 1,
                ovr: 1,
                bsy: 1,
                fre: 1;
} SRFields;

#pragma pack()

static uint64_t stm32f4xx_spi_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    VEE_STM32F4XXSPIState *s = opaque;

    addr >>= 2;

    if (addr == StmSpiDR) {
        SRFields *sr = (SRFields *) &s->regs[StmSpiSR];
        if (sr->rxne) {
            sr->rxne = 0;
        }
    }

    return s->regs[addr];
}

static void stm32f4xx_spi_write(void *opaque, hwaddr addr,
                                uint64_t val64, unsigned int size)
{
    qemu_log("qemu: %s(), opaque=%p, addr=%lld(0x%llx), val64=%lld(0x%llx), size=%u \r\n",
             __FUNCTION__, opaque, (long long) addr, (long long) addr, (long long) val64, (long long) val64, size
    );

    VEE_STM32F4XXSPIState *s = opaque;

    addr >>= 2;

    switch (addr) {
    case StmSpiCR1: {
        CR1Fields last_cr1 = * (CR1Fields *) &s->regs[StmSpiCR1];
        s->regs[addr] = val64;
        CR1Fields cr1 = * (CR1Fields *) &s->regs[StmSpiCR1];

        if (last_cr1.spe && (! cr1.spe)) {
            memset(&s->transmitStatus, 0, sizeof(s->transmitStatus));
            break;
        }
        if (cr1.spe) {
            if (cr1.ssm) {
                s->afio_cs->level = cr1.ssi;

                if (s->afio_cs->bh) {
                    aio_bh_call(s->afio_cs->bh);
                }
            }
        }
    } break;

    case StmSpiCR2: {
        s->regs[addr] = val64;
    } break;

    case StmSpiSR: break;

    case StmSpiDR: {
        qemu_log("qemu: %s() %d: write dr 0x%08x \r\n", __FUNCTION__, __LINE__, (uint32_t) val64);

        CR1Fields *cr1 = (CR1Fields *) &s->regs[StmSpiCR1];
        if (! cr1->spe) {
            break;
        }
        SRFields *sr = (SRFields *) &s->regs[StmSpiSR];
        if (! sr->txe) {
            break;
        }
        if (! sr->bsy) {
            sr->bsy = 1;
            s->transmitStatus.tx_data = val64 & 0xffff;
            s->transmitStatus.bits_remain = cr1->dff ? 16 : 8;

            // clock_propagate(s->clock);

            timer_mod(&s->clockTimer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 1);
        }
        else {
            sr->txe = 0;
            s->transmitStatus.tx_buffer = val64 & 0xffff;
            s->transmitStatus.tx_buffer_valid = 1;
        }
    } break;

    case StmSpiCRCPR:
    case StmSpiRxCRCR:
    case StmSpiTxCRCR:
    case StmSpiI2SCFGR:
    case StmSpiI2SPR: break;
    default: break;
    }
}

static const MemoryRegionOps stm32f4xx_spi_ops = {
    .read = stm32f4xx_spi_read,
    .write = stm32f4xx_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void clock_handler(void *opaque)
{
    {
        static int count = 8;
        if (count > 0) {
            qemu_log("qemu: %s(), opaque=%p \r\n", __FUNCTION__, opaque);
            count --;
        }
    }

    VEE_STM32F4XXSPIState *s = opaque;

    // if (event == ClockPreUpdate) {
    //     return;
    // }

    SRFields *sr = (SRFields *) &s->regs[StmSpiSR];
    if (! sr->bsy) {
        return;
    }

    CR1Fields *cr1 = (CR1Fields *) &s->regs[StmSpiCR1];
    // irq not support with this version
    // CR2Fields *cr2 = (CR2Fields *) &s->regs[StmSpiCR2];

    while (s->transmitStatus.bits_remain) {
        if (s->afio_sck->level) {
            if (s->afio_miso->connectedGpioState) {
                VeePinState *misoPinState = &s->afio_miso->connectedGpioState->pinsState[s->afio_miso->connectedGpioPin];
                s->transmitStatus.rx_data |= (misoPinState->status.in_voltage_mv ? 1 : 0) << (s->transmitStatus.bits_remain - 1);
            }
            s->transmitStatus.bits_remain --;

            if (! s->transmitStatus.bits_remain) {
                if (s->transmitStatus.tx_buffer_valid) {
                    s->transmitStatus.tx_buffer_valid = 0;
                    s->transmitStatus.tx_data = s->transmitStatus.tx_buffer;
                    s->transmitStatus.bits_remain = cr1->dff ? 16 : 8;

                    sr->txe = 1;
                }
                else {
                    sr->bsy = 0;
                }
                s->regs[StmSpiDR] = s->transmitStatus.rx_data;
                s->transmitStatus.rx_data = 0;

                sr->rxne = 1;
            }
        }
        else {
            s->afio_mosi->level = (s->transmitStatus.tx_data >> (s->transmitStatus.bits_remain - 1)) & 1;
            if (s->afio_mosi->bh) {
                aio_bh_call(s->afio_mosi->bh);
            }
        }
        s->afio_sck->level = ! s->afio_sck->level;
        if (s->afio_sck->bh) {
            aio_bh_call(s->afio_sck->bh);
        }
    }

    // if (sr->bsy) {
    //     timer_mod(&s->clockTimer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 1);
    // }
}

static void stm32f4xx_spi_init(Object *obj)
{
    VEE_STM32F4XXSPIState *s = VEE_STM32F4XX_SPI(obj);
    // DeviceState *dev = DEVICE(obj);

    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_spi_ops, s,
                          TYPE_VEE_STM32F4XX_SPI, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    stm32f4xx_spi_reset((DeviceState *) s);

    // s->clock = clock_new((Object *) s, "clk");
    // clock_set_callback(s->clock, clock_handler, s, ClockUpdate | ClockPreUpdate);
    // clock_update_hz(s->clock, 1e6);
    // clock_update_hz(s->clock, 1e5);

    // qemu_log("qemu: %s() %d: clock: %p \r\n", __FUNCTION__, __LINE__, s->clock);

    timer_init_us(&s->clockTimer, QEMU_CLOCK_VIRTUAL, clock_handler, s);

    s->afio_cs = &g_stm32f4xxAfioSocState.spi[s->portIndex].cs;
    s->afio_sck = &g_stm32f4xxAfioSocState.spi[s->portIndex].sck;
    s->afio_miso = &g_stm32f4xxAfioSocState.spi[s->portIndex].miso;
    s->afio_mosi = &g_stm32f4xxAfioSocState.spi[s->portIndex].mosi;

    s->afio_cs->level = 1;
    s->afio_sck->level = 0;
    s->afio_miso->level = 1;
    s->afio_mosi->level = 1;
}

static void dev_realize(DeviceState *dev, Error **errp)
{
    VEE_STM32F4XXSPIState *s = VEE_STM32F4XX_SPI(dev);

    qemu_log("qemu: %s %s(): portIndex=%d \r\n", __FILE__, __FUNCTION__, s->portIndex);

    // s->clock = clock_new((Object *) dev, "clock");
    // clock_set_callback(s->clock, clock_handler, s, ClockUpdate);
    // clock_set_hz(s->clock, 1e6);

    // qemu_log("qemu: %s %s(): clock is enabled: %d \r\n", __FILE__, __FUNCTION__, clock_is_enabled(s->clock));
}

static const VMStateDescription vmstate_stm32f4xx_spi = {
    .name = TYPE_VEE_STM32F4XX_SPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(regs[StmSpiCR1], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiCR2], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiSR], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiDR], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiCRCPR], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiRxCRCR], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiTxCRCR], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiI2SCFGR], VEE_STM32F4XXSPIState),
        VMSTATE_UINT32(regs[StmSpiI2SPR], VEE_STM32F4XXSPIState),
        VMSTATE_END_OF_LIST()
    }
};

static Property dev_properties[] = {
    DEFINE_PROP_UINT32("port-index", VEE_STM32F4XXSPIState, portIndex, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f4xx_spi_class_init(ObjectClass *klass, void *data)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, stm32f4xx_spi_reset);
    dc->vmsd = &vmstate_stm32f4xx_spi;
    dc->realize = dev_realize;

    device_class_set_props(dc, dev_properties);

    // object_class_property_add_link(klass, TYPE_CLOCK, TYPE_VEE_STM32F4XX_SPI,
    //     offsetof(VEE_STM32F4XXSPIState, clock), qdev_prop_allow_set_link_before_realize,
    //     OBJ_PROP_LINK_STRONG
    // );
}

static const TypeInfo stm32f4xx_spi_info = {
    .name          = TYPE_VEE_STM32F4XX_SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(VEE_STM32F4XXSPIState),
    .instance_init = stm32f4xx_spi_init,
    .class_init    = stm32f4xx_spi_class_init,
};

static void stm32f4xx_spi_register_types(void)
{
    type_register_static(&stm32f4xx_spi_info);
}

type_init(stm32f4xx_spi_register_types)
