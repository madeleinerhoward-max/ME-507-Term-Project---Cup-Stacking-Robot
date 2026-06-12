/**
 * @file    tmc5160.c
 * @brief   TMC5160 SPI driver implementation (see tmc5160.h for the API).
 *
 * @details
 * SPI datagram (datasheet section 5): 40 bits, MSB first, Mode 3.
 * - TX byte 0  = address; bit 7 = 1 for write, 0 for read.
 * - TX bytes 1..4 = data, MSB first.
 * - RX byte 0  = SPI status.
 * - RX bytes 1..4 = data from the PREVIOUS datagram, so a read needs two
 *   transfers and the value lands on the second.
 *
 * @addtogroup tmc5160
 * @{
 */

#include "tmc5160.h"

/** @brief Assert (drive low) the driver's active-low chip select. */
static inline void cs_low (TMC5160_t *d){ HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_RESET); }
/** @brief De-assert (drive high) the driver's chip select. */
static inline void cs_high(TMC5160_t *d){ HAL_GPIO_WritePin(d->cs_port, d->cs_pin, GPIO_PIN_SET);   }

/**
 * @brief  Perform one 40-bit (5-byte) full-duplex SPI transfer with CS framing.
 * @param  d   Driver instance.
 * @param  tx  5-byte transmit buffer.
 * @param  rx  5-byte receive buffer.
 * @return The SPI status byte (rx[0]).
 */
static uint8_t xfer40(TMC5160_t *d, const uint8_t *tx, uint8_t *rx)
{
    cs_low(d);
    HAL_SPI_TransmitReceive(d->hspi, (uint8_t *)tx, rx, 5, TMC_SPI_TIMEOUT_MS);
    cs_high(d);
    return rx[0];
}

/* --- Register access --- */
uint8_t tmc5160_write(TMC5160_t *d, uint8_t reg, uint32_t val)
{
    uint8_t tx[5], rx[5];
    tx[0] = (reg & 0x7Fu) | 0x80u;
    tx[1] = (uint8_t)(val >> 24);
    tx[2] = (uint8_t)(val >> 16);
    tx[3] = (uint8_t)(val >>  8);
    tx[4] = (uint8_t)(val);
    return xfer40(d, tx, rx);
}

uint8_t tmc5160_read(TMC5160_t *d, uint8_t reg, uint32_t *out)
{
    uint8_t tx[5] = {0}, rx[5] = {0};
    tx[0] = reg & 0x7Fu;
    xfer40(d, tx, rx);                  /* request */
    uint8_t status = xfer40(d, tx, rx); /* clock out the value */
    if (out)
        *out = ((uint32_t)rx[1] << 24) | ((uint32_t)rx[2] << 16)
             | ((uint32_t)rx[3] <<  8) |  (uint32_t)rx[4];
    return status;
}


tmc_result_t tmc5160_test_comms(TMC5160_t *d)
{
    uint32_t ioin = 0;
    tmc5160_read(d, TMC_REG_IOIN, &ioin);
    uint8_t version = (uint8_t)(ioin >> TMC_IOIN_VERSION_SHIFT);
    return (version == TMC_IOIN_VERSION_TMC5160) ? TMC_OK : TMC_NO_COMMS;
}


void tmc5160_init(TMC5160_t *d)
{
    /* Clear latched faults */
    tmc5160_write(d, TMC_REG_GSTAT, TMC_GSTAT_RESET | TMC_GSTAT_DRV_ERR | TMC_GSTAT_UV_CP);

    /* GCONF: SpreadCycle, multistep filter on, optional direction invert */
    uint32_t gconf = TMC_GCONF_MULTISTEP_FILT;
    if (d->invert) gconf |= TMC_GCONF_SHAFT;
    tmc5160_write(d, TMC_REG_GCONF, gconf);

    /* CHOPCONF: TOFF=5 (outputs ON), HSTRT=4, HEND=1, TBL=2, MRES, INTPOL */
    uint32_t chop = 0;
    chop |= (5u << TMC_CHOPCONF_TOFF_SHIFT);
    chop |= (4u << TMC_CHOPCONF_HSTRT_SHIFT);
    chop |= (1u << TMC_CHOPCONF_HEND_SHIFT);
    chop |= (2u << TMC_CHOPCONF_TBL_SHIFT);
    chop |= ((uint32_t)TMC_MRES << TMC_CHOPCONF_MRES_SHIFT);
    chop |= TMC_CHOPCONF_INTPOL;
    tmc5160_write(d, TMC_REG_CHOPCONF, chop);

    /* Current (per-driver; for the parallel Z pair this is the TOTAL the
     * driver regulates, so each Z motor carries half of d->irun) */
    tmc5160_write(d, TMC_REG_IHOLD_IRUN, TMC_IHOLD_IRUN(d->ihold, d->irun, TMC_IHOLDDELAY));
    tmc5160_write(d, TMC_REG_TPOWERDOWN, 10u);

    /* Ramp params */
    tmc5160_write(d, TMC_REG_VSTART, 0u);
    tmc5160_write(d, TMC_REG_A1,     TMC_AMAX);
    tmc5160_write(d, TMC_REG_V1,     1000u);
    tmc5160_write(d, TMC_REG_AMAX,   TMC_AMAX);
    tmc5160_write(d, TMC_REG_VMAX,   TMC_VMAX_TRAVEL);
    tmc5160_write(d, TMC_REG_DMAX,   TMC_DMAX);
    tmc5160_write(d, TMC_REG_D1,     TMC_DMAX);
    tmc5160_write(d, TMC_REG_VSTOP,  10u);
    tmc5160_write(d, TMC_REG_TZEROWAIT, 100u);

    /* Positioning mode, no stops, zeroed */
    tmc5160_write(d, TMC_REG_SW_MODE,  0u);
    tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_POSITIONING);
    tmc5160_write(d, TMC_REG_XACTUAL,  0u);
    tmc5160_write(d, TMC_REG_XTARGET,  0u);
}


int32_t tmc5160_get_position(TMC5160_t *d)
{
    uint32_t x = 0;
    tmc5160_read(d, TMC_REG_XACTUAL, &x);
    return (int32_t)x;
}

void tmc5160_move_to(TMC5160_t *d, int32_t steps)
{
    tmc5160_write(d, TMC_REG_VMAX,     TMC_VMAX_TRAVEL);
    tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_POSITIONING);
    tmc5160_write(d, TMC_REG_XTARGET,  (uint32_t)steps);
}

void tmc5160_move_mm(TMC5160_t *d, float mm)
{
    tmc5160_move_to(d, (int32_t)(mm * (float)d->steps_per_mm));
}

uint8_t tmc5160_move_done(TMC5160_t *d)
{
    uint32_t rs = 0;
    tmc5160_read(d, TMC_REG_RAMP_STAT, &rs);
    return ((rs & TMC_RAMPSTAT_POSITION_REACHED) && (rs & TMC_RAMPSTAT_VZERO)) ? 1u : 0u;
}

tmc_result_t tmc5160_move_to_blocking(TMC5160_t *d, int32_t steps)
{
    tmc5160_move_to(d, steps);
    uint32_t t0 = HAL_GetTick();
    while (!tmc5160_move_done(d)) {
        if ((HAL_GetTick() - t0) > TMC_MOVE_TIMEOUT_MS) return TMC_TIMEOUT;
        HAL_Delay(1);
    }
    return TMC_OK;
}


tmc_result_t tmc5160_home(TMC5160_t *d)
{
    uint32_t rs = 0, t0;
    int stable = 0;

    /* Clean slate: stop motion, drop any stale stop config, clear latched
     * events by reading RAMP_STAT. Prevents a leftover stop from a prior home
     * or from user mode making this move quit early. */
    tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_HOLD);
    tmc5160_write(d, TMC_REG_VMAX, 0u);
    tmc5160_write(d, TMC_REG_SW_MODE, 0u);
    tmc5160_read (d, TMC_REG_RAMP_STAT, &rs);
    HAL_Delay(2);

    /* Slow homing speed */
    tmc5160_write(d, TMC_REG_VMAX, TMC_VMAX_HOME);
    tmc5160_write(d, TMC_REG_AMAX, TMC_AMAX);
    tmc5160_write(d, TMC_REG_DMAX, TMC_DMAX);

    /* Enable REFL (left) stop, active-low, soft stop */
    tmc5160_write(d, TMC_REG_SW_MODE,
                  TMC_SWMODE_STOP_L_ENABLE | TMC_SWMODE_EN_SOFTSTOP);

    /* Drive toward the endstop */
    tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_VEL_NEG);

    /* Debounced stop detection: accept STOP_L only after it stays active for
     * TMC_STOP_DEBOUNCE consecutive reads. A brief glitch releases before the
     * count is reached, and we re-command the velocity move to keep going --
     * so a noise spike on REFL no longer halts homing a few mm in. A real
     * switch press stays active and is accepted. */
    t0 = HAL_GetTick();
    for (;;) {
        if ((HAL_GetTick() - t0) > TMC_HOME_TIMEOUT_MS) {
            tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_HOLD);
            tmc5160_write(d, TMC_REG_SW_MODE, 0u);
            return TMC_TIMEOUT;
        }
        tmc5160_read(d, TMC_REG_RAMP_STAT, &rs);
        if (rs & TMC_RAMPSTAT_STATUS_STOP_L) {
            if (++stable >= TMC_STOP_DEBOUNCE) break;   /* sustained = real */
        } else if (stable > 0) {
            stable = 0;                                  /* glitch released */
            tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_VEL_NEG); /* resume */
        }
        HAL_Delay(1);
    }

    /* Zero at the switch. */
    tmc5160_write(d, TMC_REG_XACTUAL, 0u);
    tmc5160_write(d, TMC_REG_XTARGET, 0u);

    /* Disable the stop so we can move off it, then back off. */
    tmc5160_write(d, TMC_REG_SW_MODE, 0u);

    int32_t backoff = TMC_HOME_BACKOFF_MM * d->steps_per_mm;
    tmc5160_write(d, TMC_REG_RAMPMODE, TMC_RAMPMODE_POSITIONING);
    tmc5160_write(d, TMC_REG_XTARGET, (uint32_t)backoff);

    t0 = HAL_GetTick();
    do {
        if ((HAL_GetTick() - t0) > TMC_MOVE_TIMEOUT_MS) return TMC_TIMEOUT;
        HAL_Delay(1);
        tmc5160_read(d, TMC_REG_RAMP_STAT, &rs);
    } while (!((rs & TMC_RAMPSTAT_POSITION_REACHED) && (rs & TMC_RAMPSTAT_VZERO)));

    /* Backed-off point is machine zero. */
    tmc5160_write(d, TMC_REG_XACTUAL, 0u);
    tmc5160_write(d, TMC_REG_XTARGET, 0u);

    tmc5160_write(d, TMC_REG_VMAX, TMC_VMAX_TRAVEL);
    return TMC_OK;
}

/** @} */
