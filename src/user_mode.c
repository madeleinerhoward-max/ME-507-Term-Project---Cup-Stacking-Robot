/**
 * @file    user_mode.c
 * @brief   Interactive teleop loop implementation (see user_mode.h).
 * @addtogroup user_mode
 * @{
 */

#include "user_mode.h"
#include "stm32f4xx_hal.h"

/** @brief Vacuum ramp (defined in gantry_test.c). */
extern void vacuum_ramp(uint32_t from_pct, uint32_t to_pct);
#define VAC_RUN_PCT 30u   /**< Vacuum run duty (%) for the toggle. */

/**
 * @name Travel limits and step size
 * Soft limits in mm from home (0). Tighten if real travel is shorter, or a
 * held key reaches the mechanical end before the clamp engages.
 * @{
 */
#define LIM_X_MM   500.0f  /**< X soft limit (mm).        */
#define LIM_Y_MM   500.0f  /**< Y soft limit (mm).        */
#define LIM_Z_MM   500.0f  /**< Z soft limit (mm).        */
#define STEP_MM    5.0f    /**< Jog distance per keypress (mm). */
/** @} */

/**
 * @name RX ring buffer
 * Filled by the USART2 ISR, drained by the loop / menu.
 * @{
 */
#define RXBUF_SZ 64u                    /**< Ring-buffer capacity (bytes). */
static volatile uint8_t  rxbuf[RXBUF_SZ];/**< Storage.                     */
static volatile uint16_t rx_head = 0;   /**< Write index (ISR).           */
static volatile uint16_t rx_tail = 0;   /**< Read index (loop).           */
/** @} */

void user_mode_rx_push(uint8_t byte)
{
    uint16_t next = (uint16_t)((rx_head + 1u) % RXBUF_SZ);
    if (next != rx_tail) {          /* drop on overflow rather than clobber */
        rxbuf[rx_head] = byte;
        rx_head = next;
    }
}

/**
 * @brief  Pop one byte from the ring buffer.
 * @param  out  Destination for the byte.
 * @retval 1  A byte was returned.
 * @retval 0  Buffer empty.
 */
static int rx_pop(uint8_t *out)
{
    if (rx_tail == rx_head) return 0;   /* empty */
    *out = rxbuf[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) % RXBUF_SZ);
    return 1;
}

int user_mode_pop_key(uint8_t *out)
{
    return rx_pop(out);
}

/** @brief Jog state: per-axis commanded positions and vacuum flag. */
static float pos_x_mm = 0.0f;  /**< Commanded X position (mm). */
static float pos_y_mm = 0.0f;  /**< Commanded Y position (mm). */
static float pos_z_mm = 0.0f;  /**< Commanded Z position (mm). */
static uint8_t vac_on = 0;     /**< Vacuum toggle state.       */

/**
 * @brief  Clamp a float to a range.
 * @param  v   Value.
 * @param  lo  Lower bound.
 * @param  hi  Upper bound.
 * @return The clamped value.
 */
static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/**
 * @brief  Apply a delta to one axis, clamp to [0, limit], retarget (non-blocking).
 * @param  d       Driver instance.
 * @param  pos_mm  Pointer to that axis's commanded position (mm); updated in place.
 * @param  delta   Signed step (mm).
 * @param  limit   Upper soft limit (mm).
 */
static void jog(TMC5160_t *d, float *pos_mm, float delta, float limit)
{
    *pos_mm = clampf(*pos_mm + delta, 0.0f, limit);
    tmc5160_move_to(d, (int32_t)(*pos_mm * (float)d->steps_per_mm));
}

void user_mode_run(TMC5160_t *x, TMC5160_t *y, TMC5160_t *z)
{
    /* Axes are at home (0) coming out of boot. */
    pos_x_mm = pos_y_mm = pos_z_mm = 0.0f;
    vac_on = 0;

    for (;;) {
        uint8_t k;
        while (rx_pop(&k)) {
            switch (k) {
                case 'w': case 'W':  jog(y, &pos_y_mm, +STEP_MM, LIM_Y_MM); break;
                case 's': case 'S':  jog(y, &pos_y_mm, -STEP_MM, LIM_Y_MM); break;
                case 'd': case 'D':  jog(x, &pos_x_mm, +STEP_MM, LIM_X_MM); break;
                case 'a': case 'A':  jog(x, &pos_x_mm, -STEP_MM, LIM_X_MM); break;
                case 'l': case 'L':  jog(z, &pos_z_mm, +STEP_MM, LIM_Z_MM); break;
                case 'k': case 'K':  jog(z, &pos_z_mm, -STEP_MM, LIM_Z_MM); break;
                case ' ':
                    if (vac_on) { vacuum_ramp(VAC_RUN_PCT, 0); vac_on = 0; }
                    else        { vacuum_ramp(0, VAC_RUN_PCT); vac_on = 1; }
                    break;
                default: break;   /* ignore anything else */
            }
        }
        /* nothing queued -- let the TMC ramp generators run the moves */
    }
}

/** @} */
