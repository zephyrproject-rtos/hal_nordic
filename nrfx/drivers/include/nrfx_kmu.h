/*$$$LICENCE_NORDIC_STANDARD<2026>$$$*/

#ifndef NRFX_KMU_H__
#define NRFX_KMU_H__

#include <nrfx.h>
#include <hal/nrf_kmu.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_kmu KMU driver
 * @{
 * @ingroup nrf_kmu
 * @brief   Key Management Unit (KMU) peripheral driver.
 */

/** @brief Macro for calculating how many 32-bit words can be placed in one key slot. */
#define KEY_SLOT_WORDS_COUNT ((KMU_KEYSLOTBITS) / 32)

#if NRF_KMU_HAS_REVOKE_POLICY || defined(__NRFX_DOXYGEN__)
/** @brief KMU revoke policy type. */
typedef enum
{
    NRFX_KMU_RPOLICY_LOCKED   = KMU_TASKS_REVOKE_POLICY_Locked,   ///< Locked revoke policy.
    NRFX_KMU_RPOLICY_ROTATING = KMU_TASKS_REVOKE_POLICY_Rotating, ///< Rotating revoke policy.
    NRFX_KMU_RPOLICY_REVOKED  = KMU_TASKS_REVOKE_POLICY_Revoked,  ///< Revoked revoke policy.
} nrfx_kmu_rpolicy_t;
#endif

#if NRF_KMU_HAS_METADATA || defined(__NRFX_DOXYGEN__)
/** @brief KMU metadata struct type. */
typedef struct {
    uint32_t metadata;     ///< Metadata register.
#if NRF_KMU_HAS_METADATAEXT || defined(__NRFX_DOXYGEN__)
    uint32_t metadata_ext; ///< Metadata extended register.
#endif
} nrfx_kmu_key_slot_metadata_t;
#endif

/** @brief Key slot data structure. */
typedef struct __PACKED
{
    uint32_t                     keyslot_value[KEY_SLOT_WORDS_COUNT]; ///< Key data to be provisioned.
#if NRF_KMU_HAS_REVOKE_POLICY || defined(__NRFX_DOXYGEN__)
    uint32_t                     revoke_policy;                       /**< Key revoke policy.
                                                                       *   @ref nrfx_kmu_rpolicy_t
                                                                       *   holds possible values. */
#endif
    uint32_t                     keyslot_dest;                        /**< Key slot destination when
                                                                       *   performing key push. */
#if NRF_KMU_HAS_METADATA || defined(__NRFX_DOXYGEN__)
    nrfx_kmu_key_slot_metadata_t metadata;                            ///< Metadata to write to keyslot.
#endif
} nrfx_kmu_key_slot_data_t;

/**
 * @brief KMU driver event handler type.
 *
 * @param[in] event_type KMU event.
*/
typedef void (* nrfx_kmu_event_handler_t)(nrf_kmu_event_t event_type);

/**
 * @brief Function for initializing KMU driver.
 *
 * @param[in] handler Event handler. Used only if KMU interrupts are present on a device (can be
 *                    checked with @ref NRF_KMU_HAS_INTEN)
 *
 * @retval 0         Initialization successful.
 * @retval -EALREADY The driver is already initialized.
 */
int nrfx_kmu_init(nrfx_kmu_event_handler_t * handler);

/** @brief Function for uninitializing KMU driver. */
void nrfx_kmu_uninit(void);

/**
 * @brief Function for provisioning data to given key slot in KMU.
 *
 * @note Requires enabling writing to non-volatile memory.
 *
 * @param[in] p_key_slot_data Pointer to the key slot data structure.
 * @param[in] slot_id         ID of a slot to use.
 *
 * @retval 0          Provisioning was successful.
 * @retval -EFAULT    An error occured when provisioning the key.
 * @retval -ETIMEDOUT Timeout occured when waiting for result event.
 */
int nrfx_kmu_key_slot_provision(nrfx_kmu_key_slot_data_t const * p_key_slot_data, uint32_t slot_id);

/**
 * @brief Function for pushing @p num_slots consecutive key slots from KMU to address specified in
 *        @p keyslot_dest field in @ref nrfx_kmu_key_slot_data_t at provisioning.
 *
 * @param[in] slot_id   ID of a slot to use.
 * @param[in] num_slots Number of consecutive keys slots to push.
 *
 * @retval 0          Pushing was successful.
 * @retval -EACCES    The key slot is revoked.
 * @retval -EFAULT    An error occured when pushing the key slot (slot is bloked or empty).
 * @retval -ETIMEDOUT Timeout occured when waiting for result event.
 */
int nrfx_kmu_key_slots_push(uint32_t slot_id, uint32_t num_slots);

#if NRF_KMU_HAS_PUSH_BLOCK || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for blocking @p num_slots consecutive key slots from being pushed.
 *
 * @param[in] slot_id   ID of a slot to use.
 * @param[in] num_slots Number of consecutive keys slots to push block.
 *
 * @retval 0          Blocking was successful.
 * @retval -EFAULT    An error occured when push blocking the key slot.
 * @retval -ETIMEDOUT Timeout occured when waiting for result event.
 */
int nrfx_kmu_key_slots_push_block(uint32_t slot_id, uint32_t num_slots);
#endif

#if NRF_KMU_HAS_BLOCK || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for blocking @p num_slots consecutive key slots from being pushed or revoked.
 *
 * @param[in] slot_id   ID of a slot to use.
 * @param[in] num_slots Number of consecutive keys slots to block.
 *
 * @retval 0          Blocking was successful.
 * @retval -EFAULT    An error occured when blocking the key slot.
 * @retval -ETIMEDOUT Timeout occured when waiting for result event.
 */
int nrfx_kmu_key_slots_block(uint32_t slot_id, uint32_t num_slots);
#endif

/**
 * @brief Function for revoking @p num_slots consecutive key slots.
 *
 * @param[in] slot_id   ID of a slot to use.
 * @param[in] num_slots Number of consecutive keys slots to revoke.
 *
 * @retval 0          Revoking was successful.
 * @retval -EFAULT    An error occured when revoking the key slot.
 * @retval -ETIMEDOUT Timeout occured when waiting for result event.
 */
int nrfx_kmu_key_slots_revoke(uint32_t slot_id, uint32_t num_slots);

#if NRF_KMU_HAS_METADATA || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for reading metadata from a key slot.
 *
 * @param[in]  slot_id    ID of a slot to use.
 * @param[out] p_metadata Pointer to a metadata structure to be filled.
 *
 * @retval 0          Metadata was read.
 * @retval -EACCES    The key slot is revoked.
 * @retval -EFAULT    The key slot is empty.
 * @retval -ETIMEDOUT Timeout occured when waiting for result event.
 */
int nrfx_kmu_key_slot_metadata_read(uint32_t slot_id, nrfx_kmu_key_slot_metadata_t * p_metadata);
#endif

/**
 * @brief Function for checking if given @p num_slots consecutive key slots can be written to.
 *
 * @param[in] slot_id   ID of a slot to check.
 * @param[in] num_slots Number of consecutive keys slots to check.
 *
 * @retval true  Key slots are empty.
 * @retval false At least one of the key slots is occupied or revoked (when non-rotating policy was used).
 */
bool nrfx_kmu_key_slots_empty_check(uint32_t slot_id, uint32_t num_slots);

/**
 * @brief Function for checking if given @p num_slots consecutive key slots are revoked.
 *
 * @param[in] slot_id   ID of a slot to check.
 * @param[in] num_slots Number of consecutive keys slots to check.
 *
 * @retval true  Key slots are revoked.
 * @retval false At least one of the key slots is either empty or occupied.
 */
bool nrfx_kmu_key_slots_revoked_check(uint32_t slot_id, uint32_t num_slots);

/**
 * @brief Function for finding first @p num_slots consecutive available key slots.
 *
 * @param[in] num_slots Number of consecutive key slots to find.
 *
 * @retval non-negative Slot ID of the first available key slot.
 * @retval -ENOMEM      num_slots consecutive empty key slots were not found, too many occupied
 *                      and/or revoked key slots.
 */
int nrfx_kmu_next_available_key_slots_find(uint32_t num_slots);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_KMU_H__
