/*
 * linux/include/asm-mips/mach-jz4780/jz4780bch.h
 *
 * JZ4780 bch register definition.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __BCH_H__
#define __BCH_H__

/*************************************************************************
 * BCH
 *************************************************************************/
#define BCH_CR		(BCH_BASE + 0x00) /* BCH Control register */
#define BCH_CRS		(BCH_BASE + 0x04) /* BCH Control Set register */
#define BCH_CRC		(BCH_BASE + 0x08) /* BCH Control Clear register */
#define BCH_CNT		(BCH_BASE + 0x0C) /* BCH ENC/DEC Count register */
#define BCH_DR		(BCH_BASE + 0x10) /* BCH Data register */
#define BCH_PAR0	(BCH_BASE + 0x14) /* BCH Parity 0 register */
#define BCH_PAR(n)	(BCH_PAR0 + ((n) << 2)) /* BCH Parity n register */
#define BCH_ERR0	(BCH_BASE + 0x84) /* BCH Error Report 0 register */
#define BCH_ERR(n)	(BCH_ERR0 + ((n) << 2)) /* BCH Parity n register */
#define BCH_INTS	(BCH_BASE + 0x184) /* BCH Interrupt Status register */
#define BCH_INTES	(BCH_BASE + 0x188) /* BCH Interrupt Set register */
#define BCH_INTEC	(BCH_BASE + 0x18C) /* BCH Interrupt Clear register */
#define BCH_INTE	(BCH_BASE + 0x190) /* BCH Interrupt Enable register */
#define BCH_TO		(BCH_BASE + 0x194) /* BCH User Tag Output register */

/* BCH Control Register*/
#define BCH_CR_TAG_BIT		16 /* BCH User-provided TAG */
#define BCH_CR_TAG_MASK		(0xffff << BCH_CR_TAG_BIT)
#define BCH_CR_MZSB_BIT		13 /* BCH MAX Zero Bit in Erased Block */
#define BCH_CR_MZSB_MASK(n)	(((n) > 12?0x7:0x2) << BCH_CR_MZSB_BIT)
#define BCH_CR_BPS		(1 << 12)  /* BCH Decoder Bypass */
#define BCH_CR_BSEL_BIT		4
#define BCH_CR_BSEL_MASK	(0x7f << BCH_CR_BSEL_BIT)
#define BCH_CR_BSEL(n)		((n) << BCH_CR_BSEL_BIT)  /* n Bit BCH Select */
  #define BCH_CR_BSEL_4		(0x4 << BCH_CR_BSEL_BIT)  /* 4 Bit BCH Select */
  #define BCH_CR_BSEL_8		(0x8 << BCH_CR_BSEL_BIT)  /* 8 Bit BCH Select */
  #define BCH_CR_BSEL_24	(0x18 << BCH_CR_BSEL_BIT)  /* 24 Bit BCH Select */
  #define BCH_CR_BSEL_40	(0x28 << BCH_CR_BSEL_BIT)  /* 40 Bit BCH Select */
  #define BCH_CR_BSEL_64	(0x40 << BCH_CR_BSEL_BIT)  /* 64 Bit BCH Select */
#define	BCH_CR_ENCE		(1 << 2)  /* BCH Encoding Select */
#define	BCH_CR_DECE		(0 << 2)  /* BCH Decoding Select */
#define	BCH_CR_INIT		(1 << 1)  /* BCH Reset */
#define	BCH_CR_BCHE		(1 << 0)  /* BCH Enable */

/* BCH ENC/DEC Count Register */
#define BCH_CNT_PARITY_BIT	16
#define BCH_CNT_PARITY_MASK	(0x7f << BCH_CNT_PARITY_BIT)
#define BCH_CNT_BLOCK_BIT	0
#define BCH_CNT_BLOCK_MASK	(0x7ff << BCH_CNT_BLOCK_BIT)

/* BCH Error Report Register */
#define BCH_ERR_MASK_BIT        16
#define BCH_ERR_MASK_MASK	(0xffff << BCH_ERR_MASK_BIT)
#define BCH_ERR_INDEX_BIT       0
#define BCH_ERR_INDEX_MASK      (0x7ff << BCH_ERR_INDEX_BIT)

/* BCH Interrupt Status Register */
#define	BCH_INTS_ERRC_BIT	24
#define	BCH_INTS_ERRC_MASK	(0x7f << BCH_INTS_ERRC_BIT)
#define BCH_INTS_BPSO		(1 << 23)
#define BCH_INTS_TERRC_BIT	16
#define BCH_INTS_TERRC_MASK	(0x7f << BCH_INTS_TERRC_BIT)
#define	BCH_INTS_SDMF		(1 << 5)
#define	BCH_INTS_ALLf		(1 << 4)
#define	BCH_INTS_DECF		(1 << 3)
#define	BCH_INTS_ENCF		(1 << 2)
#define	BCH_INTS_UNCOR		(1 << 1)
#define	BCH_INTS_ERR		(1 << 0)

/* BCH Interrupt Enable Register */
#define BCH_INTE_SDMFE		(1 << 5)
#define BCH_INTE_ALL_FE		(1 << 4)
#define BCH_INTE_DECFE		(1 << 3)
#define BCH_INTE_ENCFE		(1 << 2)
#define BCH_INTE_UNCORE		(1 << 1)
#define BCH_INTE_ERRE		(1 << 0)

/* BCH Interrupt Enable Set Register (BHINTES) */
#define BCH_INTES_SDMFES	(1 << 5)
#define BCH_INTES_ALL_FES	(1 << 4)
#define BCH_INTES_DECFES	(1 << 3)
#define BCH_INTES_ENCFES	(1 << 2)
#define BCH_INTES_UNCORES	(1 << 1)
#define BCH_INTES_ERRES		(1 << 0)

/* BCH Interrupt Enable Clear Register (BHINTEC) */
#define BCH_INTEC_SDMFES	(1 << 5)
#define BCH_INTEC_ALL_FEC	(1 << 4)
#define BCH_INTEC_DECFEC	(1 << 3)
#define BCH_INTEC_ENCFEC	(1 << 2)
#define BCH_INTEC_UNCOREC	(1 << 1)
#define BCH_INTEC_ERREC		(1 << 0)

/* BCH User TAG OUTPUT Register */
#define BCH_BHTO_TAGO_BIT	0
#define BCH_BHTO_TAGO_MASK	(0xffff << BCH_TAGO_MASK)
#endif /* __JZBCH_H__ */
