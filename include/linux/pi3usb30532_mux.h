#ifndef __PI3USB30532_MUX_H__
#define __PI3USB30532_MUX_H__

enum pi3usb30532_mux_conf {
	PI3USBMUX_OPEN_WITHPD,		/* switch open with power down */
	PI3USBMUX_OPEN_ONLYNOPD,	/* switch open only, no power down */
	PI3USBMUX_4LDP1P2,		/* 4 Lane of DP1.2 */
	PI3USBMUX_4LDP1P2_SWAP,		/* 4 Lane of DP1.2 swap */
	PI3USBMUX_USB30,		/* USB3.0 only */
	PI3USBMUX_USB30_SWAP,		/* USB3.0 swap */
	PI3USBMUX_USB30N2LDP1P2,	/* USB3.0 + 2 Lane of DP1.2 */
	PI3USBMUX_USB30N2LDP1P2_SWAP,	/* USB3.0 + 2 Lane of DP1.2 swap */
};

#endif /* __PI3USB30532_MUX_H__ */
