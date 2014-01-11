/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_core_if.h $
 * $Revision: #8 $
 * $Date: 2010/06/21 $
 * $Change: 1532033 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */
#if !defined(__DWC_CORE_IF_H__)
#define __DWC_CORE_IF_H__

#include "dwc_os.h"

/** @file
 * This file defines DWC_OTG Core API
 */

struct dwc_otg_core_if;
typedef struct dwc_otg_core_if dwc_otg_core_if_t;

/** Maximum number of Periodic FIFOs */
#define MAX_PERIO_FIFOS 15
/** Maximum number of Periodic FIFOs */
#define MAX_TX_FIFOS 15

/** Maximum number of Endpoints/HostChannels */
#define MAX_EPS_CHANNELS 16

extern dwc_otg_core_if_t *dwc_otg_cil_init(const uint32_t * _reg_base_addr);
extern void dwc_otg_core_init(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_core_reinit(dwc_otg_core_if_t * core_if);

extern void dwc_otg_cil_remove(dwc_otg_core_if_t * _core_if);

extern void dwc_otg_enable_global_interrupts(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_disable_global_interrupts(dwc_otg_core_if_t * _core_if);

extern uint8_t dwc_otg_is_device_mode(dwc_otg_core_if_t * _core_if);
extern uint8_t dwc_otg_is_host_mode(dwc_otg_core_if_t * _core_if);

extern uint8_t dwc_otg_is_dma_enable(dwc_otg_core_if_t * core_if);

/** This function should be called on every hardware interrupt. */
extern int32_t dwc_otg_handle_common_intr(dwc_otg_core_if_t * _core_if);


/** @} */

/** @name Access to registers and bit-fields */

/**
 * Dump core registers and SPRAM
 */
extern void dwc_otg_dump_dev_registers(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_spram(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_host_registers(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_global_registers(dwc_otg_core_if_t * _core_if);

/**
 * Get host negotiation status.
 */
extern uint32_t dwc_otg_get_hnpstatus(dwc_otg_core_if_t * core_if);

/**
 * Get srp status
 */
extern uint32_t dwc_otg_get_srpstatus(dwc_otg_core_if_t * core_if);

/**
 * Set hnpreq bit in the GOTGCTL register.
 */
extern void dwc_otg_set_hnpreq(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get Content of SNPSID register.
 */
extern uint32_t dwc_otg_get_gsnpsid(dwc_otg_core_if_t * core_if);

/**
 * Get current mode.
 * Returns 0 if in device mode, and 1 if in host mode.
 */
extern uint32_t dwc_otg_get_mode(dwc_otg_core_if_t * core_if);

/**
 * Get value of hnpcapable field in the GUSBCFG register
 */
extern uint32_t dwc_otg_get_hnpcapable(dwc_otg_core_if_t * core_if);
/**
 * Set value of hnpcapable field in the GUSBCFG register
 */
extern void dwc_otg_set_hnpcapable(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of srpcapable field in the GUSBCFG register
 */
extern uint32_t dwc_otg_get_srpcapable(dwc_otg_core_if_t * core_if);
/**
 * Set value of srpcapable field in the GUSBCFG register
 */
extern void dwc_otg_set_srpcapable(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of devspeed field in the DCFG register
 */
extern uint32_t dwc_otg_get_devspeed(dwc_otg_core_if_t * core_if);
/**
 * Set value of devspeed field in the DCFG register
 */
extern void dwc_otg_set_devspeed(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get the value of busconnected field from the HPRT0 register
 */
extern uint32_t dwc_otg_get_busconnected(dwc_otg_core_if_t * core_if);

/**
 * Gets the device enumeration Speed.
 */
extern uint32_t dwc_otg_get_enumspeed(dwc_otg_core_if_t * core_if);

/**
 * Get value of prtpwr field from the HPRT0 register
 */
extern uint32_t dwc_otg_get_prtpower(dwc_otg_core_if_t * core_if);

/**
 * Get value of flag indicating core state - hibernated or not
 */
extern uint32_t dwc_otg_get_core_state(dwc_otg_core_if_t * core_if);

/**
 * Set value of prtpwr field from the HPRT0 register
 */
extern void dwc_otg_set_prtpower(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of prtsusp field from the HPRT0 regsiter
 */
extern uint32_t dwc_otg_get_prtsuspend(dwc_otg_core_if_t * core_if);
/**
 * Set value of prtpwr field from the HPRT0 register
 */
extern void dwc_otg_set_prtsuspend(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of ModeChTimEn field from the HCFG regsiter
 */
extern uint32_t dwc_otg_get_mode_ch_tim(dwc_otg_core_if_t * core_if);
/**
 * Set value of ModeChTimEn field from the HCFG regsiter
 */
extern void dwc_otg_set_mode_ch_tim(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of Fram Interval field from the HFIR regsiter
 */
extern uint32_t dwc_otg_get_fr_interval(dwc_otg_core_if_t * core_if);
/**
 * Set value of Frame Interval field from the HFIR regsiter
 */
extern void dwc_otg_set_fr_interval(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Set value of prtres field from the HPRT0 register
 *FIXME Remove?
 */
extern void dwc_otg_set_prtresume(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of rmtwkupsig bit in DCTL register
 */
extern uint32_t dwc_otg_get_remotewakesig(dwc_otg_core_if_t * core_if);

/**
 * Get value of prt_sleep_sts field from the GLPMCFG register
 */
extern uint32_t dwc_otg_get_lpm_portsleepstatus(dwc_otg_core_if_t * core_if);

/**
 * Get value of rem_wkup_en field from the GLPMCFG register
 */
extern uint32_t dwc_otg_get_lpm_remotewakeenabled(dwc_otg_core_if_t * core_if);

/**
 * Get value of appl_resp field from the GLPMCFG register
 */
extern uint32_t dwc_otg_get_lpmresponse(dwc_otg_core_if_t * core_if);
/**
 * Set value of appl_resp field from the GLPMCFG register
 */
extern void dwc_otg_set_lpmresponse(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of hsic_connect field from the GLPMCFG register
 */
extern uint32_t dwc_otg_get_hsic_connect(dwc_otg_core_if_t * core_if);
/**
 * Set value of hsic_connect field from the GLPMCFG register
 */
extern void dwc_otg_set_hsic_connect(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of inv_sel_hsic field from the GLPMCFG register.
 */
extern uint32_t dwc_otg_get_inv_sel_hsic(dwc_otg_core_if_t * core_if);
/**
 * Set value of inv_sel_hsic field from the GLPMFG register.
 */
extern void dwc_otg_set_inv_sel_hsic(dwc_otg_core_if_t * core_if, uint32_t val);

/*
 * Some functions for accessing registers
 */

/**
 *  GOTGCTL register
 */
extern uint32_t dwc_otg_get_gotgctl(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gotgctl(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * GUSBCFG register
 */
extern uint32_t dwc_otg_get_gusbcfg(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gusbcfg(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * GRXFSIZ register
 */
extern uint32_t dwc_otg_get_grxfsiz(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_grxfsiz(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * GNPTXFSIZ register
 */
extern uint32_t dwc_otg_get_gnptxfsiz(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gnptxfsiz(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_gpvndctl(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gpvndctl(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * GGPIO register
 */
extern uint32_t dwc_otg_get_ggpio(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_ggpio(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * GUID register
 */
extern uint32_t dwc_otg_get_guid(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_guid(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * HPRT0 register
 */
extern uint32_t dwc_otg_get_hprt0(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_hprt0(dwc_otg_core_if_t * core_if, uint32_t val);

/**
 * GHPTXFSIZE
 */
extern uint32_t dwc_otg_get_hptxfsiz(dwc_otg_core_if_t * core_if);

/** @} */

#endif				/* __DWC_CORE_IF_H__ */
