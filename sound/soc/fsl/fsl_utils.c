/**
 * Freescale ALSA SoC Machine driver utility
 *
 * Author: Timur Tabi <timur@freescale.com>
 *
 * Copyright 2010 Freescale Semiconductor, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/of_address.h>
#include <sound/soc.h>

#include "fsl_utils.h"

/**
 * fsl_asoc_get_dma_channel - determine the dma channel for a SSI node
 *
 * @ssi_np: pointer to the SSI device tree node
 * @name: name of the phandle pointing to the dma channel
 * @dai: ASoC DAI link pointer to be filled with platform_name
 * @dma_channel_id: dma channel id to be returned
 * @dma_id: dma id to be returned
 *
 * This function determines the dma and channel id for given SSI node.  It
 * also discovers the platform_name for the ASoC DAI link.
 */
int fsl_asoc_get_dma_channel(struct device_node *ssi_np,
			     const char *name,
			     struct snd_soc_dai_link *dai,
			     unsigned int *dma_channel_id,
			     unsigned int *dma_id)
{
	struct resource res;
	struct device_node *dma_channel_np, *dma_np;
	const u32 *iprop;
	int ret;

	dma_channel_np = of_parse_phandle(ssi_np, name, 0);
	if (!dma_channel_np)
		return -EINVAL;

	if (!of_device_is_compatible(dma_channel_np, "fsl,ssi-dma-channel")) {
		of_node_put(dma_channel_np);
		return -EINVAL;
	}

	/* Determine the dev_name for the device_node.  This code mimics the
	 * behavior of of_device_make_bus_id(). We need this because ASoC uses
	 * the dev_name() of the device to match the platform (DMA) device with
	 * the CPU (SSI) device.  It's all ugly and hackish, but it works (for
	 * now).
	 *
	 * dai->platform name should already point to an allocated buffer.
	 */
	ret = of_address_to_resource(dma_channel_np, 0, &res);
	if (ret) {
		of_node_put(dma_channel_np);
		return ret;
	}
	snprintf((char *)dai->platform_name, DAI_NAME_SIZE, "%llx.%s",
		 (unsigned long long) res.start, dma_channel_np->name);

	iprop = of_get_property(dma_channel_np, "cell-index", NULL);
	if (!iprop) {
		of_node_put(dma_channel_np);
		return -EINVAL;
	}
	*dma_channel_id = be32_to_cpup(iprop);

	dma_np = of_get_parent(dma_channel_np);
	iprop = of_get_property(dma_np, "cell-index", NULL);
	if (!iprop) {
		of_node_put(dma_np);
<<<<<<< HEAD
=======
		of_node_put(dma_channel_np);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		return -EINVAL;
	}
	*dma_id = be32_to_cpup(iprop);

	of_node_put(dma_np);
	of_node_put(dma_channel_np);

	return 0;
}
EXPORT_SYMBOL(fsl_asoc_get_dma_channel);

/**
 * fsl_asoc_xlate_tdm_slot_mask - generate TDM slot TX/RX mask.
 *
 * @slots: Number of slots in use.
 * @tx_mask: bitmask representing active TX slots.
 * @rx_mask: bitmask representing active RX slots.
 *
 * This function used to generate the TDM slot TX/RX mask. And the TX/RX
 * mask will use a 0 bit for an active slot as default, and the default
 * active bits are at the LSB of the mask value.
 */
int fsl_asoc_xlate_tdm_slot_mask(unsigned int slots,
				    unsigned int *tx_mask,
				    unsigned int *rx_mask)
{
	if (!slots)
		return -EINVAL;

	if (tx_mask)
		*tx_mask = ~((1 << slots) - 1);
	if (rx_mask)
		*rx_mask = ~((1 << slots) - 1);

	return 0;
}
EXPORT_SYMBOL_GPL(fsl_asoc_xlate_tdm_slot_mask);

MODULE_AUTHOR("Timur Tabi <timur@freescale.com>");
MODULE_DESCRIPTION("Freescale ASoC utility code");
MODULE_LICENSE("GPL v2");
