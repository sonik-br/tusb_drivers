#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_GUNCON2)

#include "host/usbh.h"
#include "host/usbh_classdriver.h"
#include "class/hid/hid.h"
#include "guncon2_host.h"

typedef struct
{
    uint8_t inst_count;
    guncon2h_interface_t instances[CFG_TUH_GUNCON2];
} guncon2h_device_t;

static guncon2h_device_t _guncon2h_dev[CFG_TUH_DEVICE_MAX];

TU_ATTR_ALWAYS_INLINE static inline guncon2h_device_t *get_dev(uint8_t dev_addr)
{
    return &_guncon2h_dev[dev_addr - 1];
}

TU_ATTR_ALWAYS_INLINE static inline guncon2h_interface_t *get_instance(uint8_t dev_addr, uint8_t instance)
{
    return &_guncon2h_dev[dev_addr - 1].instances[instance];
}

static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr)
{
    for (uint8_t inst = 0; inst < CFG_TUH_GUNCON2; inst++)
    {
        guncon2h_interface_t *hid = get_instance(dev_addr, inst);

        if ((ep_addr == hid->ep_in) || (ep_addr == hid->ep_out))
            return inst;
    }

    return 0xff;
}

static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf)
{
    for (uint8_t inst = 0; inst < CFG_TUH_GUNCON2; inst++)
    {
        guncon2h_interface_t *hid = get_instance(dev_addr, inst);

        if ((hid->itf_num == itf) && (hid->ep_in || hid->ep_out))
            return inst;
    }

    return 0xff;
}

bool tuh_guncon2_n_ready(uint8_t dev_addr, uint8_t instance)
{
    guncon2h_interface_t *gc_itf = get_instance(dev_addr, instance);
    uint8_t const ep_in = gc_itf->ep_in;
    return !usbh_edpt_busy(dev_addr, ep_in);
}

bool tuh_guncon2_receive_report(uint8_t dev_addr, uint8_t instance)
{
    guncon2h_interface_t *gc_itf = get_instance(dev_addr, instance);
    TU_VERIFY(usbh_edpt_claim(dev_addr, gc_itf->ep_in));

    if ( !usbh_edpt_xfer(dev_addr, gc_itf->ep_in, gc_itf->epin_buf, gc_itf->epin_size) )
    {
        usbh_edpt_claim(dev_addr, gc_itf->ep_in);
        return false;
    }
    return true;
}

bool tuh_guncon2_set_60hz(uint8_t dev_addr, uint8_t instance, bool state)
{
    return tuh_guncon2_send_report(dev_addr, instance, 5, state);
}

bool tuh_guncon2_send_report(uint8_t dev_addr, uint8_t instance, uint8_t index, bool state)
{
    uint8_t txbuf[6] = {
        0, //x offset
        0, //x offset (0xFF if negative)
        0, //y offset
        0, //y offset (0xFF if negative)
        0, //?
        0 // mode (1: 60hz)
    };
    uint16_t len = sizeof(txbuf);
    
    if (index >= len)
        return false;
    
    txbuf[index] = state;

    tusb_control_request_t const request = {
        .bmRequestType_bit = {
            .recipient = TUSB_REQ_RCPT_INTERFACE,//TUSB_REQ_RCPT_INTERFACE, TUSB_REQ_RCPT_OTHER
            .type      = TUSB_REQ_TYPE_CLASS,
            .direction = TUSB_DIR_OUT
        },
        .bRequest = HID_REQ_CONTROL_SET_REPORT, // ??? HID_REQ_CONTROL_SET_REPORT   = 0x09, ///< Set Report ???
        .wValue   = tu_htole16(TU_U16(2, 0)), //0x0200, tu_htole16(TU_U16(desc_type, index))
        .wIndex   = 0,
        .wLength  = len
    };
    tuh_xfer_t xfer = {
        .daddr       = dev_addr, // from mount callback
        .ep_addr     = 0, // control endpoint has address 0
        .setup       = &request,
        .buffer      = txbuf,
        //.complete_cb = complete_cb
    };

    return tuh_control_xfer(&xfer);
}

//--------------------------------------------------------------------+
// USBH API
//--------------------------------------------------------------------+

void guncon2h_init(void)
{
    tu_memclr(_guncon2h_dev, sizeof(_guncon2h_dev));
}

bool guncon2h_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX);

    uint16_t PID, VID;
    tuh_vid_pid_get(dev_addr, &VID, &PID);

    if (VID != 0x0B9A && PID != 0x016A)
    {
        TU_LOG2("GUNCON2: not a guncon device\n");
        return false;
    }

    TU_LOG2("GUNCON2 opening Interface %u (addr = %u)\r\n", desc_itf->bInterfaceNumber, dev_addr);

    guncon2h_device_t *guncon2_dev = get_dev(dev_addr);
    TU_ASSERT(guncon2_dev->inst_count < CFG_TUH_GUNCON2, 0);

    guncon2h_interface_t *gc_itf = get_instance(dev_addr, guncon2_dev->inst_count);
    gc_itf->itf_num = desc_itf->bInterfaceNumber;

    //Parse descriptor for all endpoints and open them
    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    int endpoint = 0;
    int pos = 0;
    while (endpoint < desc_itf->bNumEndpoints && pos < max_len)
    {
        if (tu_desc_type(p_desc) != TUSB_DESC_ENDPOINT)
        {
            pos += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
            continue;
        }
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
        TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
        if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT)
        {
            gc_itf->ep_out = desc_ep->bEndpointAddress;
            gc_itf->epout_size = tu_edpt_packet_size(desc_ep);
        }
        else
        {
            gc_itf->ep_in = desc_ep->bEndpointAddress;
            gc_itf->epin_size = tu_edpt_packet_size(desc_ep);
        }
        endpoint++;
        pos += tu_desc_len(p_desc);
        p_desc = tu_desc_next(p_desc);
    }

    guncon2_dev->inst_count++;
    return true;
}

bool guncon2h_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    uint8_t instance = get_instance_id_by_itfnum(dev_addr, itf_num);
    guncon2h_interface_t *gc_itf = get_instance(dev_addr, instance);
    gc_itf->connected = true;

    //set 60hz mode
    tuh_guncon2_set_60hz(dev_addr, instance, true);

    if (tuh_guncon2_mount_cb)
    {
        tuh_guncon2_mount_cb(dev_addr, instance, gc_itf);
    }

    usbh_driver_set_config_complete(dev_addr, gc_itf->itf_num);
    return true;
}

bool guncon2h_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (result != XFER_RESULT_SUCCESS)
    {
        TU_LOG1("Error: %d\n", result);
        return false;
    }

    uint8_t const dir = tu_edpt_dir(ep_addr);
    uint8_t const instance = get_instance_id_by_epaddr(dev_addr, ep_addr);
    guncon2h_interface_t *gc_itf = get_instance(dev_addr, instance);
    guncon2_gamepad_t *pad = &gc_itf->pad;
    uint8_t *rdata = gc_itf->epin_buf;

    if (dir == TUSB_DIR_IN)
    {
        TU_LOG2("Get Report callback (%u, %u, %u bytes)\r\n", dev_addr, instance, xferred_bytes);
        TU_LOG2_MEM(gc_itf->epin_buf, xferred_bytes, 2);
        if (xferred_bytes == 6) // data is valid
        {
            tu_memclr(pad, sizeof(guncon2_gamepad_t));

            pad->bButtons = ((~rdata[1] >> 2) & 0x38) | ((~rdata[0] >> 1) & 0x07);
            pad->bDpad    = (~rdata[0] >> 4) & 0xF;

            pad->wGunX = rdata[3];
            pad->wGunX <<= 8;
            pad->wGunX |= rdata[2];

            pad->wGunY = rdata[5];
            pad->wGunY <<= 8;
            pad->wGunY |= rdata[4];

            gc_itf->new_pad_data = true;
        }
        tuh_guncon2_report_received_cb(dev_addr, instance, (const uint8_t *)gc_itf, sizeof(guncon2h_interface_t));
        gc_itf->new_pad_data = false;
    }
    else
    {
        if (tuh_guncon2_report_sent_cb)
        {
            tuh_guncon2_report_sent_cb(dev_addr, instance, gc_itf->epout_buf, xferred_bytes);
        }
    }

    return true;
}

void guncon2h_close(uint8_t dev_addr)
{
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX, );
    guncon2h_device_t *guncon2_dev = get_dev(dev_addr);

    for (uint8_t inst = 0; inst < guncon2_dev->inst_count; inst++)
    {
        if (tuh_guncon2_umount_cb)
        {
            tuh_guncon2_umount_cb(dev_addr, inst);
        }
    }
    tu_memclr(guncon2_dev, sizeof(guncon2h_device_t));
}


#endif
