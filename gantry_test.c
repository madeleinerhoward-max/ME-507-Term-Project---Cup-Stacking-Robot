/**
 * @file    gantry_test.c
 * @brief   Boot, diagnostics, and run-mode dispatch for the cup-stacking gantry.
 *
 * @details
 * On boot: checks SPI comms to the three drivers, homes X and Y into their REFL
 * switches, and takes the current Z position as zero (crank the bed to its
 * bottom before power-on). It then presents a menu over USART2 (reached through
 * the Black Pill USB-UART bridge):
 * - @c 'u' : enter @ref user_mode (WASD/KL jog, space toggles vacuum); never returns.
 * - @c '2' : run one automatic pick-and-stack, then return to the menu.
 *
 * @par Vacuum
 * HW-039 / BTS7960 driven by PWM on PA8 = TIM1_CH1. R_EN/L_EN are tied high and
 * LPWM grounded in hardware; duty is clamped to a hard ceiling and soft-ramped.
 *
 * @par Diagnostics
 * @ref g_test carries @c reset_cause and the pre-home REFL snapshots
 * (@c x_stopL_prehome / @c y_stopL_prehome) for field debugging.
 *
 * @defgroup gantry Gantry application
 * @{
 */

#include "gantry_test.h"
#include "tmc5160.h"
#include "user_mode.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include <string.h>

#define GANTRY_TEST_UART  0   /**< Set 1 if the bridge forwards TX to PuTTY (enables text output). */

/**
 * @name Pick-and-stack coordinates
 * Edit these. All positions are in mm from home (0,0,0). Z is the bed:
 * 0 = bottom (the cranked datum), positive raises the bed. The suction cup is
 * at a fixed height on the X carriage; the bed lifts a cup up to it (vacuum on),
 * lowers leaving the cup held, traverses, lifts the held cup onto the target
 * cup, releases (vacuum off), and lowers.
 * @{
 */
#define PICKUP_X_MM    100.0f   /**< X of the cup to grab.                 */
#define PICKUP_Y_MM    100.0f   /**< Y of the cup to grab.                 */
#define STACK_X_MM     200.0f   /**< X of the target/base cup.            */
#define STACK_Y_MM     100.0f   /**< Y of the target/base cup.            */
#define Z_CLEAR_MM       0.0f   /**< Bed-down height; safe to traverse X/Y. */
#define Z_PICKUP_MM     60.0f   /**< Bed-up height where a cup meets the suction cup. */
#define Z_STACK_MM      45.0f   /**< Bed-up height to set the held cup onto the base cup. */
#define PICK_DWELL_MS  600u     /**< Vacuum grab/release settle time (ms). */
/** @} */

/**
 * @name Vacuum PWM (HW-039 RPWM on PA8 = TIM1_CH1)
 * @{
 */
#define VAC_TIM            htim1          /**< Timer handle for the vacuum PWM. */
#define VAC_CH             TIM_CHANNEL_1  /**< PWM channel.                     */
#define VAC_ARR            49u            /**< Counter period; must match CubeMX. */
#define VAC_DUTY_MAX_PCT   45u            /**< Hard duty ceiling (%).           */
#define VAC_DUTY_RUN_PCT   40u            /**< Run duty (%).                    */
#define VAC_RAMP_MS        400u           /**< Soft-ramp duration (ms).         */
#define VAC_RAMP_STEPS     20u            /**< Ramp granularity (steps).        */
/** @} */

extern TIM_HandleTypeDef htim1;

/**
 * @brief  Set the vacuum PWM duty, clamped to @ref VAC_DUTY_MAX_PCT.
 * @param  pct  Requested duty (%).
 */
static void vacuum_set_pct(uint32_t pct)
{
    if (pct > VAC_DUTY_MAX_PCT) pct = VAC_DUTY_MAX_PCT;
    __HAL_TIM_SET_COMPARE(&VAC_TIM, VAC_CH, (pct * (VAC_ARR + 1u)) / 100u);
}

/**
 * @brief  Ramp the vacuum duty between two levels over @ref VAC_RAMP_MS.
 * @param  from_pct  Starting duty (%).
 * @param  to_pct    Ending duty (%).
 * @note   Also used by user_mode.c (non-static).
 */
void vacuum_ramp(uint32_t from_pct, uint32_t to_pct)
{
    for (uint32_t s = 0; s <= VAC_RAMP_STEPS; s++) {
        int32_t pct = (int32_t)from_pct
                    + ((int32_t)(to_pct - from_pct) * (int32_t)s) / (int32_t)VAC_RAMP_STEPS;
        vacuum_set_pct((uint32_t)pct);
        HAL_Delay(VAC_RAMP_MS / VAC_RAMP_STEPS);
    }
    vacuum_set_pct(to_pct);
}

/**
 * @brief  Start the vacuum PWM at 0%% duty (pump off, output enabled).
 */
static void vacuum_init(void)
{
    vacuum_set_pct(0);
    HAL_TIM_PWM_Start(&VAC_TIM, VAC_CH);
}

/* ---- CubeMX handles + UART RX interrupt -------------------------------- */
extern SPI_HandleTypeDef hspi1;   /**< SPI1 handle (driver bus).  */
extern UART_HandleTypeDef huart2; /**< USART2 handle (teleop).    */

uint8_t uart_rx_byte;             /**< Single-byte interrupt RX target. */

/**
 * @brief  USART2 RX-complete callback: buffer the byte and re-arm reception.
 * @param  huart  HAL UART handle that completed.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        user_mode_rx_push(uart_rx_byte);
        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    }
}

#if GANTRY_TEST_UART
#include <stdio.h>
#include <stdarg.h>
/**
 * @brief  printf-style helper that transmits over USART2 (compiled out unless
 *         @ref GANTRY_TEST_UART is 1).
 * @param  fmt  Format string.
 */
static void up(const char *fmt, ...) {
    char buf[128];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)n, 100);
}
#else
static inline void up(const char *fmt, ...) { (void)fmt; }
#endif

/** @brief Global status/diagnostics block; watch in Live Expressions. */
volatile gantry_test_result_t g_test;

/** @brief Driver instances for the X, Y, and Z axes. */
static TMC5160_t drv_x = { &hspi1, GPIOA, GPIO_PIN_4,  80,  1, 16, 8, "X" };
static TMC5160_t drv_y = { &hspi1, GPIOB, GPIO_PIN_10, 80,  0, 16, 8, "Y" };
static TMC5160_t drv_z = { &hspi1, GPIOC, GPIO_PIN_12, 400, 1, 16, 8, "Z" };

#define DWELL_MS  800

/**
 * @brief  Blocking absolute move in millimetres.
 * @param  d   Driver instance.
 * @param  mm  Target position (mm).
 */
static void go_mm(TMC5160_t *d, float mm)
{
    tmc5160_move_to_blocking(d, (int32_t)(mm * (float)d->steps_per_mm));
}

/**
 * @brief  Read an axis's REFL stop bit before any motion (diagnostic).
 * @param  d  Driver instance.
 * @retval 1  STOP_L active (switch line reads pressed).
 * @retval 0  STOP_L inactive.
 */
static uint8_t refl_prehome(TMC5160_t *d)
{
    uint32_t rs = 0;
    tmc5160_read(d, TMC_REG_RAMP_STAT, &rs);
    return (rs & TMC_RAMPSTAT_STATUS_STOP_L) ? 1u : 0u;
}

/**
 * @brief  Read and clear the RCC reset flags to identify the last reset.
 * @return 'O' power-on, 'B' brown-out, 'P' pin, 'S' software, 'W' watchdog,
 *         '?' unknown.
 */
static uint8_t capture_reset_cause(void)
{
    uint8_t c = '?';
    if      (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))  c = 'B';   /* brown-out */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))  c = 'P';   /* reset pin */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))  c = 'S';   /* software  */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) c = 'W';   /* watchdog  */
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))  c = 'O';   /* power-on  */
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return c;
}

/**
 * @brief  Execute one automatic pick-and-stack using the configured coordinates.
 * @details Traverses with the bed clear, lifts a cup onto the suction cup,
 *          carries it to the stack location, places it on the base cup, and
 *          parks at home. Blocking.
 */
static void pick_and_stack(void)
{
    g_test.mode = 'P';
    up("[pick] start\r\n");

    go_mm(&drv_z, Z_CLEAR_MM);              /* bed down, safe to traverse */

    go_mm(&drv_x, PICKUP_X_MM);             /* over the cup to grab */
    go_mm(&drv_y, PICKUP_Y_MM);
    go_mm(&drv_z, Z_PICKUP_MM);             /* lift cup up to the suction cup */
    vacuum_ramp(0, VAC_DUTY_RUN_PCT);       /* grab */
    HAL_Delay(PICK_DWELL_MS);
    go_mm(&drv_z, Z_CLEAR_MM);              /* lower bed; cup stays on suction */

    go_mm(&drv_x, STACK_X_MM);              /* over the base cup */
    go_mm(&drv_y, STACK_Y_MM);
    go_mm(&drv_z, Z_STACK_MM);              /* set held cup onto base cup */
    vacuum_ramp(VAC_DUTY_RUN_PCT, 0);       /* release */
    HAL_Delay(PICK_DWELL_MS);
    go_mm(&drv_z, Z_CLEAR_MM);              /* lower bed */

    go_mm(&drv_x, 0.0f);                    /* park at home */
    go_mm(&drv_y, 0.0f);

    g_test.pos_x = tmc5160_get_position(&drv_x);
    g_test.pos_y = tmc5160_get_position(&drv_y);
    g_test.pos_z = tmc5160_get_position(&drv_z);
    up("[pick] done\r\n");
    g_test.mode = 'M';
}

/**
 * @brief  Block until the operator selects a run mode.
 * @retval 'u'  User mode requested.
 * @retval '2'  Pick-and-stack requested.
 */
static uint8_t wait_for_choice(void)
{
    up("\r\nMenu: 'u' = user mode, '2' = pick & stack\r\n");
    for (;;) {
        uint8_t k;
        if (user_mode_pop_key(&k)) {
            if (k == 'u' || k == 'U') return 'u';
            if (k == '2')             return '2';
        }
        HAL_Delay(5);
    }
}

/**
 * @brief  Application entry point called once from main(): boot, then dispatch.
 * @details Captures the reset cause, initialises the vacuum, checks comms,
 *          homes X/Y, zeroes Z, then loops on the menu. Does not return.
 */
void gantry_test_run(void)
{
    TMC5160_t *all[3] = { &drv_x, &drv_y, &drv_z };
    uint8_t rc = capture_reset_cause();   /* before memset wipes it */

    vacuum_init();

    memset((void *)&g_test, 0, sizeof(g_test));
    g_test.reset_cause = rc;
    g_test.home_x_ok = 0xFF;
    g_test.home_y_ok = 0xFF;

    HAL_Delay(50);
    up("\r\n=== Gantry boot (reset=%c) ===\r\n", rc);

    /* Stage 1: comms */
    g_test.stage = 1;
    for (int i = 0; i < 3; i++) {
        uint32_t ioin = 0;
        tmc5160_read(all[i], TMC_REG_IOIN, &ioin);
        uint8_t v = (uint8_t)(ioin >> TMC_IOIN_VERSION_SHIFT);
        g_test.comms_version[i] = v;
        g_test.comms_ok[i] = (v == TMC_IOIN_VERSION_TMC5160) ? 1 : 0;
    }

    /* Stage 2: init (zeros Z at the cranked-bottom datum) */
    g_test.stage = 2;
    for (int i = 0; i < 3; i++)
        if (g_test.comms_ok[i]) tmc5160_init(all[i]);

    /* Pre-home REFL snapshot -- the key Y diagnostic. With carriages OFF the
     * switches these should read 0. If Y reads 1 here, its switch line is
     * active before any motion (hardware), which is why Y homing quits early. */
    if (g_test.comms_ok[0]) g_test.x_stopL_prehome = refl_prehome(&drv_x);
    if (g_test.comms_ok[1]) g_test.y_stopL_prehome = refl_prehome(&drv_y);

    /* Stage 3: home X */
    g_test.stage = 3;
    if (g_test.comms_ok[0]) {
        g_test.home_x_ok = (tmc5160_home(&drv_x) == TMC_OK) ? 1 : 0;
        g_test.pos_x = tmc5160_get_position(&drv_x);
    }
    HAL_Delay(DWELL_MS);

    /* Stage 4: home Y */
    g_test.stage = 4;
    if (g_test.comms_ok[1]) {
        g_test.home_y_ok = (tmc5160_home(&drv_y) == TMC_OK) ? 1 : 0;
        g_test.pos_y = tmc5160_get_position(&drv_y);
    }
    HAL_Delay(DWELL_MS);

    /* Stage 5: Z datum already set by init */
    g_test.stage = 5;
    if (g_test.comms_ok[2]) g_test.pos_z = tmc5160_get_position(&drv_z);

    /* Stage 6: arm UART, then loop on the menu */
    g_test.stage = 6;
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);

    for (;;) {
        g_test.mode = 'M';
        uint8_t choice = wait_for_choice();
        if (choice == 'u') {
            g_test.mode = 'U';
            user_mode_run(&drv_x, &drv_y, &drv_z);   /* never returns */
        } else if (choice == '2') {
            pick_and_stack();                        /* returns to menu */
        }
    }
}

/** @} */
