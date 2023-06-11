# tusb_drivers
Collection of custom host drivers for TinyUSB

I have not found a better way to implement access to non-standard class devices.

Got inspired by how [tusb_xinput](https://github.com/Ryzee119/tusb_xinput) works and I did the same.
This requires modification to tinyusb library.

If you know a way to access "vendor" class devices without modifying tusb please let me know.

## Drivers

### Densha De Go
Supports the PS2 "Type 2" device. More devices can be added but I don't have any.

### GunCon2
Supports the PS2 GunCon 2 device.

### Steel Battalion Controller (SBC)
Supports the Xbox Steel Battalion Controller device.

## Using
Copy the src files to your installed tusb lib.
I like to use put the files on the `class` folder.


Add to `tusb_config.h`
```
#define CFG_TUH_DENSHA 1
#define CFG_TUH_GUNCON2 1
#define CFG_TUH_SBC 1
```

Add to `usbh.c`:
```
  #if CFG_TUH_DENSHA
    {
      DRIVER_NAME("DENSHADEGO")
      .init       = denshah_init,
      .open       = denshah_open,
      .set_config = denshah_set_config,
      .xfer_cb    = denshah_xfer_cb,
      .close      = denshah_close
    },
  #endif

  #if CFG_TUH_GUNCON2
    {
      DRIVER_NAME("GUNCON2")
      .init       = guncon2h_init,
      .open       = guncon2h_open,
      .set_config = guncon2h_set_config,
      .xfer_cb    = guncon2h_xfer_cb,
      .close      = guncon2h_close
    },
  #endif

  #if CFG_TUH_SBC
    {
      DRIVER_NAME("SBC")
      .init       = sbch_init,
      .open       = sbch_open,
      .set_config = sbch_set_config,
      .xfer_cb    = sbch_xfer_cb,
      .close      = sbch_close
    },
  #endif
```

Add to `tusb.h` (and change the paths if needed)
```
  #if CFG_TUH_DENSHA
    #include "class/densha/densha_host.h"
  #endif

  #if CFG_TUH_GUNCON2
    #include "class/guncon2/guncon2_host.h"
  #endif

  #if CFG_TUH_SBC
    #include "class/sbc/sbc_host.h"
  #endif
```

Check the callback functions on the source file and implement them.

## Credits
Host driver based on [tusb_xinput](https://github.com/Ryzee119/tusb_xinput) by Ryzee119

Densha De Go [controller docs](https://marcriera.github.io/ddgo-controller-docs) from by marcriera

Guncon2 linux [driver](https://github.com/beardypig/guncon2) by beardypig

SBC [docs](https://xboxdevwiki.net/Xbox_Input_Devices#bType_.3D_128:_Steel_Battalion) by xboxdevwiki<br/>
SBC libusb interface [steel-battalion-net](https://github.com/jcoutch/steel-battalion-net) by jcoutch