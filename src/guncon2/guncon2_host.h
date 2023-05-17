// Guncon2 tinyusb code by sonik-br
// https://github.com/sonik-br
//
// Based on documentation by beardypig
// https://github.com/beardypig/guncon2
//
// Tinyusb host based on xinput host by Ryzee119
// https://github.com/Ryzee119/tusb_xinput

#ifndef _TUSB_GUNCON2_HOST_H_
#define _TUSB_GUNCON2_HOST_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// Class Driver Configuration
//--------------------------------------------------------------------+

#ifndef CFG_TUH_GUNCON2_EPIN_BUFSIZE
#define CFG_TUH_GUNCON2_EPIN_BUFSIZE 64
#endif

#ifndef CFG_TUH_GUNCON2_EPOUT_BUFSIZE
#define CFG_TUH_GUNCON2_EPOUT_BUFSIZE 64
#endif

#define GUNCON2_GAMEPAD_DPAD_UP    0x01
#define GUNCON2_GAMEPAD_DPAD_DOWN  0x02
#define GUNCON2_GAMEPAD_DPAD_LEFT  0x04
#define GUNCON2_GAMEPAD_DPAD_RIGHT 0x08

#define GUNCON2_GAMEPAD_C       0x01
#define GUNCON2_GAMEPAD_B       0x02
#define GUNCON2_GAMEPAD_A       0x04
#define GUNCON2_GAMEPAD_TRIGGER 0x08
#define GUNCON2_GAMEPAD_SELECT  0x10
#define GUNCON2_GAMEPAD_START   0x20

#define MAX_PACKET_SIZE 32

typedef struct guncon2_gamepad
{
    uint8_t bButtons;
    uint8_t bDpad;
    uint16_t wGunX;
    uint16_t wGunY;
} guncon2_gamepad_t;

typedef struct
{
    guncon2_gamepad_t pad;
    uint8_t connected;
    uint8_t new_pad_data;
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    uint16_t epin_size;
    uint16_t epout_size;

    uint8_t epin_buf[CFG_TUH_GUNCON2_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUH_GUNCON2_EPOUT_BUFSIZE];
} guncon2h_interface_t;

//--------------------------------------------------------------------+
// Callbacks
//--------------------------------------------------------------------+

void tuh_guncon2_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
TU_ATTR_WEAK void tuh_guncon2_report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
TU_ATTR_WEAK void tuh_guncon2_umount_cb(uint8_t dev_addr, uint8_t instance);
TU_ATTR_WEAK void tuh_guncon2_mount_cb(uint8_t dev_addr, uint8_t instance, const guncon2h_interface_t *guncon2_itf);

//--------------------------------------------------------------------+
// Interface API
//--------------------------------------------------------------------+

bool tuh_guncon2_n_ready(uint8_t dev_addr, uint8_t instance);
bool tuh_guncon2_receive_report(uint8_t dev_addr, uint8_t instance);
bool tuh_guncon2_send_report(uint8_t dev_addr, uint8_t instance, uint8_t function, bool state);
bool tuh_guncon2_set_60hz(uint8_t dev_addr, uint8_t instance, bool state);

//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
void guncon2h_init       (void);
bool guncon2h_open       (uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);
bool guncon2h_set_config (uint8_t dev_addr, uint8_t itf_num);
bool guncon2h_xfer_cb    (uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
void guncon2h_close      (uint8_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_GUNCON2_HOST_H_ */
