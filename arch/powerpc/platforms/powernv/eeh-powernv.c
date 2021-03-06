/*
 * The file intends to implement the platform dependent EEH operations on
 * powernv platform. Actually, the powernv was created in order to fully
 * hypervisor support.
 *
 * Copyright Benjamin Herrenschmidt & Gavin Shan, IBM Corporation 2013.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/msi.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>

#include <asm/eeh.h>
#include <asm/eeh_event.h>
#include <asm/firmware.h>
#include <asm/io.h>
#include <asm/iommu.h>
#include <asm/machdep.h>
#include <asm/msi_bitmap.h>
#include <asm/opal.h>
#include <asm/ppc-pci.h>

#include "powernv.h"
#include "pci.h"

/**
<<<<<<< HEAD
 * powernv_eeh_init - EEH platform dependent initialization
 *
 * EEH platform dependent initialization on powernv
 */
static int powernv_eeh_init(void)
=======
 * pnv_eeh_init - EEH platform dependent initialization
 *
 * EEH platform dependent initialization on powernv
 */
static int pnv_eeh_init(void)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose;
	struct pnv_phb *phb;

	/* We require OPALv3 */
	if (!firmware_has_feature(FW_FEATURE_OPALv3)) {
		pr_warn("%s: OPALv3 is required !\n",
			__func__);
		return -EINVAL;
	}

	/* Set probe mode */
	eeh_add_flag(EEH_PROBE_MODE_DEV);

	/*
	 * P7IOC blocks PCI config access to frozen PE, but PHB3
	 * doesn't do that. So we have to selectively enable I/O
	 * prior to collecting error log.
	 */
	list_for_each_entry(hose, &hose_list, list_node) {
		phb = hose->private_data;

		if (phb->model == PNV_PHB_MODEL_P7IOC)
			eeh_add_flag(EEH_ENABLE_IO_FOR_LOG);
		break;
	}

	return 0;
}

/**
<<<<<<< HEAD
 * powernv_eeh_post_init - EEH platform dependent post initialization
=======
 * pnv_eeh_post_init - EEH platform dependent post initialization
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 *
 * EEH platform dependent post initialization on powernv. When
 * the function is called, the EEH PEs and devices should have
 * been built. If the I/O cache staff has been built, EEH is
 * ready to supply service.
 */
<<<<<<< HEAD
static int powernv_eeh_post_init(void)
=======
static int pnv_eeh_post_init(void)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose;
	struct pnv_phb *phb;
	int ret = 0;

	list_for_each_entry(hose, &hose_list, list_node) {
		phb = hose->private_data;

		if (phb->eeh_ops && phb->eeh_ops->post_init) {
			ret = phb->eeh_ops->post_init(hose);
			if (ret)
				break;
		}
	}

	return ret;
}

/**
<<<<<<< HEAD
 * powernv_eeh_dev_probe - Do probe on PCI device
=======
 * pnv_eeh_dev_probe - Do probe on PCI device
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @dev: PCI device
 * @flag: unused
 *
 * When EEH module is installed during system boot, all PCI devices
 * are checked one by one to see if it supports EEH. The function
 * is introduced for the purpose. By default, EEH has been enabled
 * on all PCI devices. That's to say, we only need do necessary
 * initialization on the corresponding eeh device and create PE
 * accordingly.
 *
 * It's notable that's unsafe to retrieve the EEH device through
 * the corresponding PCI device. During the PCI device hotplug, which
 * was possiblly triggered by EEH core, the binding between EEH device
 * and the PCI device isn't built yet.
 */
<<<<<<< HEAD
static int powernv_eeh_dev_probe(struct pci_dev *dev, void *flag)
=======
static int pnv_eeh_dev_probe(struct pci_dev *dev, void *flag)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pci_bus_to_host(dev->bus);
	struct pnv_phb *phb = hose->private_data;
	struct device_node *dn = pci_device_to_OF_node(dev);
	struct eeh_dev *edev = of_node_to_eeh_dev(dn);
	int ret;

	/*
	 * When probing the root bridge, which doesn't have any
	 * subordinate PCI devices. We don't have OF node for
	 * the root bridge. So it's not reasonable to continue
	 * the probing.
	 */
	if (!dn || !edev || edev->pe)
		return 0;

	/* Skip for PCI-ISA bridge */
	if ((dev->class >> 8) == PCI_CLASS_BRIDGE_ISA)
		return 0;

	/* Initialize eeh device */
	edev->class_code = dev->class;
	edev->mode	&= 0xFFFFFF00;
	if (dev->hdr_type == PCI_HEADER_TYPE_BRIDGE)
		edev->mode |= EEH_DEV_BRIDGE;
	edev->pcix_cap = pci_find_capability(dev, PCI_CAP_ID_PCIX);
	if (pci_is_pcie(dev)) {
		edev->pcie_cap = pci_pcie_cap(dev);

		if (pci_pcie_type(dev) == PCI_EXP_TYPE_ROOT_PORT)
			edev->mode |= EEH_DEV_ROOT_PORT;
		else if (pci_pcie_type(dev) == PCI_EXP_TYPE_DOWNSTREAM)
			edev->mode |= EEH_DEV_DS_PORT;

		edev->aer_cap = pci_find_ext_capability(dev,
							PCI_EXT_CAP_ID_ERR);
	}

	edev->config_addr	= ((dev->bus->number << 8) | dev->devfn);
	edev->pe_config_addr	= phb->bdfn_to_pe(phb, dev->bus, dev->devfn & 0xff);

	/* Create PE */
	ret = eeh_add_to_parent_pe(edev);
	if (ret) {
		pr_warn("%s: Can't add PCI dev %s to parent PE (%d)\n",
			__func__, pci_name(dev), ret);
		return ret;
	}

	/*
	 * If the PE contains any one of following adapters, the
	 * PCI config space can't be accessed when dumping EEH log.
	 * Otherwise, we will run into fenced PHB caused by shortage
	 * of outbound credits in the adapter. The PCI config access
	 * should be blocked until PE reset. MMIO access is dropped
	 * by hardware certainly. In order to drop PCI config requests,
	 * one more flag (EEH_PE_CFG_RESTRICTED) is introduced, which
	 * will be checked in the backend for PE state retrival. If
	 * the PE becomes frozen for the first time and the flag has
	 * been set for the PE, we will set EEH_PE_CFG_BLOCKED for
	 * that PE to block its config space.
	 *
	 * Broadcom Austin 4-ports NICs (14e4:1657)
	 * Broadcom Shiner 2-ports 10G NICs (14e4:168e)
	 */
	if ((dev->vendor == PCI_VENDOR_ID_BROADCOM && dev->device == 0x1657) ||
	    (dev->vendor == PCI_VENDOR_ID_BROADCOM && dev->device == 0x168e))
		edev->pe->state |= EEH_PE_CFG_RESTRICTED;

	/*
	 * Cache the PE primary bus, which can't be fetched when
	 * full hotplug is in progress. In that case, all child
	 * PCI devices of the PE are expected to be removed prior
	 * to PE reset.
	 */
	if (!edev->pe->bus)
		edev->pe->bus = dev->bus;

	/*
	 * Enable EEH explicitly so that we will do EEH check
	 * while accessing I/O stuff
	 */
	eeh_add_flag(EEH_ENABLED);

	/* Save memory bars */
	eeh_save_bars(edev);

	return 0;
}

/**
<<<<<<< HEAD
 * powernv_eeh_set_option - Initialize EEH or MMIO/DMA reenable
=======
 * pnv_eeh_set_option - Initialize EEH or MMIO/DMA reenable
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 * @option: operation to be issued
 *
 * The function is used to control the EEH functionality globally.
 * Currently, following options are support according to PAPR:
 * Enable EEH, Disable EEH, Enable MMIO and Enable DMA
 */
<<<<<<< HEAD
static int powernv_eeh_set_option(struct eeh_pe *pe, int option)
=======
static int pnv_eeh_set_option(struct eeh_pe *pe, int option)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pe->phb;
	struct pnv_phb *phb = hose->private_data;
	int ret = -EEXIST;

	/*
	 * What we need do is pass it down for hardware
	 * implementation to handle it.
	 */
	if (phb->eeh_ops && phb->eeh_ops->set_option)
		ret = phb->eeh_ops->set_option(pe, option);

	return ret;
}

/**
<<<<<<< HEAD
 * powernv_eeh_get_pe_addr - Retrieve PE address
=======
 * pnv_eeh_get_pe_addr - Retrieve PE address
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 *
 * Retrieve the PE address according to the given tranditional
 * PCI BDF (Bus/Device/Function) address.
 */
<<<<<<< HEAD
static int powernv_eeh_get_pe_addr(struct eeh_pe *pe)
=======
static int pnv_eeh_get_pe_addr(struct eeh_pe *pe)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	return pe->addr;
}

/**
<<<<<<< HEAD
 * powernv_eeh_get_state - Retrieve PE state
=======
 * pnv_eeh_get_state - Retrieve PE state
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 * @delay: delay while PE state is temporarily unavailable
 *
 * Retrieve the state of the specified PE. For IODA-compitable
 * platform, it should be retrieved from IODA table. Therefore,
 * we prefer passing down to hardware implementation to handle
 * it.
 */
<<<<<<< HEAD
static int powernv_eeh_get_state(struct eeh_pe *pe, int *delay)
=======
static int pnv_eeh_get_state(struct eeh_pe *pe, int *delay)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pe->phb;
	struct pnv_phb *phb = hose->private_data;
	int ret = EEH_STATE_NOT_SUPPORT;

	if (phb->eeh_ops && phb->eeh_ops->get_state) {
		ret = phb->eeh_ops->get_state(pe);

		/*
		 * If the PE state is temporarily unavailable,
		 * to inform the EEH core delay for default
		 * period (1 second)
		 */
		if (delay) {
			*delay = 0;
			if (ret & EEH_STATE_UNAVAILABLE)
				*delay = 1000;
		}
	}

	return ret;
}

/**
<<<<<<< HEAD
 * powernv_eeh_reset - Reset the specified PE
=======
 * pnv_eeh_reset - Reset the specified PE
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 * @option: reset option
 *
 * Reset the specified PE
 */
<<<<<<< HEAD
static int powernv_eeh_reset(struct eeh_pe *pe, int option)
=======
static int pnv_eeh_reset(struct eeh_pe *pe, int option)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pe->phb;
	struct pnv_phb *phb = hose->private_data;
	int ret = -EEXIST;

	if (phb->eeh_ops && phb->eeh_ops->reset)
		ret = phb->eeh_ops->reset(pe, option);

	return ret;
}

/**
<<<<<<< HEAD
 * powernv_eeh_wait_state - Wait for PE state
=======
 * pnv_eeh_wait_state - Wait for PE state
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 * @max_wait: maximal period in microsecond
 *
 * Wait for the state of associated PE. It might take some time
 * to retrieve the PE's state.
 */
<<<<<<< HEAD
static int powernv_eeh_wait_state(struct eeh_pe *pe, int max_wait)
=======
static int pnv_eeh_wait_state(struct eeh_pe *pe, int max_wait)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	int ret;
	int mwait;

	while (1) {
<<<<<<< HEAD
		ret = powernv_eeh_get_state(pe, &mwait);
=======
		ret = pnv_eeh_get_state(pe, &mwait);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

		/*
		 * If the PE's state is temporarily unavailable,
		 * we have to wait for the specified time. Otherwise,
		 * the PE's state will be returned immediately.
		 */
		if (ret != EEH_STATE_UNAVAILABLE)
			return ret;

		max_wait -= mwait;
		if (max_wait <= 0) {
			pr_warn("%s: Timeout getting PE#%x's state (%d)\n",
				__func__, pe->addr, max_wait);
			return EEH_STATE_NOT_SUPPORT;
		}

		msleep(mwait);
	}

	return EEH_STATE_NOT_SUPPORT;
}

/**
<<<<<<< HEAD
 * powernv_eeh_get_log - Retrieve error log
=======
 * pnv_eeh_get_log - Retrieve error log
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 * @severity: temporary or permanent error log
 * @drv_log: driver log to be combined with retrieved error log
 * @len: length of driver log
 *
 * Retrieve the temporary or permanent error from the PE.
 */
<<<<<<< HEAD
static int powernv_eeh_get_log(struct eeh_pe *pe, int severity,
			       char *drv_log, unsigned long len)
=======
static int pnv_eeh_get_log(struct eeh_pe *pe, int severity,
			   char *drv_log, unsigned long len)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pe->phb;
	struct pnv_phb *phb = hose->private_data;
	int ret = -EEXIST;

	if (phb->eeh_ops && phb->eeh_ops->get_log)
		ret = phb->eeh_ops->get_log(pe, severity, drv_log, len);

	return ret;
}

/**
<<<<<<< HEAD
 * powernv_eeh_configure_bridge - Configure PCI bridges in the indicated PE
=======
 * pnv_eeh_configure_bridge - Configure PCI bridges in the indicated PE
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: EEH PE
 *
 * The function will be called to reconfigure the bridges included
 * in the specified PE so that the mulfunctional PE would be recovered
 * again.
 */
<<<<<<< HEAD
static int powernv_eeh_configure_bridge(struct eeh_pe *pe)
=======
static int pnv_eeh_configure_bridge(struct eeh_pe *pe)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pe->phb;
	struct pnv_phb *phb = hose->private_data;
	int ret = 0;

	if (phb->eeh_ops && phb->eeh_ops->configure_bridge)
		ret = phb->eeh_ops->configure_bridge(pe);

	return ret;
}

/**
<<<<<<< HEAD
 * powernv_pe_err_inject - Inject specified error to the indicated PE
=======
 * pnv_pe_err_inject - Inject specified error to the indicated PE
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: the indicated PE
 * @type: error type
 * @func: specific error type
 * @addr: address
 * @mask: address mask
 *
 * The routine is called to inject specified error, which is
 * determined by @type and @func, to the indicated PE for
 * testing purpose.
 */
<<<<<<< HEAD
static int powernv_eeh_err_inject(struct eeh_pe *pe, int type, int func,
				  unsigned long addr, unsigned long mask)
=======
static int pnv_eeh_err_inject(struct eeh_pe *pe, int type, int func,
			      unsigned long addr, unsigned long mask)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose = pe->phb;
	struct pnv_phb *phb = hose->private_data;
	int ret = -EEXIST;

	if (phb->eeh_ops && phb->eeh_ops->err_inject)
		ret = phb->eeh_ops->err_inject(pe, type, func, addr, mask);

	return ret;
}

<<<<<<< HEAD
static inline bool powernv_eeh_cfg_blocked(struct device_node *dn)
=======
static inline bool pnv_eeh_cfg_blocked(struct device_node *dn)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct eeh_dev *edev = of_node_to_eeh_dev(dn);

	if (!edev || !edev->pe)
		return false;

	if (edev->pe->state & EEH_PE_CFG_BLOCKED)
		return true;

	return false;
}

<<<<<<< HEAD
static int powernv_eeh_read_config(struct device_node *dn,
				   int where, int size, u32 *val)
{
	if (powernv_eeh_cfg_blocked(dn)) {
=======
static int pnv_eeh_read_config(struct device_node *dn,
			       int where, int size, u32 *val)
{
	if (pnv_eeh_cfg_blocked(dn)) {
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		*val = 0xFFFFFFFF;
		return PCIBIOS_SET_FAILED;
	}

	return pnv_pci_cfg_read(dn, where, size, val);
}

<<<<<<< HEAD
static int powernv_eeh_write_config(struct device_node *dn,
				    int where, int size, u32 val)
{
	if (powernv_eeh_cfg_blocked(dn))
=======
static int pnv_eeh_write_config(struct device_node *dn,
				int where, int size, u32 val)
{
	if (pnv_eeh_cfg_blocked(dn))
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		return PCIBIOS_SET_FAILED;

	return pnv_pci_cfg_write(dn, where, size, val);
}

/**
<<<<<<< HEAD
 * powernv_eeh_next_error - Retrieve next EEH error to handle
=======
 * pnv_eeh_next_error - Retrieve next EEH error to handle
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 * @pe: Affected PE
 *
 * Using OPAL API, to retrieve next EEH error for EEH core to handle
 */
<<<<<<< HEAD
static int powernv_eeh_next_error(struct eeh_pe **pe)
=======
static int pnv_eeh_next_error(struct eeh_pe **pe)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct pci_controller *hose;
	struct pnv_phb *phb = NULL;

	list_for_each_entry(hose, &hose_list, list_node) {
		phb = hose->private_data;
		break;
	}

	if (phb && phb->eeh_ops->next_error)
		return phb->eeh_ops->next_error(pe);

	return -EEXIST;
}

<<<<<<< HEAD
static int powernv_eeh_restore_config(struct device_node *dn)
=======
static int pnv_eeh_restore_config(struct device_node *dn)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct eeh_dev *edev = of_node_to_eeh_dev(dn);
	struct pnv_phb *phb;
	s64 ret;

	if (!edev)
		return -EEXIST;

	phb = edev->phb->private_data;
	ret = opal_pci_reinit(phb->opal_id,
			      OPAL_REINIT_PCI_DEV, edev->config_addr);
	if (ret) {
		pr_warn("%s: Can't reinit PCI dev 0x%x (%lld)\n",
			__func__, edev->config_addr, ret);
		return -EIO;
	}

	return 0;
}

<<<<<<< HEAD
static struct eeh_ops powernv_eeh_ops = {
	.name                   = "powernv",
	.init                   = powernv_eeh_init,
	.post_init              = powernv_eeh_post_init,
	.of_probe               = NULL,
	.dev_probe              = powernv_eeh_dev_probe,
	.set_option             = powernv_eeh_set_option,
	.get_pe_addr            = powernv_eeh_get_pe_addr,
	.get_state              = powernv_eeh_get_state,
	.reset                  = powernv_eeh_reset,
	.wait_state             = powernv_eeh_wait_state,
	.get_log                = powernv_eeh_get_log,
	.configure_bridge       = powernv_eeh_configure_bridge,
	.err_inject		= powernv_eeh_err_inject,
	.read_config            = powernv_eeh_read_config,
	.write_config           = powernv_eeh_write_config,
	.next_error		= powernv_eeh_next_error,
	.restore_config		= powernv_eeh_restore_config
=======
static struct eeh_ops pnv_eeh_ops = {
	.name                   = "powernv",
	.init                   = pnv_eeh_init,
	.post_init              = pnv_eeh_post_init,
	.of_probe               = NULL,
	.dev_probe              = pnv_eeh_dev_probe,
	.set_option             = pnv_eeh_set_option,
	.get_pe_addr            = pnv_eeh_get_pe_addr,
	.get_state              = pnv_eeh_get_state,
	.reset                  = pnv_eeh_reset,
	.wait_state             = pnv_eeh_wait_state,
	.get_log                = pnv_eeh_get_log,
	.configure_bridge       = pnv_eeh_configure_bridge,
	.err_inject		= pnv_eeh_err_inject,
	.read_config            = pnv_eeh_read_config,
	.write_config           = pnv_eeh_write_config,
	.next_error		= pnv_eeh_next_error,
	.restore_config		= pnv_eeh_restore_config
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
};

/**
 * eeh_powernv_init - Register platform dependent EEH operations
 *
 * EEH initialization on powernv platform. This function should be
 * called before any EEH related functions.
 */
static int __init eeh_powernv_init(void)
{
	int ret = -EINVAL;

	eeh_set_pe_aux_size(PNV_PCI_DIAG_BUF_SIZE);
<<<<<<< HEAD
	ret = eeh_ops_register(&powernv_eeh_ops);
=======
	ret = eeh_ops_register(&pnv_eeh_ops);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (!ret)
		pr_info("EEH: PowerNV platform initialized\n");
	else
		pr_info("EEH: Failed to initialize PowerNV platform (%d)\n", ret);

	return ret;
}
machine_early_initcall(powernv, eeh_powernv_init);
