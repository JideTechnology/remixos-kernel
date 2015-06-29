/*
 * Copyright (C) 2013 Intel Mobile Communications GmbH
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _IUI_FM_8000_H
#define _IUI_FM_8000_H

/**
 * @file iui_fm.h
 * @brief The IUI Frequency Manager interface exposes an RPC-compatible
 *        interface to pass frequency information between macros on the AP and
 *        the Frequency Manager component on the MEX.
 *
 * @addtogroup ISPEC_FREQ_MANAGER Frequency manager
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Choose whether to use integer types defined in the Linux Kernel (for kernel
   builds) or C Library (for Userspace/MEX builds). */
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#define INT32_MAX INT_MAX
#else /* __KERNEL__ */
#include <stdint.h>
#endif /* __KERNEL__ */

/**
 * Zero-based enum of Frequency Manager Macro ID's.
 * Identifies a Frequency Manager Macro in the IuiFm API's.
 *
 * @IUI_FM_MACRO_ID_INVALID:  Invalid Macro ID
 * @IUI_FM_MACRO_ID_EMMC:     SD/eMMC Macro
 * @IUI_FM_MACRO_ID_CLASS_D:  Class D Amplifier Macro
 * @IUI_FM_MACRO_ID_PMU_CP:   PMU Charge Pump Macro
 * @IUI_FM_MACRO_ID_MS_CP:    MS Charge Pump Macro
 * @IUI_FM_MACRO_ID_DCDC:     DC to DC Converter Macro
 * @IUI_FM_MACRO_ID_IDI:      Inter-Die Interface Macro
 * @IUI_FM_MACRO_ID_WLAN:     Wireless LAN Macro
 * @IUI_FM_MACRO_ID_FMR:      FM Radio Macro
 * @IUI_FM_MACRO_ID_BT:       Bluetooth Macro
 * @IUI_FM_MACRO_ID_GNSS:     Global Navigation Satellite System
 *                            (GPS/GLONASS) Macro
 * @IUI_FM_MACRO_ID_MAX:      Number of Macro's defined, used
 *                            internally for range checking
 * @IUI_FM_MACRO_ID_FIRST:    Point to the first macro ID, used internally
 *                            for iteration and range checking
 */
enum iui_fm_macro_id {
	IUI_FM_MACRO_ID_INVALID = -1,
	IUI_FM_MACRO_ID_EMMC = 0,
	IUI_FM_MACRO_ID_CLASS_D = 1,
	IUI_FM_MACRO_ID_PMU_CP = 2,
	IUI_FM_MACRO_ID_MS_CP = 3,
	IUI_FM_MACRO_ID_DCDC = 4,
	IUI_FM_MACRO_ID_IDI = 5,
	IUI_FM_MACRO_ID_WLAN = 6,
	IUI_FM_MACRO_ID_FMR = 7,
	IUI_FM_MACRO_ID_BT = 8,
	IUI_FM_MACRO_ID_GNSS = 9,
	IUI_FM_MACRO_ID_MAX = 10,
	IUI_FM_MACRO_ID_FIRST = IUI_FM_MACRO_ID_EMMC
};

/**
 * Enum used to indicate the priority of a mitigation request.
 *
 * @IUI_FM_MITIGATION_PRIORITY_NORMAL:    The priority value of a mitigation
 *                                        request when not in an emergency call
 *                                        or situation.
 * @IUI_FM_MITIGATION_PRIORITY_EMERGENCY: Can be used during an emergency call
 *                                        to indicate that the mitigation
 *                                        request should be applied with more
 *                                        haste than usual, even if this causes
 *                                        a degredation in currently-running
 *                                        processes.
 */
enum iui_fm_mitigation_priority {
  IUI_FM_MITIGATION_PRIORITY_NORMAL    = 0,
  IUI_FM_MITIGATION_PRIORITY_EMERGENCY = 1
};

/**
 * Enum used to identify an SD/eMMC device.
 *
 * @IUI_FM_EMMC_DEVICE_ID_0:    SD/eMMC Device 0
 * @IUI_FM_EMMC_DEVICE_ID_1:    SD/eMMC Device 1
 * @IUI_FM_EMMC_DEVICE_ID_2:    SD/eMMC Device 2
 * @IUI_FM_EMMC_DEVICE_ID_MAX:  Number of SD/eMMC Devices Defined, used
 *                              internally for range checking and array sizing
 */
enum iui_fm_emmc_device_id {
	IUI_FM_EMMC_DEVICE_ID_0   = 0,
	IUI_FM_EMMC_DEVICE_ID_1   = 1,
	IUI_FM_EMMC_DEVICE_ID_2   = 2,
	IUI_FM_EMMC_DEVICE_ID_MAX = 3
};

/**
 * Enum used to indicate from which main clock the operating frequency
 *        of an SD/eMMC device is derived.
 *
 * Possible SD/eMMC operating frequencies are derived by dividing the selected
 * main clock frequency by powers of 2.  The largest possible divisor is that
 * which results in the lowest operating frequency greater than 200 KHz.  Hence
 * the possible operating frequencies are:
 *
 * Clock Source: 104 MHz:
 *   2^0: 104 MHz
 *   2^1: 52 MHz
 *   2^2: 26 MHz
 *   2^3: 13 MHz
 *   2^4: 6.5 MHz
 *   2^5: 3.25 MHz
 *   2^6: 1.625 MHz
 *   2^7: 812.5 KHz
 *   2^8: 406.25 KHz
 *   2^9: 203.125 KHz
 *
 * Clock Source: 96 MHz:
 *   2^0: 96 MHz
 *   2^1: 48 MHz
 *   2^2: 24 MHz
 *   2^3: 12 MHz
 *   2^4: 6 MHz
 *   2^5: 3 MHz
 *   2^6: 1.5 MHz
 *   2^7: 750 KHz
 *   2^8: 375 KHz
 *
 * @IUI_FM_EMMC_CLOCK_SRC_104MHZ  104 MHz main clock source for SD/eMMC.
 * @IUI_FM_EMMC_CLOCK_SRC_96MHZ   96 MHz main clock source for SD/eMMC.
 */
enum iui_fm_emmc_clock_src {
	IUI_FM_EMMC_CLOCK_SRC_104MHZ = 0,
	IUI_FM_EMMC_CLOCK_SRC_96MHZ  = 1
};

/**
 * @brief Bitfield structure of 32-bit width to store the operating frequency
 *        information for one SD/eMMC device.
 *
 * @frequency  SD/eMMC Operating Frequency in KHz.
 * @clk_src    SD/eMMC Clock Source. Values in enum iui_fm_emmc_clock_src.
 */
struct iui_fm_emmc_freq_bitfield {
	uint32_t frequency :28;
	uint32_t clk_src   :4;
};

/**
 * @brief Contains operating frequency information for all of the SD/eMMC
 *        devices.
 *
 * @emmc_frequency  Array of frequency information for each EMMC device.
 */
struct iui_fm_emmc_freq_info {
	struct iui_fm_emmc_freq_bitfield emmc_frequency[IUI_FM_EMMC_DEVICE_ID_MAX];
};

/**
 * @brief Enum representing the possible channel bandwidths used by the WLAN
 *        macro.
 *
 * @IUI_FM_WLAN_BW_INVALID  Used if WLAN channel is in disconnected state.
 * @IUI_FM_WLAN_BW_20MHZ    20 MHz bandwidth.
 * @IUI_FM_WLAN_BW_40MHZ    40 MHz bandwidth (NOT SUPPORTED).
 * @IUI_FM_WLAN_BW_80MHZ    80 MHz bandwidth (NOT SUPPORTED).
 */
enum iui_fm_wlan_bandwidth {
	IUI_FM_WLAN_BW_INVALID = -1,
	IUI_FM_WLAN_BW_20MHZ   = 0,
	IUI_FM_WLAN_BW_40MHZ   = 1,
	IUI_FM_WLAN_BW_80MHZ   = 2,
	IUI_FM_WLAN_BW_160MHZ   = 3
};

/**
 * Structure used to store the frequency and bandwidth of a WLAN channel.
 *
 * @frequency  Current WLAN channel center frequency in KHz if in
 *             transfer/receive mode, or an invalid value otherwise.
 *             Available values: 2412000-2484000, 3657500-3690000,
 *             4915000-5825000 (KHz).
 * @bandwadth  Channel bandwidth enum. Set to IUI_FM_WLAN_BW_INVALID if WLAN
 *             channel is disconnected.
 */
struct iui_fm_wlan_channel_info {
	uint32_t frequency;
	enum iui_fm_wlan_bandwidth bandwidth;
};

/**
  * Maximum number of channels that can be used by the WLAN macro at a time.
  */
#define IUI_FM_WLAN_MAX_CHANNELS 4

/**
 * @brief Structure used for Frequency Notification by the WLAN macro.
 *
 * @num_channels  Number of valid elements in the channel_info array, set to
 *                the number of channels currently in use by the WLAN macro, or
 *                0 if WLAN is not in transfer/receive mode.
 * @channel_info  Array of WLAN channel information structures. Only the first
 *                num_channels elements are valid.
 * @wlan_adc_dac_freq   Current WLAN ADC/DAC frequency in KHz. Available
 *                      values: 0 (parameter not yet supported).
 */
struct iui_fm_wlan_info {
	uint32_t num_channels;
	struct iui_fm_wlan_channel_info channel_info[IUI_FM_WLAN_MAX_CHANNELS];
	uint32_t wlan_adc_dac_freq;
};

/**
 * Special value for the max_tx_pwr field of struct
 * iui_fm_wlan_channel_tx_power to clear a previously-set TX power limit.
 */
#define IUI_FM_WLAN_NO_TX_PWR_LIMIT INT32_MAX

/**
 * @brief Structure to store the maximum allowed transmit power for one WLAN
 *        channel.
 *
 * @frequency   WLAN Channel Frequency in KHz.
 * @max_tx_pwr  Maximum allowed transmit power for the given channel frequency
 *              in dBm within the range 0 to +30, or
 *              IUI_FM_WLAN_NO_TX_PWR_LIMIT to cancel a previously-set transmit
 *              power limit.
 */
struct iui_fm_wlan_channel_tx_power {
	uint32_t frequency;
	int32_t max_tx_pwr;
};

/**
 * @brief Enum defining mitigation actions for WLAN macro to implement for the
 *        WLAN RX Gain Stage behavior.
 *
 * @IUI_FM_WLAN_RX_GAIN_NORMAL            RX Gain Stage should behave as
 *                                        normal with no reduction in gain.
 * @IUI_FM_WLAN_RX_GAIN_REDUCE_AUTO       RX Gain should be reduced by an
 *                                        unspecified amount. The WLAN macro
 *                                        is to determine the appropriate gain
 *                                        reduction amount internally.
 * @IUI_FM_WLAN_RX_GAIN_REDUCE_SPECIFIED  RX Gain should be reduced by the
 *                                        specified amount (NOT SUPPORTED).
 */
enum iui_fm_wlan_rx_gain_behavior {
	IUI_FM_WLAN_RX_GAIN_NORMAL      = 0,
	IUI_FM_WLAN_RX_GAIN_REDUCE_AUTO = 1
/*	IUI_FM_WLAN_RX_GAIN_REDUCE_SPECIFIED = 2 */
};

/**
 * @brief Mitigation information structure for the WLAN macro.
 *
 * @num_channels       Number of valid elements in the channel_tx_pwr array,
 *                     set to the number of channels for which the maximum
 *                     allowed transmit power is being set.
 * @channel_tx_pwr     Array of maximum allowed transmit power for zero or more
 *                     WLAN channels. Only the first num_channels elements are
 *                     valid.
 * @wlan_adc_dac_freq  Requested WLAN ADC/DAC frequency in KHz. Available
 *                     values: 0 (parameter not yet supported).
 * @rx_gain_behavior   Indicates the requested behavior of the WLAN RX Gain
 *                     Stage (NOT SUPPORTED).
 * @rx_gain_reduction  Amount by which to reduce the WLAN RX Gain (in dB) if
 *                     the rx_gain_bahavior field is set to
 *                     IUI_FM_WLAN_RX_GAIN_REDUCE_SPECIFIED (NOT SUPPORTED).
 */
struct iui_fm_wlan_mitigation {
	uint32_t num_channels;
	struct iui_fm_wlan_channel_tx_power
		channel_tx_pwr[IUI_FM_WLAN_MAX_CHANNELS];
	uint32_t wlan_adc_dac_freq;
	enum iui_fm_wlan_rx_gain_behavior rx_gain_behavior;
	uint32_t wlan_2g_coex_enable;
/*	uint32_t rx_gain_reduction; */
};

/**
 * @brief Enum representing the injection side indicated by the FM Radio
 *        macro.
 *
 * @IUI_FM_FMR_INJECTION_SIDE_LOW   FM Radio Low Side Injection.
 * @IUI_FM_FMR_INJECTION_SIDE_HIGH  FM Radio High Side Injection.
 */
enum iui_fm_fmr_injection_side {
	IUI_FM_FMR_INJECTION_SIDE_LOW = 0,
	IUI_FM_FMR_INJECTION_SIDE_HIGH = 1
};

/**
 * @brief Structure containing the operating information for the FM Radio
 *        macro.
 *
 * @rx_freq   Tuned frequency in KHz, or 0 if FM Radio is not in use.
 * @inj_side  Injection Side currently in use.
 */
struct iui_fm_fmr_info {
	uint32_t rx_freq;
	enum iui_fm_fmr_injection_side inj_side;
};

/**
 * @brief Indicates the status of the Bluetooth module.
 *
 * @IUI_FM_BT_STATE_OFF  Bluetooth module is turned off.
 * @IUI_FM_BT_STATE_ON   Bluetooth module is turned on.
 */
enum iui_fm_bt_state {
	IUI_FM_BT_STATE_OFF = 0,  /**< Bluetooth module is turned off. */
	IUI_FM_BT_STATE_ON  = 1   /**< Bluetooth module is turned on. */
};

/**
 * @brief Structure containing the operating information for the Bluetooth
 *        macro.
 *
 * Passed by the Bluetooth macro to Frequency Manager as a Frequency
 * Notification.
 *
 * @bt_state  Current state of the Bluetooth macro.
 */
struct iui_fm_bt_info {
	enum iui_fm_bt_state bt_state;
};

/**
 * @brief Define the number of Bluetooth hopping channels available.
 */
#define IUI_FM_BT_NUM_CHANNELS 79

/**
 * @brief Number of bits in one word of the Bluetooth Channel Mask.
 */
#define IUI_FM_BT_CHANNEL_MASK_WORD_BITS 32

/**
 * @brief Number of words needed to contain the full Bluetooth Channel
 *        Bitmask.
 *
 * For 79 hopping channels, 3 32-bit words are needed.
 */
#define IUI_FM_BT_CHANNEL_MASK_WORDS 3

/**
 * @brief Bitfield structure representing a hopping channel mask for the
 *        Bluetooth Macro.
 *
 * A set bit indicates a Bluetooth hopping frequency that should be avoided in
 * order to mitigate crosstalk with another macro.
 *
 * @bt_ch_mask  Array of words containing the Bluetooth Channel Mask. Each bit
 *              represents one Bluetooth hopping frequency, with the least of
 *              the first word representing the lowest frequency Bluetooth
 *              Hopping Frequency. If the number of hopping frequencies
 *              available is not a multiple of the word width (in bits), then
 *              the remaining unused bits in the last word are ignored. If a
 *              bit is set, the Bluetooth Hopping Frequency corresponding to
 *              that bit should be avoided by the Bluetooth Macro in order to
 *              avoid crosstalk with another macro.
 */
struct iui_fm_bt_channel_mask {
	uint32_t bt_ch_mask[IUI_FM_BT_CHANNEL_MASK_WORDS];
};

/**
 * @brief GNSS Macro State enum indicating which (if any) of the GNSS
 *        technologies are being used.
 *
 * @IUI_FM_GNSS_STATE_OFF                 GNSS macro is switched off.
 * @IUI_FM_GNSS_STATE_GPS_ACTIVE          GNSS macro is on and in GPS mode.
 * @IUI_FM_GNSS_STATE_GLONASS_ACTIVE      GNSS macro is on and in GLONASS
 *                                        mode.
 * @IUI_FM_GNSS_STATE_GPS_GLONASS_ACTIVE  GNSS macro is on and in GPS+GLONASS
 *                                        mode.
 */
enum iui_fm_gnss_state {
	IUI_FM_GNSS_STATE_OFF                = 0,
	IUI_FM_GNSS_STATE_GPS_ACTIVE         = 1,
	IUI_FM_GNSS_STATE_GLONASS_ACTIVE     = 2,
	IUI_FM_GNSS_STATE_GPS_GLONASS_ACTIVE = 3
};

/**
 * @brief GNSS Frequency Information structure passed from GNSS macro to the FM
 *        module during a Frequency Indication.
 *
 * @state          State of the GNSS module.
 * @snr            Current RX signal to noise ratio for the active GNSS
 *                 technology (GPS or GLONASS), or the stronger of the two if
 *                 both GPS and GLONASS are active. Valid range: 0..55 dB, or
 *                 -1 if not available.
 * @bus_frequency  Bus Frequency used by the GNSS module in KHz, or 0 if GNSS
 *                 is inactive/off.  Available frequencies:
 *                   78000 KHz (currently not supported)
 *                   83200 KHz
 *                   96000 KHz
 * @pll_frequency  Current operating frequency of the GNSS PLL in KHz, or 0
 *                 if the GNSS PLL is disabled.  Available frequencies:
 *                   124800 KHz
 *                   104000 KHz
 *                   96000 KHz
 *                   89000 KHz(currently not supported)
 * @adc_frequency  Current operating frequency of the GNSS ADC output clock
 *                 in KHz, or 0 if the GNSS ADC is disabled.  Available
 *                 frequencies:
 *                   44131 KHz
 *                   52957 KHz
 *                   66196 KHz
 */
struct iui_fm_gnss_info {
	enum iui_fm_gnss_state state;
	int32_t snr;
	uint32_t bus_frequency;
	uint32_t pll_frequency;
	uint32_t adc_frequency;
};

/**
 * @brief Mitigation information structure for the GNSS macro.
 *
 * @priority       Indicates whether the ongoing processing of a previous
 *                 mitigation request should be interrupted to fulfil this new
 *                 request. Based on whether the phone is currently in an
 *                 emergency call or other situation.
 * @bus_frequency  Requested GNSS bus clock frequency in KHz, or 0 if the
 *                 GNSS bus is disabled. Available frequencies:
 *                   78000 KHz (currently not supported)
 *                   83200 KHz
 *                   96000 KHz
 * @pll_frequency  Requested operating frequency of the GNSS PLL in KHz, or 0
 *                 if the GNSS PLL is disabled.  Available frequencies:
 *                   124800 KHz
 *                   104000 KHz
 *                   96000 KHz
 *                   89000 KHz(currently not supported)
 * @adc_frequency  Requested operating frequency of the GNSS ADC output clock
 *                 in KHz, or 0 if the GNSS ADC is disabled.  Available
 *                 frequencies:
 *                   44131 KHz
 *                   52957 KHz
 *                   66196 KHz
 */
struct iui_fm_gnss_mitigation {
  enum iui_fm_mitigation_priority priority;
	uint32_t bus_frequency;
	uint32_t pll_frequency;
	uint32_t adc_frequency;
};

/**
 * @brief Indicates which field of iui_fm_freq_notification_union to use.
 *
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_EMMC   EMMC Frequency Notification.
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_KHZ    Operating Frequency Notification
 *                                       (KHz).
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_WLAN   WLAN State/Frequency Notification.
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_FMR    FM Radio Frequency Notification.
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_BT     Bluetooth Info Notification.
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_GNSS   GNSS State/Frequency Notification.
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_MAX    Number of Frequency Notification
 *                                       Types defined, used internally for
 *                                       range checking.
 * @IUI_FM_FREQ_NOTIFICATION_TYPE_FIRST  First Frequency Notification Type
 *                                       defined, used internally for range
 *                                       checking.
 */
enum iui_fm_freq_notification_type {
	IUI_FM_FREQ_NOTIFICATION_TYPE_EMMC = 0,
	IUI_FM_FREQ_NOTIFICATION_TYPE_KHZ  = 1,
	IUI_FM_FREQ_NOTIFICATION_TYPE_WLAN = 2,
	IUI_FM_FREQ_NOTIFICATION_TYPE_FMR  = 3,
	IUI_FM_FREQ_NOTIFICATION_TYPE_BT   = 4,
	IUI_FM_FREQ_NOTIFICATION_TYPE_GNSS = 5,
	IUI_FM_FREQ_NOTIFICATION_TYPE_MAX  = 6,
	IUI_FM_FREQ_NOTIFICATION_TYPE_FIRST = IUI_FM_FREQ_NOTIFICATION_TYPE_EMMC
};

/**
 * @brief Union of possible Frequency Notification data types.
 *
 * @emmc_info  Pointer to EMMC Macro Frequency Information structure (type ==
 *             IUI_FM_FREQ_NOTIFICATION_TYPE_EMMC).
 * @freq_khz   Frequency in KHz (type == IUI_FM_FREQ_NOTIFICATION_TYPE_KHZ).
 *             Used for macros:
 *               IUI_FM_MACRO_ID_CLASS_D
 *               IUI_FM_MACRO_ID_PMU_CP
 *               IUI_FM_MACRO_ID_MS_CP
 *               IUI_FM_MACRO_ID_DCDC
 *               IUI_FM_MACRO_ID_IDI
 * @wlan_info  Pointer to WLAN Macro Information structure (type ==
 *             IUI_FM_FREQ_NOTIFICATION_TYPE_WLAN).
 * @fmr_info   Pointer to FM Radio Operating Parameters structure (type ==
 *             IUI_FM_FREQ_NOTIFICATION_TYPE_FMR).
 * @bt_info    Pointer to Bluetooth Module Info structure (type ==
 *             IUI_FM_FREQ_NOTIFICATION_TYPE_BT).
 * @gnss_info  Pointer to GNSS Macro Information structure (type ==
 *             IUI_FM_FREQ_NOTIFICATION_TYPE_GNSS).
 */
union iui_fm_freq_notification_union {
	struct iui_fm_emmc_freq_info *emmc_info;
	uint32_t freq_khz;
	struct iui_fm_wlan_info *wlan_info;
	struct iui_fm_fmr_info *fmr_info;
	struct iui_fm_bt_info *bt_info;
	struct iui_fm_gnss_info *gnss_info;
};

/**
 * @brief Structure used to pass a Frequency Notification from a Macro to
 *        Frequency Manager.
 *
 * @type  Indicates which field of the "info" union field to use.
 * @info  Union containing or pointing to the macro-specific frequency or
 *        operating information.
 */
struct iui_fm_freq_notification {
	enum iui_fm_freq_notification_type type;
	union iui_fm_freq_notification_union info;
};

/**
 * @brief Indicates which field of the iui_fm_mitigation_union union to use.
 *
 * @IUI_FM_MITIGATION_TYPE_EMMC   EMMC frequency change requested.
 * @IUI_FM_MITIGATION_TYPE_KHZ    Requested change in operating frequency
 *                                (KHz).
 * @IUI_FM_MITIGATION_TYPE_WLAN   Request to set WLAN operating parameters.
 * @IUI_FM_MITIGATION_TYPE_FMR    Change of FM Radio injection side requested.
 * @IUI_FM_MITIGATION_TYPE_BT     Request to set Bluetooth Channel Mask.
 * @IUI_FM_MITIGATION_TYPE_GNSS   Request to set GNSS operating parameters.
 * @IUI_FM_MITIGATION_TYPE_MAX    Number of Frequency Mitigation Types
 *                                defined, used internally for range checking.
 * @IUI_FM_MITIGATION_TYPE_FIRST  First Frequency Mitigation Type defined,
 *                                used internally for range checking.
 */
enum iui_fm_mitigation_type {
	IUI_FM_MITIGATION_TYPE_EMMC = 0,
	IUI_FM_MITIGATION_TYPE_KHZ  = 1,
	IUI_FM_MITIGATION_TYPE_WLAN = 2,
	IUI_FM_MITIGATION_TYPE_FMR  = 3,
	IUI_FM_MITIGATION_TYPE_BT   = 4,
	IUI_FM_MITIGATION_TYPE_GNSS = 5,
	IUI_FM_MITIGATION_TYPE_MAX  = 6,
	IUI_FM_MITIGATION_TYPE_FIRST = IUI_FM_MITIGATION_TYPE_EMMC
};

/**
 * @brief Union of macro-specific Frequency Mitigiation Request message
 *        structures/pointers.
 *
 * @emmc_info        Pointer to requested EMMC operating frequency structure
 *                   (type == IUI_FM_FREQ_REQ_TYPE_EMMC).
 * @freq_khz         Requested frequency in KHz (type ==
 *                   IUI_FM_FREQ_REQ_TYPE_KHZ). Used for macros:
 *                     IUI_FM_MACRO_ID_CLASS_D
 *                     IUI_FM_MACRO_ID_PMU_CP
 *                     IUI_FM_MACRO_ID_MS_CP
 *                     IUI_FM_MACRO_ID_DCDC
 *                     IUI_FM_MACRO_ID_IDI
 * @wlan_mitigation  Pointer to requested WLAN operating parameters structure
 *                   (type == IUI_FM_MITIGATION_TYPE_WLAN).
 * @fmr_inj_side     Requested FM Radio injection side (type ==
 *                   IUI_FM_FREQ_REQ_TYPE_FMR).
 * @bt_ch_mask       Pointer to requested Bluetooth Channel Mask structure
 *                   (type == IUI_FM_FREQ_REQ_TYPE_BT).
 * @gnss_mitigation  Pointer to requested GNSS operating parameters (type ==
 *                   IUI_FM_MITIGATION_TYPE_GNSS).
 */
union iui_fm_mitigation_union {
	struct iui_fm_emmc_freq_info *emmc_info;
	uint32_t freq_khz;
	struct iui_fm_wlan_mitigation *wlan_mitigation;
	enum iui_fm_fmr_injection_side fmr_inj_side;
	struct iui_fm_bt_channel_mask *bt_ch_mask;
	struct iui_fm_gnss_mitigation *gnss_mitigation;
};

/**
 * @brief Strucure used to pass a Frequency Change Request message to a macro.
 *
 * @type  Indicates which field of the "mitigation" union to use.
 * @info  Contains or points to the macro-specific frequency mitigation
 *        information.
 */
struct iui_fm_mitigation {
	enum iui_fm_mitigation_type  type;
	union iui_fm_mitigation_union info;
};

/**
 * @brief Frequency Mitigation status codes.
 *
 * Returned by Frequency Mitigation Callback functions, and passed as an
 * argument to the iui_fm_mitigation_complete() API.
 *
 * @IUI_FM_MITIGATION_COMPLETE_OK  Macro has successfully implemented the
 *      requested change.  If this is returned by a Frequency Mitigation
 *      Callback, then the mitigation was completed synchronously and it is
 *      unnecessary to subsequently call the iui_fm_mitigation_complete() API.
 * @IUI_FM_MITIGATION_COMPLETE_NOT_ACCEPTABLE  Mitigation request was received
 *      successfully, but Macro is unable to make the requested change due to
 *      runtime constraints.  For instance, because the macro is operating in a
 *      mode that does not allow changing. In this case the values contained in
 *      the Frequency Mitigation message are treated as a new Frequency
 *      Notification by the macro.  So the macro may change to a frequency or
 *      mode other than what was requested by Frequency Manager and report the
 *      change in this manner. If this is returned by a Frequency Mitigation
 *      Callback, then the mitigation request was processed synchronously and
 *      it is unnecessary to subsequently call the iui_fm_mitigation_complete()
 *      API.
 * @IUI_FM_MITIGATION_ASYNC_PENDING  Only valid as a return value from a
 *      macro's mitigation callback function. Indicates that the requested
 *      Mitigation operation has been scheduled by the macro, and it will call
 *      the iui_fm_mitigation_complete() API with a different status code once
 *      the mitigation has finished.
 * @IUI_FM_MITIGATION_ERROR  Mitigation is not possible due to an unspecified
 *      error other than a runtime constraint. Such as a runtime error (full
 *      message queue, out of memory, etc.). The contents of the Mitigation
 *      Message are ignored in this case. If this is returned by a Frequency
 *      Mitigation Callback, then the mitigation request was processed
 *      synchronously and it is unnecessary to subsequently call the
 *      iui_fm_mitigation_complete() API.
 * @IUI_FM_MITIGATION_ERROR_INVALID_PARAM  Mitigation is not possible due to an
 *      invalid mitigation request from Frequency Manager. If this is returned
 *      by a Frequency Mitigation Callback, then the mitigation request was
 *      processed synchronously and it is unnecessary to subsequently call the
 *      iui_fm_mitigation_complete() API.
 */
enum iui_fm_mitigation_status {
	IUI_FM_MITIGATION_COMPLETE_OK             = 0,
	IUI_FM_MITIGATION_COMPLETE_NOT_ACCEPTABLE = 1,
	IUI_FM_MITIGATION_ASYNC_PENDING           = 2,
	IUI_FM_MITIGATION_ERROR                   = -1,
	IUI_FM_MITIGATION_ERROR_INVALID_PARAM     = -2
};

/**
 * @brief Prototype for a Frequency Mitigation Request Callback Function, to be
 *        implemented by Frequency Manager Macros to receive Frequency
 *        Mitigation Request messages from Frequency Manager.
 *
 * A macro's implementation of this callback would be called by Frequency
 * Manager to request it to change its operating frequency and/or other
 * operating parameter(s) in order to mitigate a crosstalk or interference
 * between two or more macros.
 *
 * Implementations of this prototype may process the mitigation request
 * synchronously or asynchronously. By returning
 * IUI_FM_MITIGATION_ASYNC_PENDING, an implementation indicates to Frequency
 * Manager that it has scheduled or otherwise queued the mitigation request for
 * processing asynchronously in another context, and further agrees to call the
 * iui_fm_mitigation_complete() API when it is finished processing the
 * Mitigation Request.  Any other return value indicates that the mitigation
 * request has been processed syncrhonously, and no subsequent call to the
 * iui_fm_mitigation_complete() API is needed.
 *
 * If returning IUI_FM_MITIGATION_COMPLETE_NOT_ACCEPTABLE to indicate that the
 * requested mitigation is not possible at the time, the mitigation argument
 * must be updated to reflect the operating mode/frequency of the macro after
 * the callback, which need not be the same as it was prior to the callback.
 * In this way the macro may make a change to the operating mode/frequency in
 * response to the mitigation request, but not exactly what was requested.
 * This may be treated as a new Frequency Notification from the macro by the
 * Frequency Manager.
 *
 * Implementations of this callback prototype are registered and deregistered
 * dynamically via the iui_fm_register_mititagion_callback API.  Only one
 * callback per Macro ID can be registered at a time.
 *
 * @param macro_id     Identifies the macro whose frequency and/or operating
 *                     parameters is being requested to change.
 * @param mitigation   Pointer to the Frequency Mitigation information
 *                     requested by Frequency Manager. If the callback returns
 *                     IUI_FM_MITIGATION_COMPLETE_NOT_ACCEPTABLE, then it is
 *                     also used as an output argument.  In this case the macro
 *                     must update the argument with the current macro-specific
 *                     mitigation information.
 * @param sequence     Sequence number used internally by Frequency Manager to
 *                     synchronize multiple in-flight Frequency Mitigation
 *                     requests to a macro.  If returning
 *                     IUI_FM_MITIGATION_ASYNC_PENDING, then this value must be
 *                     passed back unmodified to the
 *                     iui_fm_mitigation_complete() API once the macro is
 *                     finished processing the Frequency Change Request, unless
 *                     this callback is called again by Frequency Manager with
 *                     a newer Mitigation Request in which case the older
 *                     request may be dropped.
 * @return One of the iui_fm_mitigation_status codes indicating the mitigation
 *         status.
 */
typedef enum iui_fm_mitigation_status (*iui_fm_mitigation_cb) (
				const enum iui_fm_macro_id macro_id,
				const struct iui_fm_mitigation *mitigation,
				const uint32_t sequence);

/**
 * @brief Register a macro's Frequency Mitigation Callback function
 *        implementation with Frequency Manager to receive Frequency
 *        Mitigation messages from Frequency Manager.
 *
 * Also can be used to deregister a macro's callback implementation by passing
 * NULL in the "mitigation_cb" argument.  Only one callback pointer may be
 * registered at a time for each Macro ID.  Registering a new callback pointer
 * for a Macro ID will overwrite the previous registration.
 *
 * @param macro_id       Identifies to which macro the specified Frequency
 *                       Change Request Callback belongs.
 * @param mitigation_cb  Callback pointer to the macro's Frequency Change
 *                       Request Callback function implementation, or NULL
 *                       if deregistering the macro's callback.
 * @return 0 if callback (de)registration was successful, or a negative error
 *         code otherwise.
 */
int32_t iui_fm_register_mitigation_callback (
				const enum iui_fm_macro_id macro_id,
				const iui_fm_mitigation_cb mitigation_cb);

/**
 * @brief Notify the Frequency Manager component of a macro's operating
 *        frequency and/or other parameter(s).
 *
 * @param macro_id       Identifies the macro indicating a change in operating
 *                       frequency and/or other parameter(s).
 * @param notification   Pointer to a populated structure containing the macro-
 *                       specific frequency/operating information.
 * @return 0 if the Frequency Notification was received successfully, or a
 *         negative error code otherwise.
 */
int32_t iui_fm_notify_frequency (const enum iui_fm_macro_id macro_id,
		const struct iui_fm_freq_notification * const notification);

/**
 * @brief Indicate to Frequency Manager that a macro is finished
 *        asynchronously processing a Frequency  Mitigation Request.
 *
 * If a macro's Frequency Mitigation Callback returns
 * IUI_FM_MITIGATION_ASYNC_PENDING, then it must call this API to notify
 * Frequency Manager once it is finished processing the request. It may be
 * called during the execution of the macro's Frequency Mitigation Callback
 * function, or asynchronously from another context. If another Frequency
 * Mitigation is received by the macro before it has called this API for an
 * earlier request, the earlier request may be dropped and this API called
 * only for the later request.
 *
 * @param macro_id   Identifies the macro that has finished processing a
 *                   Frequency Mitigation.
 * @param status     Indicates whether or not the macro has successfully
 *                   implemented the requested change. If not, the mitigation
 *                   field is populated with the macro's current operating
 *                   information and is treated as a new Frequency Indication
 *                   from the macro.
 * @param mitigation Pointer to a structure containing the macro's updated
 *                   mitigation information.
 * @param sequence   Sequence number originally passed to the macro's Frequency
 *                   Change Request callback function.
 * @return 0 if the Frequency Change Response was received successfully, or a
 *         negative error code otherwise.
 */
int32_t iui_fm_mitigation_complete (const enum iui_fm_macro_id macro_id,
			const enum iui_fm_mitigation_status status,
			const struct iui_fm_mitigation * const mitigation,
			const uint32_t sequence);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif  /* _IUI_FM_8000_H */
