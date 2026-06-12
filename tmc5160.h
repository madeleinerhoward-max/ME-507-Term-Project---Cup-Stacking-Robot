/**
 * @file    tmc5160.h
 * @brief   TMC5160 stepper-driver SPI interface: register access, init,
 *          homing, and positioning moves.
 *
 * @details
 * Targets a custom STM32F411RET6 PCB with up to four TMC5160A drivers sharing
 * SPI1. Each driver is addressed by a software-toggled, active-low chip select.
 *
 * @par SPI configuration
 * - PA5 = SCK, PA6 = MISO (TMC SDO), PA7 = MOSI (TMC SDI)
 * - Mode 3 (CPOL = 1, CPHA = 1), ~781 kHz, NSS = software
 *
 * @par Chip-select map
 * - X  (MPC0) : PA4
 * - Y  (MPC1) : PB10
 * - Z  (MPC2) : PC12  (both Z motors wired in parallel on this one driver)
 * - (MPC3 / PD2 is unused in the current build)
 *
 * @par Endstops
 * X and Y limit switches connect to their driver's REFL pin (pulled high,
 * active-low); the stop is handled driver-side. Z has no endstop.
 *
 * @par Velocity units
 * The CLK pin is unconnected, so the internal oscillator (~12 MHz) sets the
 * timebase: microsteps/s = VMAX * fCLK / 2^24, i.e. VMAX = usteps/s * 1.398,
 * or VMAX = (mm/s) * steps_per_mm * 1.398.
 *
 * @defgroup tmc5160 TMC5160 driver
 * @{
 */

#ifndef TMC5160_H
#define TMC5160_H

#include "stm32f4xx_hal.h"
#include "tmc5160_regs.h"
#include <stdint.h>

/**
 * @name Tunable defaults
 * Reference values for bringup. Per-driver current is set in @ref TMC5160_t.
 * @{
 */
#define TMC_IRUN        16            /**< Reference run current (0-31).        */
#define TMC_IHOLD       8             /**< Reference hold current (0-31).       */
#define TMC_IHOLDDELAY  6             /**< Run-to-hold current ramp delay.      */

#define TMC_MRES        TMC_MRES_16   /**< Microstep resolution (1/16).         */

#define TMC_VMAX_HOME    2500u        /**< Homing speed (~22 mm/s on X/Y).      */
#define TMC_VMAX_TRAVEL  6000u        /**< Travel speed (~54 mm/s on X/Y).      */
#define TMC_AMAX         1000u        /**< Acceleration.                        */
#define TMC_DMAX         1000u        /**< Deceleration.                        */

#define TMC_HOME_BACKOFF_MM   3       /**< Back-off distance after the switch trips. */
#define TMC_STOP_DEBOUNCE     8       /**< Consecutive active REFL reads required to
                                           accept a stop (rejects noise glitches). */
#define TMC_SPI_TIMEOUT_MS    10      /**< Per-transfer SPI timeout (ms).       */
#define TMC_HOME_TIMEOUT_MS   15000   /**< Abort homing if the switch never trips. */
#define TMC_MOVE_TIMEOUT_MS   20000   /**< Abort a positioning move that never ends. */
/** @} */

/**
 * @brief One TMC5160 instance on the shared SPI bus.
 *
 * @note For the Z axis, two motors are wired in parallel on a single driver.
 *       @c irun / @c ihold set the current the driver regulates through its
 *       sense resistor; that current splits between the two parallel motors,
 *       so each carries half. To run each Z motor at half an X/Y motor's
 *       current, give the Z driver the same @c irun as X/Y.
 */
typedef struct {
    SPI_HandleTypeDef *hspi;          /**< HAL SPI handle (e.g. &hspi1).        */
    GPIO_TypeDef      *cs_port;       /**< Chip-select GPIO port.               */
    uint16_t           cs_pin;        /**< Chip-select GPIO pin.                */
    int32_t            steps_per_mm;  /**< 80 for X/Y belts, 400 for the Z leadscrew. */
    uint8_t            invert;        /**< Non-zero sets GCONF.shaft (reverse direction). */
    uint8_t            irun;          /**< Run current 0-31 (driver total).     */
    uint8_t            ihold;         /**< Hold current 0-31 (driver total).    */
    const char        *name;          /**< Short label for debugging.           */
} TMC5160_t;

/**
 * @brief Result/status codes returned by the driver.
 */
typedef enum {
    TMC_OK = 0,     /**< Operation completed successfully.        */
    TMC_TIMEOUT,    /**< Operation timed out (switch/move).       */
    TMC_NO_COMMS    /**< Chip did not respond with a valid VERSION. */
} tmc_result_t;

/**
 * @brief   Write a 32-bit value to a TMC5160 register.
 * @param   drv  Driver instance.
 * @param   reg  Register address (see tmc5160_regs.h).
 * @param   val  Value to write.
 * @return  The SPI status byte returned during the transfer.
 */
uint8_t tmc5160_write(TMC5160_t *drv, uint8_t reg, uint32_t val);

/**
 * @brief   Read a 32-bit register value.
 * @details Performs two SPI transfers; the value is valid on the second
 *          (the TMC5160 returns the previous datagram's data).
 * @param   drv  Driver instance.
 * @param   reg  Register address.
 * @param   out  Destination for the 32-bit value (may be NULL).
 * @return  The SPI status byte from the second transfer.
 */
uint8_t tmc5160_read(TMC5160_t *drv, uint8_t reg, uint32_t *out);

/**
 * @brief   Verify SPI communication by checking the IOIN VERSION byte.
 * @param   drv  Driver instance.
 * @retval  TMC_OK        VERSION read as 0x30 (chip is responding).
 * @retval  TMC_NO_COMMS  Otherwise.
 */
tmc_result_t tmc5160_test_comms(TMC5160_t *drv);

/**
 * @brief   Initialise a driver for SpreadCycle positioning mode.
 * @details Clears faults, configures the chopper, applies @c irun / @c ihold,
 *          loads conservative ramp parameters, selects positioning mode with
 *          stops disabled, and zeroes XACTUAL/XTARGET.
 * @param   drv  Driver instance.
 */
void tmc5160_init(TMC5160_t *drv);

/**
 * @brief   Start a non-blocking absolute move (microsteps).
 * @param   drv    Driver instance.
 * @param   steps  Signed target position in microsteps.
 */
void tmc5160_move_to(TMC5160_t *drv, int32_t steps);

/**
 * @brief   Start a non-blocking absolute move (millimetres).
 * @param   drv  Driver instance.
 * @param   mm   Signed target position in millimetres.
 */
void tmc5160_move_mm(TMC5160_t *drv, float mm);

/**
 * @brief   Test whether the last positioning move has completed.
 * @param   drv  Driver instance.
 * @retval  1    Target reached and velocity is zero.
 * @retval  0    Move still in progress.
 */
uint8_t tmc5160_move_done(TMC5160_t *drv);

/**
 * @brief   Absolute move that blocks until completion or timeout.
 * @param   drv    Driver instance.
 * @param   steps  Signed target position in microsteps.
 * @retval  TMC_OK       Target reached.
 * @retval  TMC_TIMEOUT  Did not complete within @ref TMC_MOVE_TIMEOUT_MS.
 */
tmc_result_t tmc5160_move_to_blocking(TMC5160_t *drv, int32_t steps);

/**
 * @brief   Home an axis into its REFL endstop and set machine zero.
 * @details Clears stale stop state, drives in the negative direction, and
 *          accepts the stop only after it is active for @ref TMC_STOP_DEBOUNCE
 *          consecutive reads (rejecting noise glitches). Then zeroes at the
 *          switch, backs off @ref TMC_HOME_BACKOFF_MM, and re-zeroes.
 * @param   drv  Driver instance. Do not call on Z (no endstop).
 * @retval  TMC_OK       Homed successfully.
 * @retval  TMC_TIMEOUT  Switch never tripped within @ref TMC_HOME_TIMEOUT_MS.
 */
tmc_result_t tmc5160_home(TMC5160_t *drv);

/**
 * @brief   Read the current position (XACTUAL).
 * @param   drv  Driver instance.
 * @return  Position in signed microsteps.
 */
int32_t tmc5160_get_position(TMC5160_t *drv);

#endif /* TMC5160_H */

/** @} */
