// Steel Battalion Controller tinyusb code by sonik-br
// https://github.com/sonik-br
//
// Based on documentation from:
// https://xboxdevwiki.net/Xbox_Input_Devices#bType_.3D_128:_Steel_Battalion
// https://github.com/jcoutch/steel-battalion-net
//
// Tinyusb host based on xinput host by Ryzee119
// https://github.com/Ryzee119/tusb_xinput

#ifndef _TUSB_SBC_HOST_H_
#define _TUSB_SBC_HOST_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// Class Driver Configuration
//--------------------------------------------------------------------+

#ifndef CFG_TUH_SBC_EPIN_BUFSIZE
#define CFG_TUH_SBC_EPIN_BUFSIZE 64
#endif

#ifndef CFG_TUH_SBC_EPOUT_BUFSIZE
#define CFG_TUH_SBC_EPOUT_BUFSIZE 64
#endif

#define MAX_PACKET_SIZE 32

// to do: add button mask

typedef struct sbc_gamepad
{
    uint64_t bButtons;
    uint8_t bRotationLever;
    uint8_t bSightChangeX;
    uint8_t bSightChangeY;
    uint8_t bAimingX;
    uint8_t bAimingY;
    uint8_t bLeftPedal;
    uint8_t bMiddlePedal;
    uint8_t bRightPedal;
    uint8_t bTunerDial;
    uint8_t bGearLever;
} sbc_gamepad_t;

typedef struct sbc_leds
{
    uint8_t EmergencyEject : 4;
    uint8_t CockpitHatch : 4;
    uint8_t Ignition : 4;
    uint8_t Start : 4;
    uint8_t OpenClose : 4;
    uint8_t MapZoomInOut : 4;
    uint8_t ModeSelect : 4;
    uint8_t SubMonitorModeSelect : 4;
    uint8_t MainMonitorZoomIn : 4;
    uint8_t MainMonitorZoomOut : 4;
    uint8_t ForecastShootingSystem : 4;
    uint8_t Manipulator : 4;
    uint8_t LineColorChange : 4;
    uint8_t Washing : 4;
    uint8_t Extinguisher : 4;
    uint8_t Chaff : 4;
    uint8_t TankDetach : 4;
    uint8_t Override : 4;
    uint8_t NightScope : 4;
    uint8_t F1 : 4;
    uint8_t F2 : 4;
    uint8_t F3 : 4;
    uint8_t MainWeaponControl : 4;
    uint8_t SubWeaponControl : 4;
    uint8_t MagazineChange : 4;
    uint8_t Comm1 : 4;
    uint8_t Comm2 : 4;
    uint8_t Comm3 : 4;
    uint8_t Comm4 : 4;
    uint8_t Comm5 : 4;
    uint8_t UNUSED : 4;
    uint8_t GearR : 4;
    uint8_t GearN : 4;
    uint8_t Gear1 : 4;
    uint8_t Gear2 : 4;
    uint8_t Gear3 : 4;
    uint8_t Gear4 : 4;
    uint8_t Gear5 : 4;
} sbc_leds_t;

typedef struct
{
    sbc_gamepad_t pad;
    uint8_t connected;
    uint8_t new_pad_data;
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    uint16_t epin_size;
    uint16_t epout_size;

    uint8_t epin_buf[CFG_TUH_SBC_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUH_SBC_EPOUT_BUFSIZE];
} sbch_interface_t;

//--------------------------------------------------------------------+
// Callbacks
//--------------------------------------------------------------------+

void tuh_sbc_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
TU_ATTR_WEAK void tuh_sbc_report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
TU_ATTR_WEAK void tuh_sbc_umount_cb(uint8_t dev_addr, uint8_t instance);
TU_ATTR_WEAK void tuh_sbc_mount_cb(uint8_t dev_addr, uint8_t instance, const sbch_interface_t *sbc_itf);

//--------------------------------------------------------------------+
// Interface API
//--------------------------------------------------------------------+

bool tuh_sbc_receive_report(uint8_t dev_addr, uint8_t instance);
bool tuh_sbc_send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *txbuf, uint16_t len);
bool tuh_sbc_set_leds(uint8_t dev_addr, uint8_t instance, const sbc_leds_t *value);

//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
void sbch_init       (void);
bool sbch_open       (uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);
bool sbch_set_config (uint8_t dev_addr, uint8_t itf_num);
bool sbch_xfer_cb    (uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
void sbch_close      (uint8_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_SBC_HOST_H_ */
