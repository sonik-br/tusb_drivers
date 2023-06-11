#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_SBC)

#include "host/usbh.h"
#include "host/usbh_classdriver.h"
#include "sbc_host.h"

typedef struct
{
    uint8_t inst_count;
    sbch_interface_t instances[CFG_TUH_SBC];
} sbch_device_t;

static sbch_device_t _sbch_dev[CFG_TUH_DEVICE_MAX];

TU_ATTR_ALWAYS_INLINE static inline sbch_device_t *get_dev(uint8_t dev_addr)
{
    return &_sbch_dev[dev_addr - 1];
}

TU_ATTR_ALWAYS_INLINE static inline sbch_interface_t *get_instance(uint8_t dev_addr, uint8_t instance)
{
    return &_sbch_dev[dev_addr - 1].instances[instance];
}

static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr)
{
    for (uint8_t inst = 0; inst < CFG_TUH_SBC; inst++)
    {
        sbch_interface_t *hid = get_instance(dev_addr, inst);

        if ((ep_addr == hid->ep_in) || (ep_addr == hid->ep_out))
            return inst;
    }

    return 0xff;
}

static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf)
{
    for (uint8_t inst = 0; inst < CFG_TUH_SBC; inst++)
    {
        sbch_interface_t *hid = get_instance(dev_addr, inst);

        if ((hid->itf_num == itf) && (hid->ep_in || hid->ep_out))
            return inst;
    }

    return 0xff;
}

bool tuh_sbc_receive_report(uint8_t dev_addr, uint8_t instance)
{
    sbch_interface_t *sbc_itf = get_instance(dev_addr, instance);
    TU_VERIFY(usbh_edpt_claim(dev_addr, sbc_itf->ep_in));

    if ( !usbh_edpt_xfer(dev_addr, sbc_itf->ep_in, sbc_itf->epin_buf, sbc_itf->epin_size) )
    {
        usbh_edpt_claim(dev_addr, sbc_itf->ep_in);
        return false;
    }
    return true;
}

bool tuh_sbc_set_leds(uint8_t dev_addr, uint8_t instance, const sbc_leds_t *value)
{
    // uint8_t txbuf[22] = {
    //     0x00, //Start
    //     0x16, //bLen (22 bytes)
    //     value, //0x0F EmergencyEject | 0xF0 CockpitHatch
    //     value, //0x0F Ignition | 0xF0 Start
    //     value, //0x0F OpenClose | 0xF0 MapZoomInOut
    //     value, //0x0F ModeSelect | 0xF0 SubMonitorModeSelect
    //     value, //0x0F MainMonitorZoomIn | 0xF0 MainMonitorZoomOut
    //     value, //0x0F ForecastShootingSystem | 0xF0 Manipulator
    //     value, //0x0F LineColorChange | 0xF0 Washing
    //     value, //0x0F Extinguisher | 0xF0 Chaff
    //     value, //0x0F TankDetach | 0xF0 Override
    //     value, //0x0F NightScope | 0xF0 F1
    //     value, //0x0F F2 | 0xF0 F3
    //     value, //0x0F MainWeaponControl | 0xF0 SubWeaponControl
    //     value, //0x0F MagazineChange | 0xF0 Comm1
    //     value, //0x0F Comm2 | 0xF0 Comm3
    //     value, //0x0F Comm4 | 0xF0 Comm5
    //     value ? 0x70 : 0x00, //0x0F NOTHING | 0xF0 GearR
    //     value, //0x0F GearN | 0xF0 Gear1
    //     value, //0x0F Gear2 | 0xF0 Gear3
    //     value, //0x0F Gear4 | 0xF0 Gear5
    //     0x00, //Unused?
    // };
    
    uint8_t txbuf[22];
    txbuf[0] = 0x00;  //Start
    txbuf[1] = 0x16;  //bLen (22 bytes)
    txbuf[21] = 0x00; //Unused?

    memcpy(txbuf + 2, value, sizeof(sbc_leds_t));

    uint16_t len = sizeof(txbuf);

    return tuh_sbc_send_report(dev_addr, instance, txbuf, len);
}

bool tuh_sbc_send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *txbuf, uint16_t len)
{
    sbch_interface_t *sbc_itf = get_instance(dev_addr, instance);

    TU_ASSERT(len <= sbc_itf->epout_size);
    TU_VERIFY(usbh_edpt_claim(dev_addr, sbc_itf->ep_out));

    memcpy(sbc_itf->epout_buf, txbuf, len);
    return usbh_edpt_xfer(dev_addr, sbc_itf->ep_out, sbc_itf->epout_buf, len);
}

//--------------------------------------------------------------------+
// USBH API
//--------------------------------------------------------------------+
void sbch_init(void)
{
    tu_memclr(_sbch_dev, sizeof(_sbch_dev));
}

bool sbch_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX);

    uint16_t PID, VID;
    tuh_vid_pid_get(dev_addr, &VID, &PID);

    if (VID != 0x0A7B && PID != 0xD000 &&
        desc_itf->bInterfaceClass != 0x58 &&  //XboxOG bInterfaceClass
        desc_itf->bInterfaceSubClass != 0x42) //XboxOG bInterfaceSubClass
    {
        TU_LOG2("SBC: not a known device\n");
        return false;
    }

    TU_LOG2("SBC opening Interface %u (addr = %u)\r\n", desc_itf->bInterfaceNumber, dev_addr);

    sbch_device_t *sbc_dev = get_dev(dev_addr);
    TU_ASSERT(sbc_dev->inst_count < CFG_TUH_SBC, 0);

    sbch_interface_t *sbc_itf = get_instance(dev_addr, sbc_dev->inst_count);
    sbc_itf->itf_num = desc_itf->bInterfaceNumber;

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
            sbc_itf->ep_out = desc_ep->bEndpointAddress;
            sbc_itf->epout_size = tu_edpt_packet_size(desc_ep);
        }
        else
        {
            sbc_itf->ep_in = desc_ep->bEndpointAddress;
            sbc_itf->epin_size = tu_edpt_packet_size(desc_ep);
        }
        endpoint++;
        pos += tu_desc_len(p_desc);
        p_desc = tu_desc_next(p_desc);
    }

    sbc_dev->inst_count++;
    return true;
}

bool sbch_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    uint8_t instance = get_instance_id_by_itfnum(dev_addr, itf_num);
    sbch_interface_t *sbc_itf = get_instance(dev_addr, instance);
    sbc_itf->connected = true;

    if (tuh_sbc_mount_cb)
    {
        tuh_sbc_mount_cb(dev_addr, instance, sbc_itf);
    }

    usbh_driver_set_config_complete(dev_addr, sbc_itf->itf_num);
    return true;
}

bool sbch_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (result != XFER_RESULT_SUCCESS)
    {
        TU_LOG1("Error: %d\n", result);
        return false;
    }

    uint8_t const dir = tu_edpt_dir(ep_addr);
    uint8_t const instance = get_instance_id_by_epaddr(dev_addr, ep_addr);
    sbch_interface_t *sbc_itf = get_instance(dev_addr, instance);
    sbc_gamepad_t *pad = &sbc_itf->pad;
    uint8_t *rdata = sbc_itf->epin_buf;

    if (dir == TUSB_DIR_IN)
    {
        TU_LOG2("Get Report callback (%u, %u, %u bytes)\r\n", dev_addr, instance, xferred_bytes);
        TU_LOG2_MEM(sbc_itf->epin_buf, xferred_bytes, 2);


        if (xferred_bytes == 26 && (rdata[6] & 0x80) == 0x80 && rdata[7] == 0x00 && (rdata[24] & 0xF0) == 0x00)
        {
            tu_memclr(pad, sizeof(sbc_gamepad_t));

            pad->bButtons       = (uint64_t)(rdata[6] & 0x7F) << 32 | (uint64_t)rdata[5] << 24 | rdata[4] << 16 | rdata[3] << 8 | rdata[2];
            pad->bAimingX       = rdata[9];
            pad->bAimingY       = rdata[11];
            pad->bRotationLever = rdata[13];
            pad->bSightChangeX  = rdata[15];
            pad->bSightChangeY  = rdata[17];
            pad->bLeftPedal     = rdata[19];
            pad->bMiddlePedal   = rdata[21];
            pad->bRightPedal    = rdata[23];
            pad->bTunerDial     = rdata[24] & 0x0F;
            pad->bGearLever     = rdata[25];

            sbc_itf->new_pad_data = true;
        }

        tuh_sbc_report_received_cb(dev_addr, instance, (const uint8_t *)sbc_itf, sizeof(sbch_interface_t));
        sbc_itf->new_pad_data = false;
    }
    else
    {
        if (tuh_sbc_report_sent_cb)
        {
            tuh_sbc_report_sent_cb(dev_addr, instance, sbc_itf->epout_buf, xferred_bytes);
        }
    }

    return true;
}

void sbch_close(uint8_t dev_addr)
{
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX, );
    sbch_device_t *sbc_dev = get_dev(dev_addr);

    for (uint8_t inst = 0; inst < sbc_dev->inst_count; inst++)
    {
        if (tuh_sbc_umount_cb)
        {
            tuh_sbc_umount_cb(dev_addr, inst);
        }
    }
    tu_memclr(sbc_dev, sizeof(sbch_device_t));
}

#endif
