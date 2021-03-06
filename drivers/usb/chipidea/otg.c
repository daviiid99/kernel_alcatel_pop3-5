/*
 * otg.c - ChipIdea USB IP core OTG driver
 *
 * Copyright (C) 2013 Freescale Semiconductor, Inc.
 *
 * Author: Peter Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This file mainly handles otgsc register, OTG fsm operations for HNP and SRP
 * are also included.
 */

#include <linux/usb/otg.h>
#include <linux/usb/gadget.h>
#include <linux/usb/chipidea.h>

#include "ci.h"
#include "bits.h"
#include "otg.h"
#include "otg_fsm.h"

/**
 * hw_read_otgsc returns otgsc register bits value.
 * @mask: bitfield mask
 */
u32 hw_read_otgsc(struct ci_hdrc *ci, u32 mask)
{
	return hw_read(ci, OP_OTGSC, mask);
}

/**
 * hw_write_otgsc updates target bits of OTGSC register.
 * @mask: bitfield mask
 * @data: to be written
 */
void hw_write_otgsc(struct ci_hdrc *ci, u32 mask, u32 data)
{
	hw_write(ci, OP_OTGSC, mask | OTGSC_INT_STATUS_BITS, data);
}

/**
 * ci_otg_role - pick role based on ID pin state
 * @ci: the controller
 */
enum ci_role ci_otg_role(struct ci_hdrc *ci)
{
	enum ci_role role = hw_read_otgsc(ci, OTGSC_ID)
		? CI_ROLE_GADGET
		: CI_ROLE_HOST;

	return role;
}

void ci_handle_vbus_change(struct ci_hdrc *ci)
{
	if (!ci->is_otg)
		return;

	if (hw_read_otgsc(ci, OTGSC_BSV))
		usb_gadget_vbus_connect(&ci->gadget);
	else
		usb_gadget_vbus_disconnect(&ci->gadget);
}

<<<<<<< HEAD
#define CI_VBUS_STABLE_TIMEOUT_MS 5000
=======
/**
 * When we switch to device mode, the vbus value should be lower
 * than OTGSC_BSV before connecting to host.
 *
 * @ci: the controller
 *
 * This function returns an error code if timeout
 */
static int hw_wait_vbus_lower_bsv(struct ci_hdrc *ci)
{
	unsigned long elapse = jiffies + msecs_to_jiffies(5000);
	u32 mask = OTGSC_BSV;

	while (hw_read_otgsc(ci, mask)) {
		if (time_after(jiffies, elapse)) {
			dev_err(ci->dev, "timeout waiting for %08x in OTGSC\n",
					mask);
			return -ETIMEDOUT;
		}
		msleep(20);
	}

	return 0;
}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
static void ci_handle_id_switch(struct ci_hdrc *ci)
{
	enum ci_role role = ci_otg_role(ci);

	if (role != ci->role) {
		dev_dbg(ci->dev, "switching from %s to %s\n",
			ci_role(ci)->name, ci->roles[role]->name);

		ci_role_stop(ci);
<<<<<<< HEAD
		/* wait vbus lower than OTGSC_BSV */
		hw_wait_reg(ci, OP_OTGSC, OTGSC_BSV, 0,
				CI_VBUS_STABLE_TIMEOUT_MS);
=======

		/*
		 * wait vbus lower than OTGSC_BSV before connecting
		 * to host
		 */
		hw_wait_vbus_lower_bsv(ci);

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		ci_role_start(ci, role);
	}
}
/**
 * ci_otg_work - perform otg (vbus/id) event handle
 * @work: work struct
 */
static void ci_otg_work(struct work_struct *work)
{
	struct ci_hdrc *ci = container_of(work, struct ci_hdrc, work);

	if (ci_otg_is_fsm_mode(ci) && !ci_otg_fsm_work(ci)) {
		enable_irq(ci->irq);
		return;
	}

	if (ci->id_event) {
		ci->id_event = false;
		ci_handle_id_switch(ci);
<<<<<<< HEAD
	} else if (ci->b_sess_valid_event) {
		ci->b_sess_valid_event = false;
		ci_handle_vbus_change(ci);
	} else
		dev_err(ci->dev, "unexpected event occurs at %s\n", __func__);
=======
	}

	if (ci->b_sess_valid_event) {
		ci->b_sess_valid_event = false;
		ci_handle_vbus_change(ci);
	}
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	enable_irq(ci->irq);
}


/**
 * ci_hdrc_otg_init - initialize otg struct
 * ci: the controller
 */
int ci_hdrc_otg_init(struct ci_hdrc *ci)
{
	INIT_WORK(&ci->work, ci_otg_work);
<<<<<<< HEAD
	ci->wq = create_singlethread_workqueue("ci_otg");
=======
	ci->wq = create_freezable_workqueue("ci_otg");
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (!ci->wq) {
		dev_err(ci->dev, "can't create workqueue\n");
		return -ENODEV;
	}

	if (ci_otg_is_fsm_mode(ci))
		return ci_hdrc_otg_fsm_init(ci);

	return 0;
}

/**
 * ci_hdrc_otg_destroy - destroy otg struct
 * ci: the controller
 */
void ci_hdrc_otg_destroy(struct ci_hdrc *ci)
{
	if (ci->wq) {
		flush_workqueue(ci->wq);
		destroy_workqueue(ci->wq);
	}
	/* Disable all OTG irq and clear status */
	hw_write_otgsc(ci, OTGSC_INT_EN_BITS | OTGSC_INT_STATUS_BITS,
						OTGSC_INT_STATUS_BITS);
	if (ci_otg_is_fsm_mode(ci))
		ci_hdrc_otg_fsm_remove(ci);
}
