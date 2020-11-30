/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/ipm.h>
#include <device.h>
#include <logging/log.h>

#include <openamp/open_amp.h>
#include <metal/device.h>

#include "nrf_802154_spinel_backend_callouts.h"
#include "nrf_802154_serialization_error.h"
#include "../../spinel_base/spinel.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME spinel_ipc_backend
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Configuration defines */
#if !DT_HAS_CHOSEN(zephyr_ipc_shm)
#error "Sample requires definition of shared memory for rpmsg"
#endif

#define IPC_MASTER IS_ENABLED(CONFIG_OPENAMP_MASTER)

#if IPC_MASTER
#define VIRTQUEUE_ID 0
#define RPMSG_ROLE RPMSG_MASTER
#else
#define VIRTQUEUE_ID 1
#define RPMSG_ROLE RPMSG_REMOTE
#endif

static K_SEM_DEFINE(ready_sem, 0, 1);
/* Configuration defines */

#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)

#define VDEV_START_ADDR		DT_REG_ADDR(SHM_NODE)
#define VDEV_SIZE		    DT_REG_SIZE(SHM_NODE)

#define VDEV_STATUS_ADDR	VDEV_START_ADDR
#define VDEV_STATUS_SIZE	0x400

#define SHM_START_ADDR		(VDEV_START_ADDR + VDEV_STATUS_SIZE)
#define SHM_SIZE		    (VDEV_SIZE - VDEV_STATUS_SIZE)
#define SHM_DEVICE_NAME		"sramx.shm"

#define VRING_COUNT		    2
#define VRING_RX_ADDRESS	(VDEV_START_ADDR + SHM_SIZE - VDEV_STATUS_SIZE)
#define VRING_TX_ADDRESS	(VDEV_START_ADDR + SHM_SIZE)
#define VRING_ALIGNMENT		4
#define VRING_SIZE		    16

#define IPM_WORK_QUEUE_STACK_SIZE 512

#if IS_ENABLED(CONFIG_COOP_ENABLED)
#define IPM_WORK_QUEUE_PRIORITY -1
#else
#define IPM_WORK_QUEUE_PRIORITY 0
#endif

K_THREAD_STACK_DEFINE(ipm_stack_area, IPM_WORK_QUEUE_STACK_SIZE);

struct k_work_q ipm_work_q;

/* End of configuration defines */

static const struct device *ipm_tx_handle;
static const struct device *ipm_rx_handle;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	{
		{
			.virt       = (void *) SHM_START_ADDR,
			.physmap    = shm_physmap,
			.size       = SHM_SIZE,
			.page_shift = 0xffffffff,
			.page_mask  = 0xffffffff,
			.mem_flags  = 0,
			.ops        = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.info.align = VRING_ALIGNMENT,
	},
	[1] = {
		.info.align = VRING_ALIGNMENT,
	},
};
static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *io;
static struct virtqueue *vq[2];
static struct rpmsg_endpoint ep;

#if IPC_MASTER
static struct rpmsg_virtio_shm_pool shpool;
#endif

static struct k_work ipm_work;

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
#if IPC_MASTER
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
#else
	return sys_read8(VDEV_STATUS_ADDR);
#endif
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static void virtio_set_features(struct virtio_device *vdev,
				uint32_t features)
{
}

// TODO: virtio_set_features is needed?

static void virtio_notify(struct virtqueue *vq)
{
	int status;

	status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
	if (status != 0) {
		LOG_ERR("ipm_send failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

static void ipm_callback_process(struct k_work *work)
{
	virtqueue_notification(vq[VIRTQUEUE_ID]);
}

static void ipm_callback(const struct device *dev, void *context, uint32_t id, volatile void *data)
{
    (void)dev;

	LOG_DBG("Got callback of id %u", id);
	/* TODO: Separate workqueue is needed only for serialization master (app core)
	Use sysworkq to optimize memory footprint for serialization slave (net core) */
	k_work_submit_to_queue(&ipm_work_q, &ipm_work);
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ep)
{
	rpmsg_destroy_ept(ep);
}

static int endpoint_cb(struct rpmsg_endpoint *ept,
		       void                  *data,
		       size_t                 len,
		       uint32_t               src,
		       void                  *priv)
{
	LOG_DBG("Received message of %u bytes.", len);

	k_sem_give(&ready_sem);

	if (len)
	{
		nrf_802154_spinel_encoded_packet_received(data, len);
	}

	return RPMSG_SUCCESS;
}

#if IPC_MASTER

static void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	(void)rpmsg_create_ept(&ep,
			       rdev,
			       name,
			       RPMSG_ADDR_ANY,
			       dest,
			       endpoint_cb,
			       rpmsg_service_unbind);

	k_sem_give(&ready_sem);
}

#endif


nrf_802154_ser_err_t nrf_802154_backend_init(void)
{
	int32_t                  err;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct metal_device     *device;

#if !IPC_MASTER
	struct rpmsg_device *rdev;
#endif

	LOG_DBG("Spinel backend initialization start");

	/* Start IPM workqueue */
	k_work_q_start(&ipm_work_q, ipm_stack_area,
		       K_THREAD_STACK_SIZEOF(ipm_stack_area),
		       IPM_WORK_QUEUE_PRIORITY);
	k_thread_name_set(&ipm_work_q.thread, "ipm_work_q");

	/* Setup IPM workqueue item */
	k_work_init(&ipm_work, ipm_callback_process);

	/* Libmetal setup */
	err = metal_init(&metal_params);
	if (err) {
		LOG_ERR("metal_init: failed - error code %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	err = metal_register_generic_device(&shm_device);
	if (err) {
		LOG_ERR("Couldn't register shared memory device: %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err) {
		LOG_ERR("metal_device_open failed: %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	io = metal_device_io_region(device, 0);
	if (!io) {
		LOG_ERR("metal_device_io_region failed to get region");
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	/* IPM setup */
#if IPC_MASTER
	ipm_tx_handle = device_get_binding("IPM_0");
	ipm_rx_handle = device_get_binding("IPM_1");
#else
	ipm_rx_handle = device_get_binding("IPM_0");
	ipm_tx_handle = device_get_binding("IPM_1");
#endif

	if (!ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	if (!ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	/* Virtqueue setup */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (!vq[0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[0]");
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (!vq[1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[1]");
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	rvrings[0].io = io;
	rvrings[0].info.vaddr = (void *)VRING_TX_ADDRESS;
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = io;
	rvrings[1].info.vaddr = (void *)VRING_RX_ADDRESS;
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.role = RPMSG_ROLE;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	vdev.vrings_info = &rvrings[0];

#if IPC_MASTER
	rpmsg_virtio_init_shm_pool(&shpool, (void *)SHM_START_ADDR, SHM_SIZE);
	err = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, io, &shpool);
#else
	err = rpmsg_init_vdev(&rvdev, &vdev, NULL, io, NULL);
#endif

	if (err) {
		LOG_ERR("rpmsg_init_vdev failed %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

#if IPC_MASTER
	/* Wait til nameservice ep is setup */
	k_sem_take(&ready_sem, K_FOREVER);

	/* Send dummy data so we can start communication in both directions.
	 * Simple NULL won't work as rpmsg_send won't send anything with NULL.
	 */
	rpmsg_send(&ep, (uint8_t*) "", 0);

    /* Wait until slave reports that it is ready. */
	k_sem_take(&ready_sem, K_FOREVER);
#else
	rdev = rpmsg_virtio_get_rpmsg_device(&rvdev);

	err = rpmsg_create_ept(&ep, rdev, "spinel_ipc", RPMSG_ADDR_ANY,
				  RPMSG_ADDR_ANY, endpoint_cb,
				  rpmsg_service_unbind);
	if (err) {
		LOG_ERR("rpmsg_create_ept failed %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

    /* Wait until master is ready. */
	k_sem_take(&ready_sem, K_FOREVER);

    /* Confirm slave is ready. */
	rpmsg_send(&ep, (uint8_t*) "", 0);
#endif
	LOG_DBG("Spinel backend initialized");

	return NRF_802154_SERIALIZATION_ERROR_OK;
}

// Send packet thread details
#define RING_BUFFER_LEN 4
#define SEND_THREAD_STACK_SIZE 1024

static K_SEM_DEFINE(send_sem, 0, RING_BUFFER_LEN);
K_THREAD_STACK_DEFINE(send_thread_stack, SEND_THREAD_STACK_SIZE);
struct k_thread send_thread_data;

typedef struct {
    uint32_t len;
    uint8_t data[SPINEL_FRAME_BUFFER_SIZE];
} buffer_t;

static buffer_t ring_buffer[RING_BUFFER_LEN];
static uint8_t rd_idx;
static uint8_t wr_idx;

static uint8_t get_rb_idx_plus_1(uint8_t i)
{
	return (i + 1) % RING_BUFFER_LEN;
}

static nrf_802154_ser_err_t spinel_packet_from_thread_send(const uint8_t * data, uint32_t len)
{
	if (get_rb_idx_plus_1(wr_idx) == rd_idx) {
		LOG_ERR("No spinel buffer available to send a new packet");
		return NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE;
	}

	LOG_DBG("Scheduling %u bytes for send thread", len);

	buffer_t *buf = &ring_buffer[wr_idx];
	wr_idx = get_rb_idx_plus_1(wr_idx);

	buf->len = len;
	memcpy(buf->data, data, len);

	k_sem_give(&send_sem);
	return (nrf_802154_ser_err_t)len;
}

static void spinel_packet_send_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Spinel backend send thread started");
	while (true) {
		k_sem_take(&send_sem, K_FOREVER);
        buffer_t *buf = &ring_buffer[rd_idx];
		uint32_t expected_ret = buf->len;
        LOG_DBG("Sending %u bytes from send thread", buf->len);
		int ret = rpmsg_send(&ep, buf->data, buf->len);

        rd_idx = get_rb_idx_plus_1(rd_idx);

		if (ret != expected_ret) {
			nrf_802154_ser_err_data_t err = {
				.reason = NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE,
			};
			nrf_802154_serialization_error(&err);
		}
	}
}

K_THREAD_DEFINE(spinel_packet_send_thread, SEND_THREAD_STACK_SIZE,
		spinel_packet_send_thread_fn, NULL, NULL, NULL, K_PRIO_COOP(0), 0, 0);

nrf_802154_ser_err_t nrf_802154_spinel_encoded_packet_send(const void *p_data,
                                                           size_t      data_len)
{
	if (k_is_in_isr()) {
		return spinel_packet_from_thread_send(p_data, data_len);
	}
	else {
        LOG_DBG("Sending %u bytes directly", data_len);
		int ret = rpmsg_send(&ep, p_data, data_len);
		return ((ret < 0) ? NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE :
												(nrf_802154_ser_err_t) ret);
	}
}

#if IPC_MASTER
/* Make sure we clear out the status flag very early (before we bringup the
 * secondary core) so the secondary core see's the proper status
 */
int init_status_flag(const struct device *arg)
{
	virtio_set_status(NULL, 0);

	return 0;
}

SYS_INIT(init_status_flag, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
