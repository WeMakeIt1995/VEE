/*
 * VEE QAPI
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
#include "qapi/compat-policy.h"
#include "qapi/visitor.h"
#include "qapi/qmp/qdict.h"
#include "qapi/dealloc-visitor.h"
#include "qapi/error.h"
#include "qemu/timer.h"
#include "qemu/log.h"
#include "qapi-emit-events.h"
#include "monitor/monitor-internal.h"
#include "hw/misc/vee_ssd1306.h"
#include "sysemu/runstate.h"

#include "qapi/qapi-commands-run-state.h"
#include "qapi/qapi-events-machine.h"

#include "vee-qapi-visit.h"
#include "vee-qapi-commands.h"
#include "vee-qapi-events.h"
#include "vee-qapi-emit-events.h"

static QEMUTimer g_timer;

static void timer_cb(void *opaque)
{
    qemu_log("%s() \r\n", __FUNCTION__);

    // qapi_event_send_vee_event();

    // timer_mod(&g_timer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 1e6);
}

void qmp_vee_command(Error **errp)
{
    timer_init_us(&g_timer, QEMU_CLOCK_VIRTUAL, timer_cb, NULL);
    // timer_mod(&g_timer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 1e6);

    // Object *obj = object_resolve_path_component(NULL, "");
    // if (! obj) {
    //     return;
    // }
    // if (! object_dynamic_cast(obj, TYPE_VEE_PIN_STATE)) {
    //     return;
    // }
}

void vee_qapi_event_emit(vee_QAPIEvent event, QDict *qdict)
{
    Monitor *mon;
    MonitorQMP *qmp_mon;

    QTAILQ_FOREACH(mon, &mon_list, entry) {
        if (!monitor_is_qmp(mon)) {
            continue;
        }

        qmp_mon = container_of(mon, MonitorQMP, common);
        qmp_send_response(qmp_mon, qdict);
    }
}

QmpVeeSSD1306Pixel *qmp_vee_ssd1306_get_pixel(const char *path, Error **errp)
{
    QmpVeeSSD1306Pixel *ret = (QmpVeeSSD1306Pixel *) g_malloc0(sizeof(QmpVeeSSD1306Pixel));

    bool ambiguous;
    Object *obj = object_resolve_path_type(path, TYPE_VEE_SSD1306_STATE, &ambiguous);
    if (! obj) {
        qemu_log("%s %s(): object_resolve_path_type error: %s \r\n", __FILE__, __FUNCTION__, path);
        return ret;
    }

    VeeSSD1306State *dev = (VeeSSD1306State *) object_dynamic_cast(obj, TYPE_VEE_SSD1306_STATE);

    // dev->regs.display_on = true;
    // dev->regs.display_gddram = true;

    VeeSSD1306PixelData_t *pixelData = vee_ssd1306_get_pixel(dev);

    ret->width = pixelData->width;
    ret->height = pixelData->height;

    {
        intList **tail = &ret->pixel;
        int idx = 0;
        for (int i = 0; i < pixelData->width; i ++) {
            for (int j = 0; j < pixelData->height; j ++) {
                QAPI_LIST_APPEND(tail, pixelData->pixel[idx ++]);
            }
        }
    }

    vee_ssd1306_free_pixel(pixelData);

    return ret;
}

static QEMUTimer        g_vm_feed_timer;
static QEMUBH          *g_vm_feed_bh;
static int              g_vm_feed_count = 0;

static void vm_feed_bh_handler(void *opaque)
{
    qemu_log("%s() %d \r\n", __FUNCTION__, __LINE__);

    vm_stop(RUN_STATE_PAUSED);
}

static void vm_feed_timer_handler(void *opaque)
{
    // qemu_log("%s() %d, g_vm_feed_count=%d \r\n", __FUNCTION__, __LINE__, g_vm_feed_count);

    g_vm_feed_count --;
    if (! g_vm_feed_count) {
        // qemu_log("%s() %d \r\n", __FUNCTION__, __LINE__);

        if (g_vm_feed_bh) {
            qemu_bh_delete(g_vm_feed_bh);
        }
        g_vm_feed_bh = qemu_bh_new(vm_feed_bh_handler, NULL);
        qemu_bh_schedule(g_vm_feed_bh);
    }
    else {
        // qemu_log("%s() %d \r\n", __FUNCTION__, __LINE__);

        qapi_event_send_vee_vm_feed_tick_event(qemu_clock_get_us(QEMU_CLOCK_VIRTUAL));

        timer_mod(&g_vm_feed_timer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 5e3);
    }
}

void qmp_vee_vm_feed(int64_t count, Error **errp)
{
    qemu_log("%s() %d: count=%d \r\n", __FUNCTION__, __LINE__, (int) count);

    if (! g_vm_feed_count) {
        // qemu_log("%s() %d \r\n", __FUNCTION__, __LINE__);

        timer_init_us(&g_vm_feed_timer, QEMU_CLOCK_VIRTUAL, vm_feed_timer_handler, NULL);
        timer_mod(&g_vm_feed_timer, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 5e3);

        vm_resume(RUN_STATE_RUNNING);
    }
    g_vm_feed_count += count;
}

QmpVeeVmTimeUs *qmp_vee_get_vm_time_us(Error **errp)
{
    QmpVeeVmTimeUs *ret = (QmpVeeVmTimeUs *) g_malloc0(sizeof(QmpVeeVmTimeUs));

    ret->time = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL);

    return ret;
}
