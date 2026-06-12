/**
 * @file    user_mode.h
 * @brief   Interactive teleop over USART2: keyboard jog and vacuum control.
 *
 * @details
 * Keystrokes arrive on USART2 (115200 8N1) from a Black Pill USB-UART bridge
 * running a PuTTY session on the PC. Jogs are non-blocking: each keypress
 * updates a commanded position and the TMC ramp generator runs the motion.
 *
 * @par Controls
 * - W / S : Y axis + / -   (top rail)
 * - A / D : X axis - / +   (suction-cup carriage)
 * - K / L : Z bed down / up
 * - space : toggle the vacuum (soft-ramped)
 *
 * @defgroup user_mode User mode (teleop)
 * @{
 */
#ifndef USER_MODE_H
#define USER_MODE_H

#include "tmc5160.h"

/**
 * @brief   Run the interactive teleop loop. Does not return.
 * @details Drains received keystrokes and jogs the axes / toggles the vacuum.
 * @param   x  X-axis driver instance.
 * @param   y  Y-axis driver instance.
 * @param   z  Z-axis driver instance.
 */
void user_mode_run(TMC5160_t *x, TMC5160_t *y, TMC5160_t *z);

/**
 * @brief   Push one received byte into the RX ring buffer.
 * @details Call from the USART2 RX-complete interrupt context.
 * @param   byte  The received byte.
 */
void user_mode_rx_push(uint8_t byte);

/**
 * @brief   Pop one key from the RX ring buffer (non-blocking).
 * @details Shared with the boot menu so it can read its selection.
 * @param   out  Destination for the byte if one is available.
 * @retval  1    A key was returned in @p out.
 * @retval  0    The buffer was empty.
 */
int user_mode_pop_key(uint8_t *out);

#endif /* USER_MODE_H */

/** @} */
