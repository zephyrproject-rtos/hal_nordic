/*$$$LICENCE_NORDIC_STANDARD<2017>$$$*/

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

// THIS IS A TEMPLATE FILE.
// It should be copied to a suitable location within the host environment into
// which nrfx is integrated, and the following macros should be provided with
// appropriate implementations.
// And this comment should be removed from the customized file.

#ifdef __cplusplus
extern "C" {
#endif

/* CMSIS core intrinsics (NVIC_*, __disable_irq, __CLZ) for the driver glue
 * below. The HAL needs none of this; only nrfx *drivers* (e.g. nrfx_uarte) do. */
#include <nrf.h>

/**
 * @defgroup nrfx_glue nrfx_glue.h
 * @{
 * @ingroup nrfx
 *
 * @brief This file contains macros that should be implemented according to
 *        the needs of the host environment into which @em nrfx is integrated.
 */

//------------------------------------------------------------------------------

/**
 * @brief Macro for placing a runtime assertion.
 *
 * @param expression Expression to be evaluated.
 */
#define NRFX_ASSERT(expression)

/**
 * @brief Macro for placing a compile time assertion.
 *
 * @param expression Expression to be evaluated.
 */
#define NRFX_STATIC_ASSERT(expression)

//------------------------------------------------------------------------------

/**
 * @brief Macro for setting the priority of a specific IRQ.
 *
 * @param irq_number IRQ number.
 * @param priority   Priority to be set.
 */
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority) \
	NVIC_SetPriority((IRQn_Type)(irq_number), priority)

/**
 * @brief Macro for enabling a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_ENABLE(irq_number) NVIC_EnableIRQ((IRQn_Type)(irq_number))

/**
 * @brief Macro for checking if a specific IRQ is enabled.
 *
 * @param irq_number IRQ number.
 *
 * @retval true  If the IRQ is enabled.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_ENABLED(irq_number) (NVIC_GetEnableIRQ((IRQn_Type)(irq_number)) != 0)

/**
 * @brief Macro for disabling a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_DISABLE(irq_number) NVIC_DisableIRQ((IRQn_Type)(irq_number))

/**
 * @brief Macro for setting a specific IRQ as pending.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_PENDING_SET(irq_number) NVIC_SetPendingIRQ((IRQn_Type)(irq_number))

/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_PENDING_CLEAR(irq_number) NVIC_ClearPendingIRQ((IRQn_Type)(irq_number))

/**
 * @brief Macro for checking the pending status of a specific IRQ.
 *
 * @retval true  If the IRQ is pending.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_PENDING(irq_number) (NVIC_GetPendingIRQ((IRQn_Type)(irq_number)) != 0)

/** @brief Macro for entering into a critical section. */
#define NRFX_CRITICAL_SECTION_ENTER() __disable_irq()

/** @brief Macro for exiting from a critical section. */
#define NRFX_CRITICAL_SECTION_EXIT() __enable_irq()

//------------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that
 *        @ref nrfx_coredep_delay_us uses a precise DWT-based solution.
 *        A compilation error is generated if the DWT unit is not present
 *        in the SoC used.
 */
#define NRFX_COREDEP_DELAY_DWT_BASED 0

/**
 * @brief Macro for delaying the code execution for at least the specified time.
 *
 * @param us_time Number of microseconds to wait.
 */
#define NRFX_DELAY_US(us_time)                                                       \
	do {                                                                         \
		/* ~CPU_FREQ_MHZ iterations per us; 3 cycles/iter, rounded \
		 * up. Good enough for driver bring-up delays. */         \
		volatile uint32_t _n =                                               \
			(uint32_t)(us_time) * ((NRF_CONFIG_CPU_FREQ_MHZ + 2U) / 3U); \
		while (_n--) {                                                       \
			__asm__ volatile("nop");                                     \
		}                                                                    \
	} while (0)

//------------------------------------------------------------------------------

/** @brief Atomic 32-bit unsigned type. */
#define nrfx_atomic_t uint32_t

/**
 * @brief Macro for storing a value to an atomic object and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value to store.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_STORE(p_data, value) \
	__atomic_exchange_n((p_data), (value), __ATOMIC_SEQ_CST)

/**
 * @brief Macro for running a bitwise OR operation on an atomic object and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the OR operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_OR(p_data, value) __atomic_fetch_or((p_data), (value), __ATOMIC_SEQ_CST)

/**
 * @brief Macro for running a bitwise AND operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the AND operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_AND(p_data, value) __atomic_fetch_and((p_data), (value), __ATOMIC_SEQ_CST)

/**
 * @brief Macro for running a bitwise XOR operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the XOR operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_XOR(p_data, value) __atomic_fetch_xor((p_data), (value), __ATOMIC_SEQ_CST)

/**
 * @brief Macro for running an addition operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the ADD operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_ADD(p_data, value) __atomic_fetch_add((p_data), (value), __ATOMIC_SEQ_CST)

/**
 * @brief Macro for running a subtraction operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the SUB operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_SUB(p_data, value) __atomic_fetch_sub((p_data), (value), __ATOMIC_SEQ_CST)

/**
 * @brief Macro for running compare and swap on an atomic object.
 *
 * Value is updated to the new value only if it previously equaled old value.
 *
 * @param[in,out] p_data    Atomic memory pointer.
 * @param[in]     old_value Expected old value.
 * @param[in]     new_value New value.
 *
 * @retval true  If value was updated.
 * @retval false If value was not updated because location was not equal to @p old_value.
 */
#define NRFX_ATOMIC_CAS(p_data, old_value, new_value)                                              \
	({                                                                                         \
		uint32_t _exp = (old_value);                                                       \
		__atomic_compare_exchange_n((p_data), &_exp, (new_value), false, __ATOMIC_SEQ_CST, \
					    __ATOMIC_SEQ_CST);                                     \
	})

/**
 * @brief Macro for counting leading zeros.
 *
 * @param[in] value A word value.
 *
 * @return Number of leading 0-bits in @p value, starting at the most significant bit position.
 *         If x is 0, the result is undefined.
 */
#define NRFX_CLZ(value) __builtin_clz(value)

/**
 * @brief Macro for counting trailing zeros.
 *
 * @param[in] value A word value.
 *
 * @return Number of trailing 0-bits in @p value, starting at the least significant bit position.
 *         If x is 0, the result is undefined.
 */
#define NRFX_CTZ(value) __builtin_ctz(value)

//------------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that inside HALs
 *        the event registers are read back after clearing, on devices that
 *        otherwise could defer the actual register modification.
 */
#define NRFX_EVENT_READBACK_ENABLED 1

//------------------------------------------------------------------------------

/**
 * @brief Macro for writing back cache lines associated with the specified buffer.
 *
 * @note The part has no data cache, so the macro only uses its arguments, which keeps
 *       callers free of unused-variable and unused-parameter warnings.
 *
 * @param[in] p_buffer Pointer to the buffer.
 * @param[in] size     Size of the buffer.
 */
#define NRFY_CACHE_WB(p_buffer, size) \
	do {                          \
		(void)(p_buffer);     \
		(void)(size);         \
	} while (0)

/**
 * @brief Macro for invalidating cache lines associated with the specified buffer.
 *
 * @note The part has no data cache, so the macro only uses its arguments, which keeps
 *       callers free of unused-variable and unused-parameter warnings.
 *
 * @param[in] p_buffer Pointer to the buffer.
 * @param[in] size     Size of the buffer.
 */
#define NRFY_CACHE_INV(p_buffer, size) \
	do {                           \
		(void)(p_buffer);      \
		(void)(size);          \
	} while (0)

/**
 * @brief Macro for writing back and invalidating cache lines associated with
 *        the specified buffer.
 *
 * @note The part has no data cache, so the macro only uses its arguments, which keeps
 *       callers free of unused-variable and unused-parameter warnings.
 *
 * @param[in] p_buffer Pointer to the buffer.
 * @param[in] size     Size of the buffer.
 */
#define NRFY_CACHE_WBINV(p_buffer, size) \
	do {                             \
		(void)(p_buffer);        \
		(void)(size);            \
	} while (0)

//------------------------------------------------------------------------------

/** @brief Bitmask that defines DPPI channels that are reserved for use outside of the nrfx library. */
#define NRFX_DPPI_CHANNELS_USED 0

/** @brief Bitmask that defines DPPI groups that are reserved for use outside of the nrfx library. */
#define NRFX_DPPI_GROUPS_USED 0

/** @brief Bitmask that defines PPI channels that are reserved for use outside of the nrfx library. */
#define NRFX_PPI_CHANNELS_USED 0

/** @brief Bitmask that defines PPI groups that are reserved for use outside of the nrfx library. */
#define NRFX_PPI_GROUPS_USED 0

/** @brief Bitmask that defines GPIOTE channels that are reserved for use outside of the nrfx library. */
#define NRFX_GPIOTE_CHANNELS_USED 0

/** @brief Bitmask that defines EGU instances that are reserved for use outside of the nrfx library. */
#define NRFX_EGUS_USED 0

/** @brief Bitmask that defines TIMER instances that are reserved for use outside of the nrfx library. */
#define NRFX_TIMERS_USED 0

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_GLUE_H__
