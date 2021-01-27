/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements the thermometer abstraction that uses Zephyr sensor
 *   API for the die termomether.
 *
 */

#include <platform/temperature/nrf_802154_temperature.h>

#include <device.h>
#include <drivers/sensor.h>
#include <kernel.h>

#define TEMPERATURE_UPDATE_THREAD_STACK_SIZE 128
#define TEMPERATURE_UPDATE_THREAD_PRIORITY   5

/** @brief Thread handler for temperature update. */
static void temperature_update_thread(void * p1, void * p2, void * p3);

K_THREAD_DEFINE(temperature_update_tid,
                TEMPERATURE_UPDATE_THREAD_STACK_SIZE,
                temperature_update_thread,
                NULL,
                NULL,
                NULL,
                TEMPERATURE_UPDATE_THREAD_PRIORITY,
                0,
                0);

/** @brief Value of the last temperature measurement. */
static int8_t m_temperature;

void nrf_802154_temperature_init(void)
{
    // Intentionally empty
}

void nrf_802154_temperature_deinit(void)
{
    // Intentionally empty
}

int8_t nrf_802154_temperature_get(void)
{
    return m_temperature;
}

static void temperature_update_thread(void * p1, void * p2, void * p3)
{
    (void)p1;
    (void)p2;
    (void)p3;

    static struct sensor_value   temperature_value;
    static const struct device * temperature_dev;

    temperature_dev = DEVICE_DT_GET(DT_NODELABEL(temp));

    while (true)
    {
        sensor_sample_fetch(temperature_dev);
        sensor_channel_get(temperature_dev, SENSOR_CHAN_DIE_TEMP, &temperature_value);

        if (m_temperature != temperature_value.val1)
        {
            m_temperature = temperature_value.val1;

            nrf_802154_temperature_changed();
        }

        k_sleep(K_SECONDS(1));
    }
}
