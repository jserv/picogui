#ifndef PGSERVER_TOUCHSCREEN_H
#define PGSERVER_TOUCHSCREEN_H
#ifdef CONFIG_TOUCHSCREEN
/* Touchscreen helpers */

/* converts pen coordinates to physical; always call this, as it sends the
 * raw coordinates to tpcal when touchscreen_calibrated is unset */
void touchscreen_pentoscreen(int *x, int *y);
/* message function for the calibration codes */
void touchscreen_message(u32 message, u32 param, u32 *ret);
/* loads touchscreen calibration - CALL THIS! */
g_error touchscreen_init(void);
/* 1 if touchscreen has been calibrated
 * if 0, pgserver could run tpcal automatically */
extern u8 touchscreen_calibrated;
#endif
#endif
