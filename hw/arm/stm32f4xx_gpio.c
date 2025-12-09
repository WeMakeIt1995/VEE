/*
 * STM32F4XX GPIO
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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
#include "hw/misc/stm32f4xx_gpio.h"
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

enum {
    ModeIN,
    ModePP,
    ModeAF,
    ModeAN,
};

enum {
    OutPP,
    OutOD,
};

enum {
    PullNO,
    PullUP,
    PullDOWN,
};

enum {
    AF0_System                  = 1 << 0,
    AF1_tim1_to_2               = 1 << 1,
    AF2_tim3_to_5               = 1 << 2,
    AF3_tim8_to_11              = 1 << 3,
    AF4_i2c1_to_3               = 1 << 4,
    AF5_spi1_2_i2s2_i2s2ext     = 1 << 5,
    AF6_spi3_i2sext_i2s3        = 1 << 6,
    AF7_usart1_to_3_i2s3ext     = 1 << 7,
    AF8_usart4_to_6             = 1 << 8,
    AF9_can1_2_tim12_to_14      = 1 << 9,
    AF10_otgfs_otghs            = 1 << 10,
    AF11_eth                    = 1 << 11,
    AF12_fsmc_sdio_otgfs        = 1 << 12,
    AF13_dcmi                   = 1 << 13,
    AF14_reserved               = 1 << 14,
    AF15_Eventout               = 1 << 15,
};

#define AF_COUNT        16

typedef struct {
    STM32F4XXGPIOState     *gpioState;

    int     pin;
} PinToGpioInfo;

// reference: mcu data sheet
static STM32F4XXAFIOState *g_stm32f4xxAfioSocMap[][STM_HW_GPIO_PINN][AF_COUNT] = {
    // PA
    {
        { 0 },
        { 0 },
        { 0 },
        { 0 },

        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].cs, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].sck, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].miso, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].mosi, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },

        { 0 },
        { 0 },
        { 0 },
        { 0 },

        { 0 },
        { 0 },
        { 0 },
        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].cs, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
    },
    // PB
    {
        { 0 },
        { 0 },
        { 0 },
        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].sck, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },

        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].miso, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            0, &g_stm32f4xxAfioSocState.spi[0].mosi, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            &g_stm32f4xxAfioSocState.i2c[0].sda, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            &g_stm32f4xxAfioSocState.i2c[0].scl, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },

        {
            0, 0, 0, 0,
            &g_stm32f4xxAfioSocState.i2c[0].sda, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        {
            0, 0, 0, 0,
            &g_stm32f4xxAfioSocState.i2c[0].scl, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        },
        { 0 },
        { 0 },

        { 0 },
        { 0 },
        { 0 },
        { 0 },
    },
    // PC
    {
        { 0 },
        { 0 },
        { 0 },
        { 0 },

        { 0 },
        { 0 },
        { 0 },
        { 0 },

        { 0 },
        { 0 },
        { 0 },
        { 0 },

        { 0 },
        { 0 },
        { 0 },
        { 0 },
    }
};

STM32F4XXAFIOSocState       g_stm32f4xxAfioSocState;

static void pin_state_arbitration(STM32F4XXGPIOState *s, int pin)
{
    VeePinState     state = { 0 };

    assert(pin < STM_HW_GPIO_PINN);

    int mode = extract32(s->regs[StmGpioMODER], pin * 2, 2);
    int otype = extract32(s->regs[StmGpioOTYPER], pin * 1, 1);
    int pupd = extract32(s->regs[StmGpioPUPDR], pin * 2, 2);
    int od = extract32(s->regs[StmGpioODR], pin * 1, 1);
    int af = 0;
    if (pin < 8) {
        af = extract32(s->regs[StmGpioAFRL], pin * 4, 4);
    }
    else {
        af = extract32(s->regs[StmGpioAFRH], (pin - 8) * 4, 4);
    }

    switch (mode) {
    case ModeIN: {
        state.status.direction = VeePinIn;

        switch (pupd) {
        case PullNO: state.status.out_voltage_mv = 3300; break;
        case PullUP: state.status.out_voltage_mv = 3300; break;
        case PullDOWN: break;
        }
    } break;

    case ModePP: {
        state.status.direction = VeePinOut;

        if (otype == OutPP) {
            if (od) {
                state.status.out_voltage_mv = 3300;
                state.status.out_current_ma = 20;
            }
        }
        else if (otype == OutOD) {
            if (od) {
                switch (pupd) {
                case PullNO: state.status.out_voltage_mv = 3300; break;
                case PullUP: state.status.out_voltage_mv = 3300; break;
                case PullDOWN: break;
                }
            }
        }
    } break;

    case ModeAF: {
        state.status.direction = VeePinOut;

        STM32F4XXAFIOState     *afioState = g_stm32f4xxAfioSocMap[s->portIndex][pin][af];
        if (! afioState) {
            break;
        }
        if (afioState->connectedGpioState != s || afioState->connectedGpioPin != pin) {
            break;
        }

        if (otype == OutPP) {
            if (afioState->level) {
                state.status.out_voltage_mv = 3300;
                state.status.out_current_ma = 20;
            }
        }
        else if (otype == OutOD) {
            if (afioState->level) {
                switch (pupd) {
                case PullNO: state.status.out_voltage_mv = 3300; break;
                case PullUP: state.status.out_voltage_mv = 3300; break;
                case PullDOWN: break;
                }
            }
        }
    } break;

    case ModeAN: {
        state.status.direction = VeePinHighImpedance;
    } break;

    default: break;
    }

    // 如果有外部电路，需要由外部通知更改IDR寄存器
    if (state.status.out_voltage_mv && ! s->pinsState[pin].has_extern_circuit) {
        s->regs[StmGpioIDR] &= ~ (1 << pin);
        s->regs[StmGpioIDR] |= (state.status.out_voltage_mv ? 1 : 0) << pin;
    }

    if (memcmp(&state.status, &s->pinsState[pin].status, sizeof(state.status)) == 0) {
        return;
    }

    qemu_log("%s %s %d: port-index: %d, pin: %d, direction: %d, in_vol: %d, out_vol: %d \r\n",
        __FILE__, __FUNCTION__, __LINE__,
        s->portIndex, pin, (int) state.status.direction, (int) state.status.in_voltage_mv, (int) state.status.out_voltage_mv
    );

    s->pinsState[pin].status = state.status;
    if (s->pinsState[pin].outStateChangeBH) {
        aio_bh_call(s->pinsState[pin].outStateChangeBH);
    }
}

void stm32f4xx_gpio_afio_state_change_handler(void *opaque)
{
    STM32F4XXAFIOState *state = (STM32F4XXAFIOState *) opaque;
    pin_state_arbitration(state->connectedGpioState, state->connectedGpioPin);
}

static void stm32f4xx_gpio_reset(DeviceState *dev)
{
    // STM32F4XXGPIOState *s = STM32F4XX_GPIO(dev);

    // s->spi_cr1 = 0x00000000;
    // s->spi_cr2 = 0x00000000;
    // s->spi_sr = 0x0000000A;
    // s->spi_dr = 0x0000000C;
    // s->spi_crcpr = 0x00000007;
    // s->spi_rxcrcr = 0x00000000;
    // s->spi_txcrcr = 0x00000000;
    // s->spi_i2scfgr = 0x00000000;
    // s->spi_i2spr = 0x00000002;
}

// static void stm32f4xx_gpio_transfer(STM32F4XXGPIOState *s)
// {
    // DB_PRINT("Data to send: 0x%x\n", s->spi_dr);

    // s->spi_dr = ssi_transfer(s->ssi, s->spi_dr);
    // s->spi_sr |= STM_SPI_SR_RXNE;

    // DB_PRINT("Data received: 0x%x\n", s->spi_dr);
// }

static uint64_t stm32f4xx_gpio_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXGPIOState *s = opaque;

    addr >>= 2;

    return s->regs[addr];
}

static void change_pin_state(STM32F4XXGPIOState *s, int addr, uint32_t val)
{
    if (s->regs[addr] == val) {
        return;
    }
    s->regs[addr] = val;

    // check MODER necessary?
    if (addr == StmGpioAFRL) {
        for (int i = 0; i < STM_HW_GPIO_PINN / 2; i ++) {
            int af = extract32(s->regs[StmGpioAFRL], i * 4, 4);
            STM32F4XXAFIOState     *afioState = g_stm32f4xxAfioSocMap[s->portIndex][i][af];
            if (! afioState) {
                continue;
            }

            afioState->connectedGpioState = s;
            afioState->connectedGpioPin = i;

            if (afioState->bh) {
                qemu_bh_delete(afioState->bh);
            }
            afioState->bh = qemu_bh_new(stm32f4xx_gpio_afio_state_change_handler, afioState);
        }
    }
    if (addr == StmGpioAFRH) {
        for (int i = STM_HW_GPIO_PINN / 2; i < STM_HW_GPIO_PINN; i ++) {
            int af = extract32(s->regs[StmGpioAFRH], (i - STM_HW_GPIO_PINN / 2) * 4, 4);
            STM32F4XXAFIOState     *afioState = g_stm32f4xxAfioSocMap[s->portIndex][i][af];
            if (! afioState) {
                continue;
            }

            afioState->connectedGpioState = s;
            afioState->connectedGpioPin = i;

            if (afioState->bh) {
                qemu_bh_delete(afioState->bh);
            }
            afioState->bh = qemu_bh_new(stm32f4xx_gpio_afio_state_change_handler, afioState);
        }
    }

    for (int i = 0; i < STM_HW_GPIO_PINN; i ++) {
        pin_state_arbitration(s, i);
    }
}

static void stm32f4xx_gpio_write(void *opaque, hwaddr addr,
                                uint64_t val64, unsigned int size)
{
    STM32F4XXGPIOState *s = opaque;

    qemu_log("qemu: %s(), port-index=%d, addr=%lld(0x%llx), val64=%lld(0x%llx), size=%u \r\n",
        __FUNCTION__, s->portIndex, (long long) addr, (long long) addr, (long long) val64, (long long) val64, size
    );

    addr >>= 2;

    switch (addr) {
    case StmGpioMODER:
    case StmGpioOTYPER:
    case StmGpioOSPEEDR:
    case StmGpioPUPDR: {
        if (s->lckrActive) {
            break;
        }
        change_pin_state(s, addr, val64);
    } break;

    case StmGpioIDR: {

    } break;

    case StmGpioODR: {
        if (s->lckrActive) {
            break;
        }
        change_pin_state(s, addr, val64);
    } break;

    case StmGpioBSRR: {
        if (s->lckrActive) {
            break;
        }
        uint32_t temp = s->regs[StmGpioODR];
        temp |= val64 & 0xffff;
        temp &= ~((val64 >> 16) & 0xffff);

        change_pin_state(s, StmGpioODR, temp);
    } break;

    case StmGpioLCKR: {
        if (s->lckrActive) {
            break;
        }
        s->lckrSequence[2] = s->lckrSequence[1];
        s->lckrSequence[1] = s->lckrSequence[0];
        s->lckrSequence[0] = (typeof(s->regs[0])) val64;

        if (((s->lckrSequence[2] & 0x10000) == 0x10000) &&
            ((s->lckrSequence[1] & 0x10000) == 0x00000) &&
            ((s->lckrSequence[0] & 0x10000) == 0x10000) &&
            ((s->lckrSequence[2] & 0x0ffff) == (s->lckrSequence[1] & 0x0ffff)) &&
            ((s->lckrSequence[2] & 0x0ffff) == (s->lckrSequence[0] & 0x0ffff))
        ) {
            s->lckrActive = true;
        }
    } break;

    case StmGpioAFRL:
    case StmGpioAFRH: {
        if (s->lckrActive) {
            break;
        }
        change_pin_state(s, addr, val64);
    } break;

    default: break;
    }
}

static const MemoryRegionOps stm32f4xx_gpio_ops = {
    .read = stm32f4xx_gpio_read,
    .write = stm32f4xx_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_stm32f4xx_gpio = {
    .name = TYPE_STM32F4XX_GPIO,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(regs[StmGpioMODER], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioOTYPER], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioOSPEEDR], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioPUPDR], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioIDR], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioODR], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioBSRR], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioLCKR], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioAFRL], STM32F4XXGPIOState),
        VMSTATE_UINT32(regs[StmGpioAFRH], STM32F4XXGPIOState),
        VMSTATE_END_OF_LIST()
    }
};

static void pin_in_state_change_handler(void *opaque)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    PinToGpioInfo *info = (PinToGpioInfo *) opaque;
    VeePinState *s = &info->gpioState->pinsState[info->pin];

    info->gpioState->regs[StmGpioIDR] &= ~ (1 << info->pin);
    info->gpioState->regs[StmGpioIDR] |= (s->status.in_voltage_mv ? 1 : 0) << info->pin;

    qemu_log("change info: port-index=%d, pin=%d, vol=%d \r\n", info->gpioState->portIndex, info->pin, (int) s->status.in_voltage_mv);
}

static void stm32f4xx_gpio_init(Object *obj)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    STM32F4XXGPIOState *s = STM32F4XX_GPIO(obj);
    // DeviceState *dev = DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_gpio_ops, s,
                          TYPE_STM32F4XX_GPIO, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    for (int i = 0; i < STM_HW_GPIO_PINN; i ++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "pin%d", i);
        object_initialize_child(obj, buf, &s->pinsState[i], TYPE_VEE_PIN_STATE);

        PinToGpioInfo *info = (PinToGpioInfo *) g_malloc(sizeof(PinToGpioInfo));
        info->gpioState = s;
        info->pin = i;

        s->pinsState[i].inStateChangeBH = qemu_bh_new(pin_in_state_change_handler, info);
    }

    // sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    // s->ssi = ssi_create_bus(dev, "ssi");
}

static Property dev_properties[] = {
    DEFINE_PROP_UINT32("port-index", STM32F4XXGPIOState, portIndex, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f4xx_gpio_class_init(ObjectClass *klass, void *data)
{
    qemu_log("qemu: %s() \r\n", __FUNCTION__);

    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, stm32f4xx_gpio_reset);
    dc->vmsd = &vmstate_stm32f4xx_gpio;

    device_class_set_props(dc, dev_properties);

    for (int i = 0; i < sizeof(g_stm32f4xxAfioSocState) / sizeof(STM32F4XXAFIOState); i ++) {
        ((STM32F4XXAFIOState *) &g_stm32f4xxAfioSocState)[i].level = 0;
        ((STM32F4XXAFIOState *) &g_stm32f4xxAfioSocState)[i].bh = NULL;
        ((STM32F4XXAFIOState *) &g_stm32f4xxAfioSocState)[i].connectedGpioPin = 0;
        ((STM32F4XXAFIOState *) &g_stm32f4xxAfioSocState)[i].connectedGpioState = NULL;
    }
}

static const TypeInfo stm32f4xx_gpio_info = {
    .name          = TYPE_STM32F4XX_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4XXGPIOState),
    .instance_init = stm32f4xx_gpio_init,
    .class_init    = stm32f4xx_gpio_class_init,
};

static void stm32f4xx_gpio_register_types(void)
{
    type_register_static(&stm32f4xx_gpio_info);
}

type_init(stm32f4xx_gpio_register_types)
