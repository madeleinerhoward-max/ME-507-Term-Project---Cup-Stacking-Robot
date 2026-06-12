/**
 * @file    gantry_test.h
 * @brief   Boot/dispatch entry point and the shared status block (g_test).
 * @ingroup gantry
 */
#ifndef GANTRY_TEST_H
#define GANTRY_TEST_H

#include <stdint.h>

/**
 * @brief Global status and diagnostics, updated through boot and run modes.
 * @details Watch this in STM32CubeIDE Live Expressions during a debug session.
 */
typedef struct {
    volatile uint8_t  stage;            /**< Boot progress 1..6; 6 = at menu/running. */
    volatile uint8_t  reset_cause;      /**< Last reset: 'O','B','P','S','W','?' (see capture_reset_cause). */
    volatile uint8_t  comms_version[3]; /**< IOIN VERSION per driver [X,Y,Z]; expect 0x30. */
    volatile uint8_t  comms_ok[3];      /**< 1 if that driver answered with 0x30.   */
    volatile uint8_t  x_stopL_prehome;  /**< X REFL stop bit before homing (expect 0 off switch). */
    volatile uint8_t  y_stopL_prehome;  /**< Y REFL stop bit before homing (expect 0 off switch). */
    volatile uint8_t  home_x_ok;        /**< 1 homed, 0 timeout, 0xFF not run.      */
    volatile uint8_t  home_y_ok;        /**< 1 homed, 0 timeout, 0xFF not run.      */
    volatile uint8_t  mode;             /**< 'M' menu, 'U' user mode, 'P' pick-and-stack. */
    volatile int32_t  pos_x;            /**< Last captured X position (microsteps).  */
    volatile int32_t  pos_y;            /**< Last captured Y position (microsteps).  */
    volatile int32_t  pos_z;            /**< Last captured Z position (microsteps).  */
} gantry_test_result_t;

/** @brief The global status block defined in gantry_test.c. */
extern volatile gantry_test_result_t g_test;

/**
 * @brief Application entry point: boot the gantry, then dispatch run modes.
 * @details Call once from main(); does not return.
 */
void gantry_test_run(void);

#endif /* GANTRY_TEST_H */
