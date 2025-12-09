
#ifndef HW_STM32F4XX_GPIO_H
#define HW_STM32F4XX_GPIO_H

#include "hw/sysbus.h"
#include "hw/misc/vee_pin.h"
#include "hw/misc/vee_stm32f4xx_hw_info.h"
#include "qemu/main-loop.h"
#include "qemu/queue.h"
#include "qom/object.h"

typedef enum StmGpioReg {
    StmGpioMODER,
    StmGpioOTYPER,
    StmGpioOSPEEDR,
    StmGpioPUPDR,
    StmGpioIDR,
    StmGpioODR,
    StmGpioBSRR,
    StmGpioLCKR,
    StmGpioAFRL,
    StmGpioAFRH,
    StmGpioRegCount,
} StmGpioReg;

#define TYPE_STM32F4XX_GPIO "stm32f4xx-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F4XXGPIOState, STM32F4XX_GPIO)

typedef struct STM32F4XXGPIOState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t portIndex;

    uint32_t regs[StmGpioRegCount];

    uint32_t lckrSequence[3];

    bool lckrActive;

    VeePinState pinsState[STM_HW_GPIO_PINN];
} STM32F4XXGPIOState;

typedef struct STM32F4XXAFIOState {
    int     level;

    QEMUBH             *bh;

    STM32F4XXGPIOState *connectedGpioState;

    int     connectedGpioPin;
} STM32F4XXAFIOState;

// TODO: compelete this
// Member type STM32F4XXAFIOState ONLY !!!
typedef struct STM32F4XXAFIOSocState {
    struct {

    } tim[STM32F4XX_TIM_NUM];

    struct {
        STM32F4XXAFIOState      scl, sda;
    } i2c[STM32F4XX_I2C_NUM];

    struct {
        STM32F4XXAFIOState      cs, sck, miso, mosi;
    } spi[STM32F4XX_SPI_NUM];

    struct {

    } i2s[STM32F4XX_I2S_NUM];

    struct {

    } i2sext[STM32F4XX_I2S_EXT_NUM];

    struct {

    } usart[STM32F4XX_USART_NUM];

    struct {

    } can[STM32F4XX_CAN_NUM];

    struct {

    } otg_fs[STM32F4XX_OTG_FS_NUM];

    struct {

    } otg_hs[STM32F4XX_OTG_HS_NUM];

    struct {

    } eth[STM32F4XX_ETH_NUM];

    struct {

    } fmc[STM32F4XX_FMC_NUM];

    struct {

    } sdio[STM32F4XX_SDIO_NUM];

    struct {

    } dcmi[STM32F4XX_DCMI_NUM];

    struct {

    } eventout[STM32F4XX_EVENTOUT_NUM];
} STM32F4XXAFIOSocState;

extern STM32F4XXAFIOSocState g_stm32f4xxAfioSocState;

void stm32f4xx_gpio_afio_state_change_handler(void *opaque);

#endif
