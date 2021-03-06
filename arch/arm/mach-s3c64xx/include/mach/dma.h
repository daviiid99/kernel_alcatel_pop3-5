/* linux/arch/arm/mach-s3c6400/include/mach/dma.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S3C6400 - DMA support
 */

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H __FILE__

#define S3C64XX_DMA_CHAN(name)		((unsigned long)(name))

/* DMA0/SDMA0 */
<<<<<<< HEAD
#define DMACH_UART0		S3C64XX_DMA_CHAN("uart0_tx")
#define DMACH_UART0_SRC2	S3C64XX_DMA_CHAN("uart0_rx")
#define DMACH_UART1		S3C64XX_DMA_CHAN("uart1_tx")
#define DMACH_UART1_SRC2	S3C64XX_DMA_CHAN("uart1_rx")
#define DMACH_UART2		S3C64XX_DMA_CHAN("uart2_tx")
#define DMACH_UART2_SRC2	S3C64XX_DMA_CHAN("uart2_rx")
#define DMACH_UART3		S3C64XX_DMA_CHAN("uart3_tx")
#define DMACH_UART3_SRC2	S3C64XX_DMA_CHAN("uart3_rx")
#define DMACH_PCM0_TX		S3C64XX_DMA_CHAN("pcm0_tx")
#define DMACH_PCM0_RX		S3C64XX_DMA_CHAN("pcm0_rx")
#define DMACH_I2S0_OUT		S3C64XX_DMA_CHAN("i2s0_tx")
#define DMACH_I2S0_IN		S3C64XX_DMA_CHAN("i2s0_rx")
#define DMACH_SPI0_TX		S3C64XX_DMA_CHAN("spi0_tx")
#define DMACH_SPI0_RX		S3C64XX_DMA_CHAN("spi0_rx")
#define DMACH_HSI_I2SV40_TX	S3C64XX_DMA_CHAN("i2s2_tx")
#define DMACH_HSI_I2SV40_RX	S3C64XX_DMA_CHAN("i2s2_rx")

/* DMA1/SDMA1 */
#define DMACH_PCM1_TX		S3C64XX_DMA_CHAN("pcm1_tx")
#define DMACH_PCM1_RX		S3C64XX_DMA_CHAN("pcm1_rx")
#define DMACH_I2S1_OUT		S3C64XX_DMA_CHAN("i2s1_tx")
#define DMACH_I2S1_IN		S3C64XX_DMA_CHAN("i2s1_rx")
#define DMACH_SPI1_TX		S3C64XX_DMA_CHAN("spi1_tx")
#define DMACH_SPI1_RX		S3C64XX_DMA_CHAN("spi1_rx")
#define DMACH_AC97_PCMOUT	S3C64XX_DMA_CHAN("ac97_out")
#define DMACH_AC97_PCMIN	S3C64XX_DMA_CHAN("ac97_in")
#define DMACH_AC97_MICIN	S3C64XX_DMA_CHAN("ac97_mic")
#define DMACH_PWM		S3C64XX_DMA_CHAN("pwm")
#define DMACH_IRDA		S3C64XX_DMA_CHAN("irda")
#define DMACH_EXTERNAL		S3C64XX_DMA_CHAN("external")
#define DMACH_SECURITY_RX	S3C64XX_DMA_CHAN("sec_rx")
#define DMACH_SECURITY_TX	S3C64XX_DMA_CHAN("sec_tx")
=======
#define DMACH_UART0		"uart0_tx"
#define DMACH_UART0_SRC2	"uart0_rx"
#define DMACH_UART1		"uart1_tx"
#define DMACH_UART1_SRC2	"uart1_rx"
#define DMACH_UART2		"uart2_tx"
#define DMACH_UART2_SRC2	"uart2_rx"
#define DMACH_UART3		"uart3_tx"
#define DMACH_UART3_SRC2	"uart3_rx"
#define DMACH_PCM0_TX		"pcm0_tx"
#define DMACH_PCM0_RX		"pcm0_rx"
#define DMACH_I2S0_OUT		"i2s0_tx"
#define DMACH_I2S0_IN		"i2s0_rx"
#define DMACH_SPI0_TX		S3C64XX_DMA_CHAN("spi0_tx")
#define DMACH_SPI0_RX		S3C64XX_DMA_CHAN("spi0_rx")
#define DMACH_HSI_I2SV40_TX	"i2s2_tx"
#define DMACH_HSI_I2SV40_RX	"i2s2_rx"

/* DMA1/SDMA1 */
#define DMACH_PCM1_TX		"pcm1_tx"
#define DMACH_PCM1_RX		"pcm1_rx"
#define DMACH_I2S1_OUT		"i2s1_tx"
#define DMACH_I2S1_IN		"i2s1_rx"
#define DMACH_SPI1_TX		S3C64XX_DMA_CHAN("spi1_tx")
#define DMACH_SPI1_RX		S3C64XX_DMA_CHAN("spi1_rx")
#define DMACH_AC97_PCMOUT	"ac97_out"
#define DMACH_AC97_PCMIN	"ac97_in"
#define DMACH_AC97_MICIN	"ac97_mic"
#define DMACH_PWM		"pwm"
#define DMACH_IRDA		"irda"
#define DMACH_EXTERNAL		"external"
#define DMACH_SECURITY_RX	"sec_rx"
#define DMACH_SECURITY_TX	"sec_tx"
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

enum dma_ch {
	DMACH_MAX = 32
};

struct s3c2410_dma_client {
	char	*name;
};

static inline bool samsung_dma_has_circular(void)
{
	return true;
}

static inline bool samsung_dma_is_dmadev(void)
{
	return true;
}

#include <linux/amba/pl08x.h>
#include <plat/dma-ops.h>

#endif /* __ASM_ARCH_IRQ_H */
