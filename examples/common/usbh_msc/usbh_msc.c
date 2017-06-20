/*
 * This file is part of the unicore-mx project.
 *
 * Copyright (C) 2017 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unicore-mx/usbh/usbh.h>
#include <unicore-mx/usbh/helper/ctrlreq.h>
#include <unicore-mx/usbh/class/msc.h>

#include "usbh_msc-target.h"

#if defined(USBH_DEBUG)
# define LOG_MARKER usart_puts("### ");
#else
# define LOG_MARKER
#endif

#define NEW_LINE "\n"

#define LOG_LN(str)		\
	LOG_MARKER			\
	usart_puts(str);	\
	usart_puts(NEW_LINE)

#define LOGF_LN(fmt, ...)				\
	LOG_MARKER							\
	usart_printf(fmt, ##__VA_ARGS__);	\
	usart_puts(NEW_LINE)

#define LOGF(fmt, ...)					\
	LOG_MARKER							\
	usart_printf(fmt, ##__VA_ARGS__);	\

#define LOG(str)						\
	LOG_MARKER							\
	usart_puts(str);					\

#if defined(USBH_DEBUG)
void usbh_log_puts(const char *str)
{
	usart_puts(str);
}

void usbh_log_printf(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	usart_vprintf(fmt, va);
	va_end(va);
}
#endif

uint8_t buf[512];
struct usbh_msc_bbb_arg bbb;

uint8_t msc_iface_num, msc_iface_altset;
uint8_t msc_max_lun;

#define CLASS_MSC 0x08
#define SUBCLASS_MSC_SCSI 0x06
#define PROTOCOL_MSC_BBB 0x50

static void readed_first_512_bytes(usbh_device *dev,
			usbh_transfer_status status,
			struct usbh_msc_bbb_arg *arg)
{
	(void) dev;
	(void) arg;

	unsigned i;

	if (status != USBH_SUCCESS) {
		LOGF_LN("Reading first 512 bytes was not successful (status = %i)", status);
		return;
	}

	LOGF_LN("Unable to read last %"PRIu32" bytes out of %"PRIu32" bytes",
		arg->csw.dCSWDataResidue, arg->cbw.dCBWDataTransferLength);

	LOG("DATA (first 16 bytes): ");
	for (i = 0; i < 16; i++) {
		LOGF("0x%"PRIX8", ", buf[i]);
	}
	LOG_LN("[END OF DATA]");
}

static void read_first_512_bytes(usbh_device *dev)
{
	memset(buf, 0xA3, sizeof(buf));

	usbh_msc_bbb_read10_cbw(&bbb.cbw, 0, 0, 512);
	bbb.data = buf;
	bbb.callback = readed_first_512_bytes;
	usbh_msc_bbb(dev, &bbb);
}

static void test_unit_ready_callback(usbh_device *dev,
			usbh_transfer_status status,
			struct usbh_msc_bbb_arg *arg)
{
	if (status != USBH_SUCCESS) {
		LOGF_LN("TEST UNIT READY transfer failed (status = %i)", status);
		return;
	}

	LOGF_LN("TEST UNIT READY: bCSWStatus = %"PRIu8, arg->csw.bCSWStatus);
	read_first_512_bytes(dev);
}

static void test_unit_ready(usbh_device *dev)
{
	usbh_msc_bbb_test_unit_ready_cbw(&bbb.cbw);
	bbb.callback = test_unit_ready_callback;
	usbh_msc_bbb(dev, &bbb);
}

static void inquiry_callback(usbh_device *dev,
			usbh_transfer_status status,
			struct usbh_msc_bbb_arg *arg)
{
	if (status != USBH_SUCCESS) {
		LOGF_LN("INQUIRY transfer failed (status = %i)", status);
		return;
	}

	LOGF_LN("INQUIRY: bCSWStatus = %"PRIu8, arg->csw.bCSWStatus);
	test_unit_ready(dev);
}

static void inquiry(usbh_device *dev)
{
	usbh_msc_bbb_inquiry_cbw(&bbb.cbw);
	bbb.callback = inquiry_callback;
	usbh_msc_bbb(dev, &bbb);
}

static void request_sense_callback(usbh_device *dev,
			usbh_transfer_status status,
			struct usbh_msc_bbb_arg *arg)
{
	if (status != USBH_SUCCESS) {
		LOGF_LN("REQUEST SENSE transfer failed (status = %i)", status);
		return;
	}

	LOGF_LN("REQUEST SENSE: bCSWStatus = %"PRIu8, arg->csw.bCSWStatus);
	inquiry(dev);
}

static void request_sense(usbh_device *dev)
{
	usbh_msc_bbb_request_sense_cbw(&bbb.cbw);
	bbb.callback = request_sense_callback;
	bbb.data = buf;
	usbh_msc_bbb(dev, &bbb);
}

static void get_max_lun_callback(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;
	(void) transfer;

	if (status == USBH_ERR_STALL) {
		LOG_LN("assuming MAX LUN = 0 since got STALLED");
	} else if (status != USBH_SUCCESS) {
		LOG_LN("failed to get MAX LUN value");
	} else {
		LOGF_LN("MAX LUN value: %"PRIu8, msc_max_lun);
	}

	request_sense(transfer->device);
}

static void bbb_interface_reset_callback(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to reset BBB interface");
	} else {
		LOG_LN("reset BBB interface success");
	}

	usbh_msc_get_max_lun(transfer->device, msc_iface_num, &msc_max_lun,
		get_max_lun_callback);
}

static void interface_set_callback(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to set interface, assuming the interface will work");
	} else {
		LOG_LN("set interface success, interface will work");
	}

	usbh_msc_bbb_reset(transfer->device, msc_iface_num, bbb_interface_reset_callback);
}

static void config_set_callback(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to set configuration!");
		return;
	}

	usbh_device_ep_dtog_reset_all(transfer->device);

	LOGF_LN("trying to interface num=%"PRIu8", altset=%"PRIu8,
		msc_iface_num, msc_iface_altset);
	usbh_ctrlreq_set_interface(transfer->device, msc_iface_num,
			msc_iface_altset, interface_set_callback);
}

static void got_conf_desc(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to read configuration descriptor 0!");
		return;
	}

	LOGF_LN("got %"PRIu16" bytes of configuration descriptor",
		transfer->transferred);


	struct usb_config_descriptor *cfg = transfer->data;
	if (!transfer->transferred || cfg->bLength < 4) {
		LOG_LN("descriptor is to small");
		return;
	}

	if (cfg->wTotalLength > transfer->transferred) {
		LOG_LN("descriptor only partially readed, "
			"what kind of storage device have such a big descriptor?*%!");
		return;
	}

	LOGF_LN("EP0 size: %"PRIu8, usbh_device_ep0_size(transfer->device));
	LOGF_LN("DEVICE SPEED: %i", usbh_device_speed(transfer->device));

	int len = cfg->wTotalLength - cfg->bLength;
	void *ptr = transfer->data + cfg->bLength;
	struct usb_interface_descriptor *iface_interest = NULL;
	struct usb_endpoint_descriptor *ep_interest_in = NULL;
	struct usb_endpoint_descriptor *ep_interest_out = NULL;

	while (len > 0) {
		uint8_t *d = ptr;
		LOGF_LN("trying: descriptor with length = %"PRIu8
			" and type = 0x%"PRIx8, d[0], d[1]);

		switch (d[1]) {
		case USB_DT_INTERFACE: {
			struct usb_interface_descriptor *iface = ptr;
			if (iface->bInterfaceClass == CLASS_MSC &&
				iface->bInterfaceSubClass == SUBCLASS_MSC_SCSI &&
				iface->bInterfaceProtocol == PROTOCOL_MSC_BBB &&
				iface->bNumEndpoints) {
				iface_interest = iface;
				LOGF_LN("interface %"PRIu8" (alt set: %"PRIu8") "
					"could be of interest",
					iface->bInterfaceNumber, iface->bAlternateSetting);
			} else {
				iface_interest = NULL;
			}

			ep_interest_in = ep_interest_out = NULL;
		} break;
		case USB_DT_ENDPOINT: {
			struct usb_endpoint_descriptor *ep = ptr;
			if ((ep->bmAttributes & USB_ENDPOINT_ATTR_TYPE) ==
						USB_ENDPOINT_ATTR_BULK) {
				if (ep->bEndpointAddress & 0x80) {
					ep_interest_in = ep;
				} else {
					ep_interest_out = ep;
				}
			}
		} break;
		}

		if (iface_interest != NULL && ep_interest_in != NULL && ep_interest_out != NULL) {
			break;
		}

		ptr += d[0];
		len -= d[0];
	}

	if (iface_interest == NULL || ep_interest_in == NULL || ep_interest_out == NULL) {
		LOG_LN("no valid MSC interface");
		return;
	}

	LOG_LN("got a valid MSC interface");
	msc_iface_num = iface_interest->bInterfaceNumber;
	msc_iface_altset = iface_interest->bAlternateSetting;

	bbb.ep.in.addr = ep_interest_in->bEndpointAddress;
	bbb.ep.in.size = ep_interest_in->wMaxPacketSize;

	bbb.ep.out.addr = ep_interest_out->bEndpointAddress;
	bbb.ep.out.size = ep_interest_out->wMaxPacketSize;

	bbb.timeout.cbw =
	bbb.timeout.data_in =
	bbb.timeout.data_out =
	bbb.timeout.csw = 100;

	LOGF_LN("trying to set config: %"PRIu8, cfg->bConfigurationValue);
	usbh_ctrlreq_set_config(transfer->device, cfg->bConfigurationValue,
			config_set_callback);
}

static void got_dev_desc(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to read device descriptor!");
		return;
	}

	struct usb_device_descriptor *desc = transfer->data;

	if (!desc->bNumConfigurations) {
		LOG_LN("device do not have any configuration"
			"**THROW IT OUT OF THE WINDOW!**");
		return;
	}

	uint8_t class = desc->bDeviceClass;
	uint8_t subclass = desc->bDeviceSubClass;
	uint8_t protocol = desc->bDeviceProtocol;

	LOGF_LN("bDeviceClass: 0x%"PRIx32, class);
	LOGF_LN("bDeviceSubClass: 0x%"PRIx32, subclass);
	LOGF_LN("bDeviceProtocol: 0x%"PRIx32, protocol);

	if (class == CLASS_MSC && subclass == SUBCLASS_MSC_SCSI &&
			protocol == PROTOCOL_MSC_BBB) {
		LOG_LN("device is a MSC SCSI BBB of our interest");
	} else if (!class || !subclass || !protocol) {
		LOG_LN("have to look into configuration descriptor");
	} else {
		LOG_LN("device is not a MSC interface");
		return;
	}

	usbh_device *dev = transfer->device;
	usbh_ctrlreq_read_desc(dev, USB_DT_CONFIGURATION, 0, buf, 128, got_conf_desc);
}

/**
 * when a device is connected, the following sequence is followed
 *  - fetch full device descriptor
 *  [search for valid MSC device]
 *  - fetch configuration descriptor
 *  [search for valid MSC interface]
 *  - set configuration
 *  - set interface
 *  - BBB reset interface
 *  - get MAX LUN
 *  - read 512 from LBA = 0
 */

static void device_disconnected(usbh_device *dev)
{
	(void) dev;

	LOG_LN("mass storage device disconnected!");
}

static void got_a_device(usbh_device *dev)
{
	LOG_LN("finally got a device!");
	usbh_device_register_disconnected_callback(dev, device_disconnected);
	usbh_ctrlreq_read_desc(dev, USB_DT_DEVICE, 0, buf, 18, got_dev_desc);
}

const usbh_backend_config * __attribute__((weak))
usbh_msc_config(void) { return NULL; }

void __attribute__((weak)) usbh_msc_before_poll(void) {}
void __attribute__((weak)) usbh_msc_after_poll(void) {}

int main(void)
{
	usbh_msc_init();

	usart_puts("\n\n\n\n======STARTING=============\n");

	usbh_host *host = usbh_init(usbh_msc_backend(), usbh_msc_config());
	usbh_register_connected_callback(host, got_a_device);

	uint16_t last = tim_get_counter();

	while (1) {
		usbh_msc_before_poll();

		uint16_t now = tim_get_counter();
		uint16_t diff = (last > now) ? (now + (0xFFFF - last)) : (now - last);
		uint32_t diff_us = diff * 100;

		usbh_poll(host, diff_us);

		last = now;

		usbh_msc_after_poll();
	}

	return 0;
}
