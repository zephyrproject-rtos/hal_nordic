/*
 * Copyright (c) 2022 - 2024, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx_example.h>
#include <nrfx_twim.h>
#include <nrfx_twis.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_twim_twis_txtx_example TX-TX TWIM with TWIS example
 * @{
 * @ingroup nrfx_twim_twis_examples
 *
 * @brief Example showing the functionality of nrfx_twim and nrfx_twis drivers, when TWI transfer is
 *        performed in TX-TX mode.
 *
 * @details Application initializes nrfx_twim and nrfx_twis drivers. Main purpose of this example is
 *          to emulate a typical situation, when the user has to write data to the particular register of the
 *          slave (usual controller->sensor communication using two wire interface). The example
 *          works as a simple simulation in which the user can communicate with the drone by
 *          defining X, Y, Z forces and sending some additional data. @ref m_drone_reg is used as
 *          internal register map of the drone. In the first write operation (TX1) user should define
 *          register number to which data from the second write operation (TX2) will be saved.
 *          @ref twim_handler() is executed with relevant log messages. @ref twis_handler() is used
 *          to prepare data for sending and update particular registers of @ref m_drone_reg.
 */

/** @brief Symbol specifying pin number of master SCL. */
#define MASTER_SCL_PIN LOOPBACK_PIN_1A

/** @brief Symbol specifying pin number of master SDA. */
#define MASTER_SDA_PIN LOOPBACK_PIN_2A

/** @brief Symbol specifying pin number of slave SCL. */
#define SLAVE_SCL_PIN LOOPBACK_PIN_1B

/** @brief Symbol specifying pin number of slave SDA. */
#define SLAVE_SDA_PIN LOOPBACK_PIN_2B

#if defined(NRF52_SERIES) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol specifying TWIM instance to be used. */
#define TWIM_INST_IDX 0

/** @brief Symbol specifying TWIS instance to be used. */
#define TWIS_INST_IDX 1
#else
#define TWIM_INST_IDX 1
#define TWIS_INST_IDX 2
#endif

/** @brief Symbol specifying drone (slave) address on TWI bus. */
#define DRONE_IDX 0x01U

/** @brief Symbol specifying drone mass [kg], it is used to calculate accelerations. */
#define DRONE_MASS 10U

/** @brief Drone registers names. */
typedef enum
{
    DRONE_REG_ID,         ///< Drone identifier.
    DRONE_REG_MASS,       ///< Drone mass.
    DRONE_REG_FORCE_X,    ///< Force that acts on the drone along X axis.
    DRONE_REG_FORCE_Y,    ///< Force that acts on the drone along Y axis.
    DRONE_REG_FORCE_Z,    ///< Force that acts on the drone along Z axis.
    DRONE_REG_ACC_X,      ///< Acceleration of the drone along X axis.
    DRONE_REG_ACC_Y,      ///< Acceleration of the drone along Y axis.
    DRONE_REG_ACC_Z,      ///< Acceleration of the drone along Z axis.
    DRONE_REG_ADDT_DATA0, ///< Additional data 0.
    DRONE_REG_ADDT_DATA1, ///< Additional data 1.
    DRONE_REG_ADDT_DATA2, ///< Additional data 2.
    DRONE_REG_ADDT_DATA3, ///< Additional data 3.
} drone_reg_name_t;

/** @brief Structure for the drone register map. */
typedef struct
{
    uint8_t drone_idx;          ///< Drone identifier.
    uint8_t mass;               ///< Drone mass.
    uint8_t force[3];           ///< Forces that acts on the drone along X, Y, Z axes.
    uint8_t acceleration[3];    ///< Accelerations of the drone along X, Y, Z axes.
    uint8_t additional_data[4]; ///< Additional data.
} drone_reg_map_t;

/**
 * @brief Union containing a drone register map structure, used to refer to individual elements
 *        of the structure.
 */
typedef union
{
    drone_reg_map_t register_map;           ///< Drone register map.
    uint8_t bytes[sizeof(drone_reg_map_t)]; ///< Array for referring to individual structure elements.
} drone_regs_t;

/**
 * @brief Macro for creating register map structure with the default configuration.
 *
 * This configuration sets up drone registers with the following options:
 * - forces set to 0
 * - accelerations set to 0
 * - additional data set to 0
 *
 * @param[in] _id   Drone identifier.
 * @param[in] _mass Drone mass.
 */
#define DRONE_DEFAULT_CONFIG(_id, _mass) \
{                                        \
    .drone_idx       = (_id),            \
    .mass            = (_mass),          \
    .force           = {0},              \
    .acceleration    = {0},              \
    .additional_data = {0},              \
}

/** @brief Union containing drone register map structure. */
static drone_regs_t m_drone_reg = {.register_map = DRONE_DEFAULT_CONFIG(DRONE_IDX, DRONE_MASS)};

/** @brief Structure containing TWIS driver instance. */
static nrfx_twis_t m_twis_inst = NRFX_TWIS_INSTANCE(TWIS_INST_IDX);

/** @brief Structure containing TWIM driver instance. */
static nrfx_twim_t m_twim_inst = NRFX_TWIM_INSTANCE(TWIM_INST_IDX);

/**
 * @brief Function for showing values of all drone registers.
 *
 * @param[in] p_drone_regs Pointer to the union with drone register map structure.
 */
static void drone_reg_print(drone_regs_t * p_drone_regs)
{
    NRFX_LOG_INFO("................................");
    NRFX_LOG_INFO("DRONE REGISTERS:    DEC\t|  HEX");
    NRFX_LOG_INFO("drone_idx:          %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ID],
                                                      p_drone_regs->bytes[DRONE_REG_ID]);
    NRFX_LOG_INFO("drone_mass:         %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_MASS],
                                                      p_drone_regs->bytes[DRONE_REG_MASS]);
    NRFX_LOG_INFO("x_force:            %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_FORCE_X],
                                                      p_drone_regs->bytes[DRONE_REG_FORCE_X]);
    NRFX_LOG_INFO("y_force:            %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_FORCE_Y],
                                                      p_drone_regs->bytes[DRONE_REG_FORCE_Y]);
    NRFX_LOG_INFO("z_force:            %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_FORCE_Z],
                                                      p_drone_regs->bytes[DRONE_REG_FORCE_Z]);
    NRFX_LOG_INFO("x_acceleration:     %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ACC_X],
                                                      p_drone_regs->bytes[DRONE_REG_ACC_X]);
    NRFX_LOG_INFO("y_acceleration:     %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ACC_Y],
                                                      p_drone_regs->bytes[DRONE_REG_ACC_Y]);
    NRFX_LOG_INFO("z_acceleration:     %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ACC_Z],
                                                      p_drone_regs->bytes[DRONE_REG_ACC_Z]);
    NRFX_LOG_INFO("additional_data[0]: %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ADDT_DATA0],
                                                      p_drone_regs->bytes[DRONE_REG_ADDT_DATA0]);
    NRFX_LOG_INFO("additional_data[1]: %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ADDT_DATA1],
                                                      p_drone_regs->bytes[DRONE_REG_ADDT_DATA1]);
    NRFX_LOG_INFO("additional_data[2]: %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ADDT_DATA2],
                                                      p_drone_regs->bytes[DRONE_REG_ADDT_DATA2]);
    NRFX_LOG_INFO("additional_data[3]: %d \t|  0x%X", p_drone_regs->bytes[DRONE_REG_ADDT_DATA3],
                                                      p_drone_regs->bytes[DRONE_REG_ADDT_DATA3]);
    NRFX_EXAMPLE_LOG_PROCESS();
}

/**
 * @brief Function for handling TWIM driver events.
 *
 * @param[in] p_event   Event information structure.
 * @param[in] p_context General purpose parameter set during initialization of the TWIM.
 *                      This parameter can be used to pass additional information to the
 *                      handler function.
 */
static void twim_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
    if (p_event->type == NRFX_TWIM_EVT_DONE)
    {
        char * p_msg = p_context;
        NRFX_LOG_INFO("%s", p_msg);
    }
    else
    {
        NRFX_LOG_INFO("--> Master event: %d.", p_event->type);
    }
}

/**
 * @brief Function for handling TWIS driver events.
 *
 * @param[in] p_event Event information structure.
 */
static void twis_handler(nrfx_twis_evt_t const * p_event)
{
    /* Flag to show if the next transfer will contain register number (TX1) or data (TX2). */
    static bool first_tx_flag = true;

    /* Variable to store register number sent in the most recent TX1. */
    static uint8_t reg_buff;

    if (p_event->type == NRFX_TWIS_EVT_WRITE_REQ)
    {
        if (first_tx_flag)
        {
            nrfx_err_t status = nrfx_twis_rx_prepare(&m_twis_inst, &reg_buff, sizeof(reg_buff));
            NRFX_ASSERT(status == NRFX_SUCCESS);
            first_tx_flag = !first_tx_flag;
        }
        else
        {
            nrfx_err_t status = nrfx_twis_rx_prepare(&m_twis_inst,
                                                     &m_drone_reg.bytes[reg_buff],
                                                     sizeof(m_drone_reg) - reg_buff);
            NRFX_ASSERT(status == NRFX_SUCCESS);
            first_tx_flag = !first_tx_flag;
        }
    }
    else if (p_event->type == NRFX_TWIS_EVT_WRITE_DONE)
    {
        m_drone_reg.bytes[DRONE_REG_ACC_X] = NRFX_ROUNDED_DIV(m_drone_reg.bytes[DRONE_REG_FORCE_X],
                                                              m_drone_reg.bytes[DRONE_REG_MASS]);
        m_drone_reg.bytes[DRONE_REG_ACC_Y] = NRFX_ROUNDED_DIV(m_drone_reg.bytes[DRONE_REG_FORCE_Y],
                                                              m_drone_reg.bytes[DRONE_REG_MASS]);
        m_drone_reg.bytes[DRONE_REG_ACC_Z] = NRFX_ROUNDED_DIV(m_drone_reg.bytes[DRONE_REG_FORCE_Z],
                                                              m_drone_reg.bytes[DRONE_REG_MASS]);
    }
}

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    nrfx_err_t status;
    (void)status;

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TWIM_INST_GET(TWIM_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_TWIM_INST_HANDLER_GET(TWIM_INST_IDX), 0, 0);

    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TWIS_INST_GET(TWIS_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_TWIS_INST_HANDLER_GET(TWIS_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_twim_twis txtx example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    /* Variable to store register number. */
    drone_reg_name_t register_num;

    /* Buffer that will be sent from master to slave (controller->drone). */
    uint32_t drone_ctrl_buffer;

    void * p_context = "--> Master event: done - transfer completed";
    nrfx_twim_xfer_desc_t twim_xfer_desc = NRFX_TWIM_XFER_DESC_TXTX(DRONE_IDX,
                                                                    (uint8_t *)&register_num,
                                                                    sizeof(register_num),
                                                                    (uint8_t *)&drone_ctrl_buffer,
                                                                    sizeof(drone_ctrl_buffer));
    nrfx_twim_config_t twim_config = NRFX_TWIM_DEFAULT_CONFIG(MASTER_SCL_PIN, MASTER_SDA_PIN);
    status = nrfx_twim_init(&m_twim_inst, &twim_config, twim_handler, p_context);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_twis_config_t twis_config = NRFX_TWIS_DEFAULT_CONFIG(SLAVE_SCL_PIN,
                                                              SLAVE_SDA_PIN,
                                                              DRONE_IDX);
    status = nrfx_twis_init(&m_twis_inst, &twis_config, twis_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    drone_reg_print(&m_drone_reg);

    NRFX_LOG_INFO("................................");
    NRFX_LOG_INFO("MASTER EVENTS:");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_twim_enable(&m_twim_inst);
    nrfx_twis_enable(&m_twis_inst);

    register_num = DRONE_REG_FORCE_X;
    drone_ctrl_buffer = 50;
    status = nrfx_twim_xfer(&m_twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (nrfx_twim_is_busy(&m_twim_inst))
    {}

    register_num = DRONE_REG_FORCE_Y;
    drone_ctrl_buffer = 100;
    status = nrfx_twim_xfer(&m_twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (nrfx_twim_is_busy(&m_twim_inst))
    {}

    register_num = DRONE_REG_FORCE_Z;
    drone_ctrl_buffer = 150;
    status = nrfx_twim_xfer(&m_twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (nrfx_twim_is_busy(&m_twim_inst))
    {}

    register_num = DRONE_REG_ADDT_DATA0;
    drone_ctrl_buffer = 0xDEADBEEF;
    status = nrfx_twim_xfer(&m_twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (nrfx_twim_is_busy(&m_twim_inst))
    {}

    NRFX_LOG_INFO("TXTX transfers finished.");
    drone_reg_print(&m_drone_reg);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
