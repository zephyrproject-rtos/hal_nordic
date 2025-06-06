/*
 * Copyright (c) 2019 - 2025, Nordic Semiconductor ASA
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

#ifndef NRF_AAR_H__
#define NRF_AAR_H__

#include <nrfx.h>
#ifdef EASYVDMA_PRESENT
#include <helpers/nrf_vdma.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_aar_hal AAR HAL
 * @{
 * @ingroup nrf_aar
 * @brief   Hardware access layer for managing the Accelerated Address Resolver (AAR) peripheral.
 */

#if defined(AAR_EVENTS_ERROR_EVENTS_ERROR_Msk) || defined(AAR_INTENSET_ERROR_Msk) || \
    defined(AAR_ERRORSTATUS_ERRORSTATUS_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the ERROR event and ERRORSTATUS register. */
#define NRF_AAR_HAS_ERROR 1
#else
#define NRF_AAR_HAS_ERROR 0
#endif

#if defined(AAR_OUT_AMOUNT_AMOUNT_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the OUT.AMOUNT register. */
#define NRF_AAR_HAS_OUT_AMOUNT 1
#else
#define NRF_AAR_HAS_OUT_AMOUNT 0
#endif

#if defined(AAR_NIRK_NIRK_Msk) || defined(NRF51) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the NIRK register. */
#define NRF_AAR_HAS_NIRK 1
#else
#define NRF_AAR_HAS_NIRK 0
#endif

#if defined(AAR_IRKPTR_IRKPTR_Msk) || defined(NRF51) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the IRKPTR register. */
#define NRF_AAR_HAS_IRKPTR 1
#else
#define NRF_AAR_HAS_IRKPTR 0
#endif

#if defined(AAR_IN_PTR_PTR_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the IN.PTR register. */
#define NRF_AAR_HAS_IN_PTR 1
#else
#define NRF_AAR_HAS_IN_PTR 0
#endif

#if defined(AAR_OUT_PTR_PTR_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the OUT.PTR register. */
#define NRF_AAR_HAS_OUT_PTR 1
#else
#define NRF_AAR_HAS_OUT_PTR 0
#endif

#if defined(AAR_ADDRPTR_ADDRPTR_Msk) || defined(NRF51) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the ADDRPTR register. */
#define NRF_AAR_HAS_ADDRPTR 1
#else
#define NRF_AAR_HAS_ADDRPTR 0
#endif

#if defined(AAR_SCRATCHPTR_SCRATCHPTR_Msk) || defined(NRF51) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the SCRATCHPTR register. */
#define NRF_AAR_HAS_SCRATCHPTR 1
#else
#define NRF_AAR_HAS_SCRATCHPTR 0
#endif

#if defined(AAR_STATUS_STATUS_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the STATUS register. */
#define NRF_AAR_HAS_STATUS 1
#else
#define NRF_AAR_HAS_STATUS 0
#endif

#if defined(AAR_MAXRESOLVED_MAXRESOLVED_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the MAXRESOLVED register. */
#define NRF_AAR_HAS_MAXRESOLVED 1
#else
#define NRF_AAR_HAS_MAXRESOLVED 0
#endif

#if defined(AAR_ERRORSTATUS_ERRORSTATUS_PrematureOutptrEnd) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the error signalling premature end of the output pointer. */
#define NRF_AAR_HAS_PREMATUREOUTPTREND 1
#else
#define NRF_AAR_HAS_PREMATUREOUTPTREND 0
#endif

/** @brief AAR events. */
typedef enum
{
    NRF_AAR_EVENT_END         = offsetof(NRF_AAR_Type, EVENTS_END),         ///< Address resolution procedure complete.
    NRF_AAR_EVENT_RESOLVED    = offsetof(NRF_AAR_Type, EVENTS_RESOLVED),    ///< Address resolved.
    NRF_AAR_EVENT_NOTRESOLVED = offsetof(NRF_AAR_Type, EVENTS_NOTRESOLVED), ///< Address not resolved.
#if NRF_AAR_HAS_ERROR
    NRF_AAR_EVENT_ERROR       = offsetof(NRF_AAR_Type, EVENTS_ERROR),       ///< Address resolution procedure aborted due to STOP task or error.
#endif
} nrf_aar_event_t;

/** @brief AAR interrupts. */
typedef enum
{
    NRF_AAR_INT_END_MASK         = AAR_INTENSET_END_Msk,         ///< Interrupt on END event.
    NRF_AAR_INT_RESOLVED_MASK    = AAR_INTENSET_RESOLVED_Msk,    ///< Interrupt on RESOLVED event.
    NRF_AAR_INT_NOTRESOLVED_MASK = AAR_INTENSET_NOTRESOLVED_Msk, ///< Interrupt on NOTRESOLVED event.
#if NRF_AAR_HAS_ERROR
    NRF_AAR_INT_ERROR_MASK       = AAR_INTENSET_ERROR_Msk, ///< Interrupt on NOTRESOLVED event.
#endif
} nrf_aar_int_mask_t;

#if NRF_AAR_HAS_ERROR
/** @brief AAR error status when ERROR event is generated. */
typedef enum
{
    NRF_AAR_ERROR_NO_ERROR             = AAR_ERRORSTATUS_ERRORSTATUS_NoError,            ///< No errors have occurred.
    NRF_AAR_ERROR_PREMATURE_INPTR_END  = AAR_ERRORSTATUS_ERRORSTATUS_PrematureInptrEnd,  ///< End of INPTR job list before data structure was read.
#if NRF_AAR_HAS_PREMATUREOUTPTREND
    NRF_AAR_ERROR_PREMATURE_OUTPTR_END = AAR_ERRORSTATUS_ERRORSTATUS_PrematureOutptrEnd, ///< End of OUTPTR job list before data structure was read.
#endif
    NRF_AAR_ERROR_DMA_ERROR            = AAR_ERRORSTATUS_ERRORSTATUS_DmaError,           ///< Bus error during DMA access.
} nrf_aar_error_t;
#endif

/** @brief AAR tasks. */
typedef enum
{
    NRF_AAR_TASK_START = offsetof(NRF_AAR_Type, TASKS_START), ///< Start address resolution procedure.
    NRF_AAR_TASK_STOP  = offsetof(NRF_AAR_Type, TASKS_STOP),  ///< Stop address resolution procedure.
} nrf_aar_task_t;

/**
 * @brief Function for retrieving the state of the AAR event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  Event is set.
 * @retval false Event is not set.
 */
NRF_STATIC_INLINE bool nrf_aar_event_check(NRF_AAR_Type const * p_reg,
                                           nrf_aar_event_t      event);

/**
 * @brief Function for clearing the specified AAR event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
NRF_STATIC_INLINE void nrf_aar_event_clear(NRF_AAR_Type *  p_reg,
                                           nrf_aar_event_t event);

/**
 * @brief Function for getting the address of the specified AAR event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to get the address of.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_aar_event_address_get(NRF_AAR_Type const * p_reg,
                                                     nrf_aar_event_t      event);

/**
 * @brief Function for enabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 *                  Use @ref nrf_aar_int_mask_t values for bit masking.
 */
NRF_STATIC_INLINE void nrf_aar_int_enable(NRF_AAR_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *                  Use @ref nrf_aar_int_mask_t values for bit masking.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_aar_int_enable_check(NRF_AAR_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 *                  Use @ref nrf_aar_int_mask_t values for bit masking.
 */
NRF_STATIC_INLINE void nrf_aar_int_disable(NRF_AAR_Type * p_reg, uint32_t mask);

/**
 * @brief Function for starting an AAR task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_aar_task_trigger(NRF_AAR_Type * p_reg, nrf_aar_task_t task);

/**
 * @brief Function for getting the address of a specific AAR task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Requested AAR task.
 *
 * @return Address of the specified task register.
 */
NRF_STATIC_INLINE uint32_t nrf_aar_task_address_get(NRF_AAR_Type const * p_reg,
                                                    nrf_aar_task_t       task);

/**
 * @brief Function for enabling AAR.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_aar_enable(NRF_AAR_Type * p_reg);

/**
 * @brief Function for disabling AAR.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_aar_disable(NRF_AAR_Type * p_reg);

#if NRF_AAR_HAS_ERROR
/**
 * @brief Function for getting the error status when ERROR event is generated.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval Error status when the ERROR event is generated.
 */
NRF_STATIC_INLINE nrf_aar_error_t nrf_aar_error_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_ERROR

#if NRF_AAR_HAS_IRKPTR
/**
 * @brief Function for setting the pointer to the Identity Resolving Keys (IRK) data structure.
 *
 * The size of the provided data structure must correspond to the number of keys available.
 * Each key occupies 16 bytes.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] irk_ptr Pointer to the IRK data structure. Must point to the Data RAM region.
 *
 * @sa nrf_aar_irk_number_set
 */
NRF_STATIC_INLINE void nrf_aar_irk_pointer_set(NRF_AAR_Type * p_reg, uint8_t const * irk_ptr);

/**
 * @brief Function for getting the pointer to the Identity Resolving Keys
 *        data structure.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the IRK data structure.
 */
NRF_STATIC_INLINE uint8_t const * nrf_aar_irk_pointer_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_IRKPTR

#if NRF_AAR_HAS_NIRK
/**
 * @brief Function for setting the number of keys available in the Identity Resolving Keys
 *        data structure.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] irk_num Number of keys available in the IRK data structure. Maximum is 16.
 *                    Must correspond to the size of the provided IRK data structure.
 *
 * @sa nrf_aar_irk_pointer_set
 */
NRF_STATIC_INLINE void nrf_aar_irk_number_set(NRF_AAR_Type * p_reg, uint8_t irk_num);

/**
 * @brief Function for getting the number of keys available in the Identity Resolving Keys
 *        data structure.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Number of keys in the IRK data structure.
 */
NRF_STATIC_INLINE uint8_t nrf_aar_irk_number_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_NIRK

#if NRF_AAR_HAS_MAXRESOLVED
/**
 * @brief Function for setting maximum number of Identity Resolving Keys to resolve.
 *
 * @param[in] p_reg       Pointer to the structure of registers of the peripheral.
 * @param[in] maxresolved Maximum number of Identity Resolving Keys to resolve.
 *
 * @sa nrf_aar_irk_pointer_set
 */
NRF_STATIC_INLINE void nrf_aar_maxresolved_set(NRF_AAR_Type * p_reg, uint16_t maxresolved);

/**
 * @brief Function for getting maximum number of Identity Resolving Keys to resolve.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Maximum number of Identity Resolving Keys to resolve.
 */
NRF_STATIC_INLINE uint16_t nrf_aar_maxresolved_get(NRF_AAR_Type const * p_reg);
#endif

#if NRF_AAR_HAS_ADDRPTR
/**
 * @brief Function for setting the pointer to the resolvable address.
 *
 * The resolvable address must consist of 6 bytes.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] addr_ptr Pointer to the address to resolve using the available IRK keys.
 *                     Must point to the Data RAM region.
 */
NRF_STATIC_INLINE void nrf_aar_addr_pointer_set(NRF_AAR_Type * p_reg, uint8_t const * addr_ptr);

/**
 * @brief Function for getting the pointer to the resolvable address.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the address to resolve.
 */
NRF_STATIC_INLINE uint8_t const * nrf_aar_addr_pointer_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_ADDRPTR

#if NRF_AAR_HAS_OUT_PTR
/**
 * @brief Function for setting the pointer to a job list containing description to store
 *        resolved addresses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_job Pointer to a job list.
 */
NRF_STATIC_INLINE void nrf_aar_out_ptr_set(NRF_AAR_Type *         p_reg,
                                           nrf_vdma_job_t const * p_job);

/**
 * @brief Function for getting the pointer to a job list containing description to store
 *        resolved addresses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the job list.
 */
NRF_STATIC_INLINE nrf_vdma_job_t * nrf_aar_out_ptr_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_OUT_PTR

#if NRF_AAR_HAS_IN_PTR
/**
 * @brief Function for setting the pointer to a job list containing both
 *        the Hash and Prand parts of the private resolvable address (DEVICEADDR)
 *        field from the Bluetooth packet, and a number of Identity Resolving Keys (IRK).
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_job Pointer to a job list.
 */
NRF_STATIC_INLINE void nrf_aar_in_ptr_set(NRF_AAR_Type *         p_reg,
                                          nrf_vdma_job_t const * p_job);

/**
 * @brief Function for getting the pointer to a job list containing both
 *        the Hash and Prand parts of the private resolvable address (DEVICEADDR)
 *        field from the Bluetooth packet, and a number of Identity Resolving Keys (IRK).
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the job list.
 */
NRF_STATIC_INLINE nrf_vdma_job_t * nrf_aar_in_ptr_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_IN_PTR

#if NRF_AAR_HAS_OUT_AMOUNT
/**
 * @brief Function for getting number of bytes available in the output data,
 *        not including the job list structure.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Number of bytes available in the output data.
 */
NRF_STATIC_INLINE uint32_t nrf_aar_out_amount_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_OUT_AMOUNT

#if NRF_AAR_HAS_SCRATCHPTR
/**
 * @brief Function for setting the pointer to the scratch data area.
 *
 * The scratch data area is used for temporary storage during the address resolution procedure.
 * A space of minimum 3 bytes must be reserved for the scratch data area.
 *
 * @param[in] p_reg       Pointer to the structure of registers of the peripheral.
 * @param[in] scratch_ptr Pointer to the scratch data area. Must point to the Data RAM region.
 */
NRF_STATIC_INLINE void nrf_aar_scratch_pointer_set(NRF_AAR_Type * p_reg, uint8_t * scratch_ptr);

/**
 * @brief Function for getting the pointer to the scratch data area.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the scratch data area.
 */
NRF_STATIC_INLINE uint8_t * nrf_aar_scratch_pointer_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_SCRATCHPTR

#if NRF_AAR_HAS_STATUS
/**
 * @brief Function for getting the index of the Identity Resolving Key that was used
 *        the last time an address was resolved.
 *
 * This function can be used to get the IRK index that matched the resolvable address,
 * provided that @ref NRF_AAR_EVENT_RESOLVED occured. Otherwise, it will return
 * the index of the last IRK stored in the IRK data structure.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The index of the IRK that was used the last time an address was resolved.
 */
NRF_STATIC_INLINE uint8_t nrf_aar_resolution_status_get(NRF_AAR_Type const * p_reg);
#endif // NRF_AAR_HAS_STATUS

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        AAR task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_aar_subscribe_set(NRF_AAR_Type * p_reg,
                                             nrf_aar_task_t task,
                                             uint8_t        channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        AAR task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_aar_subscribe_clear(NRF_AAR_Type * p_reg,
                                               nrf_aar_task_t task);

/**
 * @brief Function for getting the subscribe configuration for a given
 *        AAR task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to read the configuration.
 *
 * @return AAR subscribe configuration.
 */
NRF_STATIC_INLINE uint32_t nrf_aar_subscribe_get(NRF_AAR_Type const * p_reg,
                                                 nrf_aar_task_t       task);
/**
 * @brief Function for setting the publish configuration for a given
 *        AAR event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_aar_publish_set(NRF_AAR_Type *  p_reg,
                                           nrf_aar_event_t event,
                                           uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        AAR event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_aar_publish_clear(NRF_AAR_Type *  p_reg,
                                             nrf_aar_event_t event);

/**
 * @brief Function for getting the publish configuration for a given
 *        AAR event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to read the configuration.
 *
 * @return AAR publish configuration.
 */
NRF_STATIC_INLINE uint32_t nrf_aar_publish_get(NRF_AAR_Type const * p_reg,
                                               nrf_aar_event_t      event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

#ifndef NRF_DECLARE_ONLY
NRF_STATIC_INLINE bool nrf_aar_event_check(NRF_AAR_Type const * p_reg,
                                           nrf_aar_event_t      aar_event)
{
    return nrf_event_check(p_reg, aar_event);
}

NRF_STATIC_INLINE void nrf_aar_event_clear(NRF_AAR_Type *  p_reg,
                                           nrf_aar_event_t aar_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)aar_event)) = 0;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)aar_event);
}

NRF_STATIC_INLINE uint32_t nrf_aar_event_address_get(NRF_AAR_Type const * p_reg,
                                                     nrf_aar_event_t      aar_event)
{
    return nrf_task_event_address_get(p_reg, aar_event);
}

NRF_STATIC_INLINE void nrf_aar_int_enable(NRF_AAR_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE uint32_t nrf_aar_int_enable_check(NRF_AAR_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_aar_int_disable(NRF_AAR_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE void nrf_aar_task_trigger(NRF_AAR_Type * p_reg, nrf_aar_task_t task)
{
    *(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task) = 1;
}

NRF_STATIC_INLINE uint32_t nrf_aar_task_address_get(NRF_AAR_Type const * p_reg,
                                                    nrf_aar_task_t       task)
{
    return nrf_task_event_address_get(p_reg, task);
}

NRF_STATIC_INLINE void nrf_aar_enable(NRF_AAR_Type * p_reg)
{
    p_reg->ENABLE = AAR_ENABLE_ENABLE_Enabled << AAR_ENABLE_ENABLE_Pos;
}

NRF_STATIC_INLINE void nrf_aar_disable(NRF_AAR_Type * p_reg)
{
    p_reg->ENABLE = AAR_ENABLE_ENABLE_Disabled << AAR_ENABLE_ENABLE_Pos;
}

#if NRF_AAR_HAS_IRKPTR
NRF_STATIC_INLINE void nrf_aar_irk_pointer_set(NRF_AAR_Type * p_reg, uint8_t const * irk_ptr)
{
    p_reg->IRKPTR = (uint32_t)irk_ptr;
}

NRF_STATIC_INLINE uint8_t const * nrf_aar_irk_pointer_get(NRF_AAR_Type const * p_reg)
{
    return (uint8_t const *)(p_reg->IRKPTR);
}
#endif // NRF_AAR_HAS_IRKPTR

#if NRF_AAR_HAS_NIRK
NRF_STATIC_INLINE void nrf_aar_irk_number_set(NRF_AAR_Type * p_reg, uint8_t irk_num)
{
    p_reg->NIRK = irk_num;
}

NRF_STATIC_INLINE uint8_t nrf_aar_irk_number_get(NRF_AAR_Type const * p_reg)
{
    return (uint8_t)(p_reg->NIRK);
}
#endif // NRF_AAR_HAS_NIRK

#if NRF_AAR_HAS_ADDRPTR
NRF_STATIC_INLINE void nrf_aar_addr_pointer_set(NRF_AAR_Type * p_reg, uint8_t const * addr_ptr)
{
    p_reg->ADDRPTR = (uint32_t)addr_ptr;
}

NRF_STATIC_INLINE uint8_t const * nrf_aar_addr_pointer_get(NRF_AAR_Type const * p_reg)
{
    return (uint8_t const *)(p_reg->ADDRPTR);
}
#endif // NRF_AAR_HAS_ADDRPTR

#if NRF_AAR_HAS_SCRATCHPTR
NRF_STATIC_INLINE void nrf_aar_scratch_pointer_set(NRF_AAR_Type * p_reg, uint8_t * scratch_ptr)
{
    p_reg->SCRATCHPTR = (uint32_t)scratch_ptr;
}

NRF_STATIC_INLINE uint8_t * nrf_aar_scratch_pointer_get(NRF_AAR_Type const * p_reg)
{
    return (uint8_t *)(p_reg->SCRATCHPTR);
}
#endif // NRF_AAR_HAS_SCRATCHPTR

#if NRF_AAR_HAS_STATUS
NRF_STATIC_INLINE uint8_t nrf_aar_resolution_status_get(NRF_AAR_Type const * p_reg)
{
    return (uint8_t)(p_reg->STATUS);
}
#endif // NRF_AAR_HAS_STATUS

#if NRF_AAR_HAS_ERROR
NRF_STATIC_INLINE nrf_aar_error_t nrf_aar_error_get(NRF_AAR_Type const * p_reg)
{
    return (nrf_aar_error_t)(p_reg->ERRORSTATUS);
}
#endif // NRF_AAR_HAS_ERROR

#if NRF_AAR_HAS_MAXRESOLVED
NRF_STATIC_INLINE void nrf_aar_maxresolved_set(NRF_AAR_Type * p_reg, uint16_t maxresolved)
{
    p_reg->MAXRESOLVED = maxresolved;
}

NRF_STATIC_INLINE uint16_t nrf_aar_maxresolved_get(NRF_AAR_Type const * p_reg)
{
    return (uint16_t)(p_reg->MAXRESOLVED);
}
#endif // NRF_AAR_HAS_MAXRESOLVED

#if NRF_AAR_HAS_IN_PTR
NRF_STATIC_INLINE void nrf_aar_in_ptr_set(NRF_AAR_Type *         p_reg,
                                          nrf_vdma_job_t const * p_job)
{
    p_reg->IN.PTR = (uint32_t)p_job;
}

NRF_STATIC_INLINE nrf_vdma_job_t * nrf_aar_in_ptr_get(NRF_AAR_Type const * p_reg)
{
    return (nrf_vdma_job_t *)(p_reg->IN.PTR);
}
#endif // NRF_AAR_HAS_IN_PTR

#if NRF_AAR_HAS_OUT_PTR
NRF_STATIC_INLINE void nrf_aar_out_ptr_set(NRF_AAR_Type *         p_reg,
                                           nrf_vdma_job_t const * p_job)
{
    p_reg->OUT.PTR = (uint32_t)p_job;
}

NRF_STATIC_INLINE nrf_vdma_job_t * nrf_aar_out_ptr_get(NRF_AAR_Type const * p_reg)
{
    return (nrf_vdma_job_t *)(p_reg->OUT.PTR);
}
#endif // NRF_AAR_HAS_OUT_PTR

#if NRF_AAR_HAS_OUT_AMOUNT
NRF_STATIC_INLINE uint32_t nrf_aar_out_amount_get(NRF_AAR_Type const * p_reg)
{
    return p_reg->OUT.AMOUNT;
}
#endif // NRF_AAR_HAS_OUT_AMOUNT

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_aar_subscribe_set(NRF_AAR_Type * p_reg,
                                             nrf_aar_task_t task,
                                             uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | NRF_SUBSCRIBE_PUBLISH_ENABLE);
}

NRF_STATIC_INLINE void nrf_aar_subscribe_clear(NRF_AAR_Type * p_reg,
                                               nrf_aar_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE uint32_t nrf_aar_subscribe_get(NRF_AAR_Type const * p_reg,
                                                 nrf_aar_task_t       task)
{
    return *((volatile uint32_t const *) ((uint8_t const *) p_reg + (uint32_t) task + 0x80uL));
}

NRF_STATIC_INLINE void nrf_aar_publish_set(NRF_AAR_Type *  p_reg,
                                           nrf_aar_event_t event,
                                           uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | NRF_SUBSCRIBE_PUBLISH_ENABLE);
}

NRF_STATIC_INLINE void nrf_aar_publish_clear(NRF_AAR_Type *  p_reg,
                                             nrf_aar_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}

NRF_STATIC_INLINE uint32_t nrf_aar_publish_get(NRF_AAR_Type const * p_reg,
                                               nrf_aar_event_t      event)
{
    return *((volatile uint32_t const *) ((uint8_t const *) p_reg + (uint32_t) event + 0x80uL));
}
#endif // defined(DPPI_PRESENT)

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_AAR_H__
