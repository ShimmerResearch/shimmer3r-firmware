/*
 * hal_PowerProfile.h
 *
 * The Shimmer3R power-profile run (DEV-1335): a fixed sequence a production
 * test arms over the dock and the unit runs at its next boot, on battery or
 * bench supply with no cable attached, so a power analyser can capture what
 * each part draws. The record, schedule and marker formats are in
 * log-and-stream-common Test/shimmer_factory_record.h; the sequence is
 * documented in docs/SHIMMER3R_FACTORY_TEST_REPORT.md section 6.
 */

#ifndef HAL_POWERPROFILE_H_
#define HAL_POWERPROFILE_H_

#include <stdint.h>

#include "Test/shimmer_factory_record.h"

/* Bumped whenever the steps, their order or their durations change */
#define PP_SCHEDULE_VER         1U
/* Return-to-baseline window after every step */
#define PP_BASELINE_WINDOW_MS   1000U

#define PP_STEP_MS_BASELINE     5000U
#define PP_STEP_MS_SENSING_RAIL 3000U
#define PP_STEP_MS_SENSOR       2000U
#define PP_STEP_MS_ADS7028_GSR  3000U
#define PP_STEP_MS_EXP_POWER    3000U
#define PP_STEP_MS_MICROPHONE   2000U
#define PP_STEP_MS_SD           4000U
#define PP_STEP_MS_LED          1000U
#define PP_STEP_MS_BLUETOOTH    8000U

/* The part probe (docs/SHIMMER3R_FACTORY_TEST_REPORT.md section 6.5): which
 * of the SPI parts whose fitting varies by board answer, as PP_PART_* bits.
 * It reads ID registers only, with bounded SPI timeouts, and never reaches
 * Error_Handler() on a part that does not answer. */

/* The parts the SR number says this board carries */
uint16_t PartsProbe_expected(void);
/* Probes the parts. Needs the sensing rail on and SPI1 initialised (which
 * identifies the pressure sensor); initialises SPI2 and leaves it up. Where
 * the SR number says ExG it brings the ADS1292R and SPI3 up and down again,
 * which takes about 1 s. */
uint16_t PartsProbe_run(void);
/* Called once from Init(), before the configuration is loaded: powers the
 * sensing rail, identifies the pressure sensor, runs the probe for the boot
 * report, and powers the rail off again */
void PartsProbe_atBoot(void);

/* Called once from Init(), after the configuration is loaded and before
 * booting ends. Consumes an armed record: runs the sequence if the unit is
 * neither docked nor on USB, otherwise marks it SKIPPED. A record found
 * RUNNING - a run cut short by a reset or power loss - is marked ABORTED and
 * never re-run. Does nothing when no record is armed. */
void PowerProfile_runIfArmed(void);

#endif /* HAL_POWERPROFILE_H_ */
