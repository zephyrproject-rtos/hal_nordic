/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * The pad this sample drives and the VIO index that carries it -- shared by the
 * two sides that have to agree on both: the App (main.c) and the FLPR stub
 * (flpr_stub.S), which the build assembles for the VPR.
 *
 * Assembler-safe on purpose: plain integer defines, no C, no U/UL suffixes, so
 * flpr_stub.S can include it through the C preprocessor.
 *
 * Both facts are per part. The FLPR's real-time pin interface does not name a
 * physical pin: it drives a VIO index, and which pin that lands on differs
 * between parts -- the nRF54LV10A has no P2 at all, only P0 and P1. Take both
 * numbers from your own part's pin assignment rather than copying them.
 *
 * nRF54LM20A/B: P2.05 is VIO 5, and free on the DK (spi00 uses P2.01/02/04).
 * nRF54L15:     the same pair -- l15's VIO map is identical to nRF54LM20's, and
 *               P2.05 is free on the PCA10156 too (spi00 uses P2.01/02/04, the
 *               LEDs P2.07/P2.09).
 * nRF54LV10A:   P1.15 is VIO 4, the first pad of that part's VIO range.
 * The nRF54LV10A pair has not been run on an nRF54LV10 DK.
 */

#ifndef VPR_PAD_H
#define VPR_PAD_H

#if defined(NRF54LM20A_XXAA) || defined(NRF54LM20B_XXAA) || defined(NRF54L15_XXAA)
#define PAD_PORT 2
#define PAD_PIN  5
#define PAD_VIO  5
#elif defined(NRF54LV10A_XXAA)
#define PAD_PORT 1
#define PAD_PIN  15
#define PAD_VIO  4
#else
#error "pick the pad and its VIO index for this part"
#endif

#endif /* VPR_PAD_H */
