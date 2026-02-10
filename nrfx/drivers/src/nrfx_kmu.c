/*$$$LICENCE_NORDIC_STANDARD<2026>$$$*/

#include <nrfx.h>
#include <nrfx_kmu.h>

#define NRFX_LOG_MODULE KMU
#include <nrfx_log.h>

#if defined(NRF53_SERIES) || defined(NRF91_SERIES)
#error "Current implementation of KMU driver does not support nRF53 and nRF91 devices"
#endif

/** @cond Driver internal data. */
typedef struct
{
    nrfx_drv_state_t state;
} nrfx_kmu_control_block_t;

static nrfx_kmu_control_block_t m_cb;

static void clear_all_events(void)
{
#if NRF_KMU_HAS_PROVISION
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_EVENTS_PROVISIONED);
#endif
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_KEYSLOT_PUSHED);
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_KEYSLOT_REVOKED);
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_KEYSLOT_ERROR);
#if NRF_KMU_HAS_METADATA
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_EVENTS_EVENTS_METADATA_READ);
#endif
#if NRF_KMU_HAS_PUSH_BLOCK
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_EVENTS_EVENTS_PUSHBLOCKED);
#endif
#if NRF_KMU_HAS_BLOCK
    nrf_kmu_event_clear(NRF_KMU, NRF_KMU_EVENT_EVENTS_EVENTS_BLOCKED);
#endif
}

static int wait_for_task_result(nrf_kmu_event_t event)
{
    bool err;

    NRFX_WAIT_FOR(nrf_kmu_event_check(NRF_KMU, event) ||
                  nrf_kmu_event_check(NRF_KMU, NRF_KMU_EVENT_KEYSLOT_REVOKED) ||
                  nrf_kmu_event_check(NRF_KMU, NRF_KMU_EVENT_KEYSLOT_ERROR),
                  500, 10, err);

    if (!err)
    {
        return -ETIMEDOUT;
    }

    if (nrf_kmu_event_check(NRF_KMU, event))
    {
        clear_all_events();
        return 0;
    }
    else if (nrf_kmu_event_check(NRF_KMU, NRF_KMU_EVENT_KEYSLOT_REVOKED))
    {
        clear_all_events();
        return -EACCES;
    }

    clear_all_events();

    return -EFAULT;
}

static bool key_slot_empty_check(uint32_t slot_id)
{
    while (nrf_kmu_status_get(NRF_KMU));

    nrf_kmu_keyslot_set(NRF_KMU, slot_id);

    nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_READ_METADATA);

    int err = wait_for_task_result(NRF_KMU_EVENT_EVENTS_EVENTS_METADATA_READ);

    return err == -EFAULT;
}

static bool key_slot_revoked_check(uint32_t slot_id)
{
    while (nrf_kmu_status_get(NRF_KMU));

    nrf_kmu_keyslot_set(NRF_KMU, slot_id);

    nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_READ_METADATA);

    int err = wait_for_task_result(NRF_KMU_EVENT_EVENTS_EVENTS_METADATA_READ);

    return err == -EACCES;
}

int nrfx_kmu_init(nrfx_kmu_event_handler_t* event_handler)
{
    (void)event_handler;

    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        int err_code = -EALREADY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    clear_all_events();

    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    return 0;
}

void nrfx_kmu_uninit(void)
{
    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
}

int nrfx_kmu_key_slot_provision(nrfx_kmu_key_slot_data_t const * p_key_slot_data, uint32_t slot_id)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (p_key_slot_data) &&
                (slot_id < KMU_KEYSLOTNUM));

    while (nrf_kmu_status_get(NRF_KMU));

    nrf_kmu_src_set(NRF_KMU, (uint32_t)p_key_slot_data);
    nrf_kmu_keyslot_set(NRF_KMU, slot_id);
    nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_PROVISION_KEYSLOT);

    return wait_for_task_result(NRF_KMU_EVENT_EVENTS_PROVISIONED);
}

int nrfx_kmu_key_slots_push(uint32_t slot_id, uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (slot_id + num_slots - 1 < KMU_KEYSLOTNUM) &&
                (num_slots > 0));

    for (uint32_t i = slot_id; i < slot_id + num_slots; i++)
    {
        while (nrf_kmu_status_get(NRF_KMU));

        nrf_kmu_keyslot_set(NRF_KMU, i);
        nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_PUSH_KEYSLOT);

        int err = wait_for_task_result(NRF_KMU_EVENT_KEYSLOT_PUSHED);

        if (err < 0)
        {
            return err;
        }
    }

    return 0;
}

#if NRF_KMU_HAS_PUSH_BLOCK
int nrfx_kmu_key_slots_push_block(uint32_t slot_id, uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (slot_id + num_slots - 1 < KMU_KEYSLOTNUM) &&
                (num_slots > 0));

    for (uint32_t i = slot_id; i < slot_id + num_slots; i++)
    {
        while (nrf_kmu_status_get(NRF_KMU));

        nrf_kmu_keyslot_set(NRF_KMU, i);
        nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_PUSH_BLOCK);

        int err = wait_for_task_result(NRF_KMU_EVENT_EVENTS_EVENTS_PUSHBLOCKED);

        if (err < 0)
        {
            return err;
        }
    }

    return 0;
}
#endif

#if NRF_KMU_HAS_BLOCK
int nrfx_kmu_key_slots_block(uint32_t slot_id, uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (slot_id + num_slots - 1 < KMU_KEYSLOTNUM) &&
                (num_slots > 0));

    for (uint32_t i = slot_id; i < slot_id + num_slots; i++)
    {
        while (nrf_kmu_status_get(NRF_KMU));

        nrf_kmu_keyslot_set(NRF_KMU, i);
        nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_BLOCK);

        int err = wait_for_task_result(NRF_KMU_EVENT_EVENTS_EVENTS_BLOCKED);

        if (err < 0)
        {
            return err;
        }
    }

    return 0;
}
#endif

int nrfx_kmu_key_slots_revoke(uint32_t slot_id, uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (slot_id + num_slots - 1 < KMU_KEYSLOTNUM) &&
                (num_slots > 0));

    for (uint32_t i = slot_id; i < slot_id + num_slots; i++)
    {
        while (nrf_kmu_status_get(NRF_KMU));

        nrf_kmu_keyslot_set(NRF_KMU, i);
        nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_REVOKE_KEYSLOT);

        int err = wait_for_task_result(NRF_KMU_EVENT_KEYSLOT_REVOKED);

        if (err < 0)
        {
            return err;
        }
    }

    return 0;
}

#if NRF_KMU_HAS_METADATA
int nrfx_kmu_key_slot_metadata_read(uint32_t slot_id, nrfx_kmu_key_slot_metadata_t* p_metadata)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (p_metadata));

    nrf_kmu_keyslot_set(NRF_KMU, slot_id);

    nrf_kmu_task_trigger(NRF_KMU, NRF_KMU_TASK_READ_METADATA);

    int err = wait_for_task_result(NRF_KMU_EVENT_EVENTS_EVENTS_METADATA_READ);

    if (err < 0)
    {
        return err;
    }

    p_metadata->metadata = nrf_kmu_metadata_get(NRF_KMU);
#if NRF_KMU_HAS_METADATAEXT
    p_metadata->metadata_ext = nrf_kmu_metadataext_get(NRF_KMU);
#endif

    return 0;
}
#endif

bool nrfx_kmu_key_slots_empty_check(uint32_t slot_id, uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (slot_id + num_slots - 1 < KMU_KEYSLOTNUM) &&
                (num_slots > 0));

    for (uint32_t i = slot_id; i < slot_id + num_slots; i++)
    {
        if (!key_slot_empty_check(i))
        {
            return false;
        }
    }

    return true;
}

bool nrfx_kmu_key_slots_revoked_check(uint32_t slot_id, uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (slot_id < KMU_KEYSLOTNUM) &&
                (slot_id + num_slots - 1 < KMU_KEYSLOTNUM) &&
                (num_slots > 0));

    for (uint32_t i = slot_id; i < slot_id + num_slots; i++)
    {
        if (!key_slot_revoked_check(i))
        {
            return false;
        }
    }

    return true;
}

int nrfx_kmu_next_available_key_slots_find(uint32_t num_slots)
{
    NRFX_ASSERT((m_cb.state == NRFX_DRV_STATE_INITIALIZED) &&
                (num_slots > 0));

    uint32_t first_key_slot = 0;
    bool potential_key_slot = false;

    for (uint32_t i = 0; i < KMU_KEYSLOTNUM; i++)
    {
        bool slot_empty = key_slot_empty_check(i);
        if (slot_empty && !potential_key_slot)
        {
            potential_key_slot = true;
            first_key_slot = i;
        }
        if (potential_key_slot)
        {
            if (slot_empty && (i - first_key_slot + 1 == num_slots))
            {
                return (int)first_key_slot;
            }
            else if (!slot_empty)
            {
                potential_key_slot = false;
            }
        }
    }

    return -ENOMEM;
}
