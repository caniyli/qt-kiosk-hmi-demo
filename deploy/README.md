# Starting the HMI at boot

Notes for running `hmi-demo` as the only application on a panel PC or an ARM
board with a touch screen. **These are examples. They have not been tested on
real hardware**; paths, users and Qt settings depend on the device image.

The application side is the same in every setup:

```sh
hmi-demo --kiosk --hide-cursor
```

`--kiosk` opens a frameless full-screen window, `--hide-cursor` hides the
mouse pointer for touch use. In kiosk mode the Ctrl+Q shortcut is disabled.

## Option A: eglfs, no display server (example)

With Qt's `eglfs` platform the application draws directly to the display via
EGL/KMS, so no X11 or Wayland session is needed. This is the usual choice on
embedded boards whose Qt build includes an eglfs backend (for example
`eglfs_kms` on boards with a DRM/KMS GPU driver).

1. Install the binary and create a service user that may use the GPU and
   input devices (the `render` group does not exist on every distribution):

   ```sh
   sudo install -m 755 build/hmi-demo /usr/local/bin/
   sudo useradd --system --create-home --groups video,input,render hmi
   ```

2. Install and enable the unit from this directory:

   ```sh
   sudo cp deploy/hmi-demo.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable --now hmi-demo
   journalctl -u hmi-demo -f      # logs
   ```

`Restart=always` brings the HMI back if it exits or crashes. Device-specific
settings (KMS output configuration, physical screen size for correct DPI,
touch rotation) go into `Environment=` lines of the unit; see the commented
examples there and the Qt "Embedded Linux" documentation for your Qt version.

## Option B: X11 with automatic login (example)

If the image already has X11, start a bare X server with only the HMI, for
example from the auto-login user's `~/.xinitrc`:

```sh
#!/bin/sh
xset s off          # no screen saver
xset -dpms          # no display power management
xset s noblank
exec /usr/local/bin/hmi-demo --kiosk --hide-cursor
```

and run `startx` from the auto-login shell profile (or a display manager
configured for auto-login with this session). A window manager is not
required for a single full-screen window. Use `QT_QPA_PLATFORM=xcb` if the
platform is not detected automatically.

## Checklist for a real device

- Hardware stop and safety functions must not depend on the HMI. Start/Stop
  on screen are operator commands, not safety functions.
- Decide what happens on power loss: settings are written with QSettings to
  `~/.config/qt-kiosk-hmi-demo/` of the service user; on a read-only root file
  system that directory must be on a writable partition.
- Disable console blanking and kernel messages on the display (kernel command
  line, for example `consoleblank=0 quiet`).
- Calibrate or rotate the touch screen at the input layer (udev / libinput),
  not inside the application.
