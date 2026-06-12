/**
 * @file    tmc5160_regs.h
 * @brief   TMC5160 register addresses and bitfield masks (datasheet rev 1.14).
 * @details Include via tmc5160.h; not intended for direct inclusion.
 * @ingroup tmc5160
 */

#ifndef TMC5160_REGS_H
#define TMC5160_REGS_H

/* -----------------------------------------------------------------------
 * Register addresses (7-bit, write flag = bit7 set by driver layer)
 * --------------------------------------------------------------------- */

/* General */
#define TMC_REG_GCONF        0x00
#define TMC_REG_GSTAT        0x01
#define TMC_REG_IFCNT        0x02
#define TMC_REG_SLAVECONF    0x03
#define TMC_REG_IOIN         0x04   /* read: input pins; [31:24] = VERSION */

/* IOIN VERSION field — top byte must read 0x30 on a healthy TMC5160 */
#define TMC_IOIN_VERSION_SHIFT  24
#define TMC_IOIN_VERSION_TMC5160 0x30u

/* Velocity / ramp */
#define TMC_REG_XACTUAL      0x21
#define TMC_REG_VACTUAL      0x22
#define TMC_REG_VSTART       0x23
#define TMC_REG_A1           0x24
#define TMC_REG_V1           0x25
#define TMC_REG_AMAX         0x26
#define TMC_REG_VMAX         0x27
#define TMC_REG_DMAX         0x28
#define TMC_REG_D1           0x2A
#define TMC_REG_VSTOP        0x2B
#define TMC_REG_TZEROWAIT    0x2C
#define TMC_REG_XTARGET      0x2D
#define TMC_REG_RAMPMODE     0x20
#define TMC_REG_RAMP_STAT    0x35
#define TMC_REG_XLATCH       0x36

/* Reference / homing */
#define TMC_REG_SW_MODE      0x34

/* Current control */
#define TMC_REG_IHOLD_IRUN   0x10
#define TMC_REG_TPOWERDOWN   0x11
#define TMC_REG_TSTEP        0x12
#define TMC_REG_TPWMTHRS     0x13

/* Chopper */
#define TMC_REG_CHOPCONF     0x6C
#define TMC_REG_COOLCONF     0x6D
#define TMC_REG_DCCTRL       0x6E
#define TMC_REG_DRV_STATUS   0x6F
#define TMC_REG_PWMCONF      0x70
#define TMC_REG_PWM_SCALE    0x71
#define TMC_REG_PWM_AUTO     0x72
#define TMC_REG_LOST_STEPS   0x73

/* -----------------------------------------------------------------------
 * RAMPMODE values
 * --------------------------------------------------------------------- */
#define TMC_RAMPMODE_POSITIONING  0
#define TMC_RAMPMODE_VEL_POS      1   /* velocity mode, positive direction */
#define TMC_RAMPMODE_VEL_NEG      2   /* velocity mode, negative direction */
#define TMC_RAMPMODE_HOLD         3

/* -----------------------------------------------------------------------
 * GCONF bitfields
 * --------------------------------------------------------------------- */
#define TMC_GCONF_RECALIBRATE        (1u << 0)
#define TMC_GCONF_FASTSTANDSTILL     (1u << 1)
#define TMC_GCONF_EN_PWM_MODE        (1u << 2)   /* StealthChop2 enable */
#define TMC_GCONF_MULTISTEP_FILT     (1u << 3)
#define TMC_GCONF_SHAFT              (1u << 4)   /* invert motor direction */
#define TMC_GCONF_DIAG0_ERROR        (1u << 5)
#define TMC_GCONF_DIAG0_OTPW         (1u << 6)
#define TMC_GCONF_DIAG0_STALL        (1u << 7)
#define TMC_GCONF_DIAG1_STALL        (1u << 8)
#define TMC_GCONF_DIAG1_INDEX        (1u << 9)
#define TMC_GCONF_DIAG1_ONSTATE      (1u << 10)
#define TMC_GCONF_DIAG1_STEPS_SKIPPED (1u << 11)
#define TMC_GCONF_DIAG0_INT_PUSHPULL (1u << 12)
#define TMC_GCONF_DIAG1_POSCOMP_PUSHPULL (1u << 13)
#define TMC_GCONF_SMALL_HYSTERESIS   (1u << 14)
#define TMC_GCONF_STOP_ENABLE        (1u << 15)
#define TMC_GCONF_DIRECT_MODE        (1u << 16)

/* -----------------------------------------------------------------------
 * GSTAT bitfields  (write 1 to clear)
 * --------------------------------------------------------------------- */
#define TMC_GSTAT_RESET    (1u << 0)   /* set on power-on / reset */
#define TMC_GSTAT_DRV_ERR  (1u << 1)
#define TMC_GSTAT_UV_CP    (1u << 2)

/* -----------------------------------------------------------------------
 * RAMP_STAT bitfields  (read-only)
 * --------------------------------------------------------------------- */
#define TMC_RAMPSTAT_STATUS_STOP_L       (1u << 0)
#define TMC_RAMPSTAT_STATUS_STOP_R       (1u << 1)
#define TMC_RAMPSTAT_STATUS_LATCH_L      (1u << 2)
#define TMC_RAMPSTAT_STATUS_LATCH_R      (1u << 3)
#define TMC_RAMPSTAT_EVENT_STOP_L        (1u << 4)
#define TMC_RAMPSTAT_EVENT_STOP_R        (1u << 5)
#define TMC_RAMPSTAT_EVENT_STOP_SG       (1u << 6)
#define TMC_RAMPSTAT_EVENT_POS_REACHED   (1u << 7)
#define TMC_RAMPSTAT_VELOCITY_REACHED    (1u << 8)
#define TMC_RAMPSTAT_POSITION_REACHED    (1u << 9)
#define TMC_RAMPSTAT_VZERO               (1u << 10)
#define TMC_RAMPSTAT_T_ZEROWAIT_ACTIVE   (1u << 11)
#define TMC_RAMPSTAT_SECOND_MOVE         (1u << 12)
#define TMC_RAMPSTAT_STATUS_SG           (1u << 13)

/* -----------------------------------------------------------------------
 * SW_MODE bitfields
 * --------------------------------------------------------------------- */
#define TMC_SWMODE_STOP_L_ENABLE    (1u << 0)   /* enable left  (REFL) stop */
#define TMC_SWMODE_STOP_R_ENABLE    (1u << 1)   /* enable right (REFR) stop */
#define TMC_SWMODE_POL_STOP_L       (1u << 2)   /* 0=active low, 1=active high */
#define TMC_SWMODE_POL_STOP_R       (1u << 3)
#define TMC_SWMODE_SWAP_LR          (1u << 4)
#define TMC_SWMODE_LATCH_L_ACTIVE   (1u << 5)
#define TMC_SWMODE_LATCH_L_INACTIVE (1u << 6)
#define TMC_SWMODE_LATCH_R_ACTIVE   (1u << 7)
#define TMC_SWMODE_LATCH_R_INACTIVE (1u << 8)
#define TMC_SWMODE_EN_LATCH_ENCODER (1u << 9)
#define TMC_SWMODE_SG_STOP          (1u << 10)
#define TMC_SWMODE_EN_SOFTSTOP      (1u << 11)

/* -----------------------------------------------------------------------
 * DRV_STATUS bitfields
 * --------------------------------------------------------------------- */
#define TMC_DRVSTATUS_SG_RESULT_MASK  0x3FFu
#define TMC_DRVSTATUS_S2VSA           (1u << 12)
#define TMC_DRVSTATUS_S2VSB           (1u << 13)
#define TMC_DRVSTATUS_STEALTH         (1u << 14)
#define TMC_DRVSTATUS_FSACTIVE        (1u << 15)
#define TMC_DRVSTATUS_CS_ACTUAL_MASK  (0x1Fu << 16)
#define TMC_DRVSTATUS_STALLGUARD      (1u << 24)
#define TMC_DRVSTATUS_OT              (1u << 25)   /* overtemp shutdown */
#define TMC_DRVSTATUS_OTPW            (1u << 26)   /* overtemp warning */
#define TMC_DRVSTATUS_S2GA            (1u << 27)
#define TMC_DRVSTATUS_S2GB            (1u << 28)
#define TMC_DRVSTATUS_OLA             (1u << 29)
#define TMC_DRVSTATUS_OLB             (1u << 30)
#define TMC_DRVSTATUS_STST            (1u << 31)   /* standstill */

/* -----------------------------------------------------------------------
 * SPI_STATUS byte (first byte of every reply)
 * --------------------------------------------------------------------- */
#define TMC_SPI_STATUS_RESET_FLAG    (1u << 0)
#define TMC_SPI_STATUS_DRV_ERR       (1u << 1)
#define TMC_SPI_STATUS_SG2           (1u << 2)
#define TMC_SPI_STATUS_STANDSTILL    (1u << 3)
#define TMC_SPI_STATUS_VEL_REACHED   (1u << 4)
#define TMC_SPI_STATUS_POS_REACHED   (1u << 5)
#define TMC_SPI_STATUS_STOP_L        (1u << 6)
#define TMC_SPI_STATUS_STOP_R        (1u << 7)

/* -----------------------------------------------------------------------
 * CHOPCONF helpers — TOFF must be non-zero or outputs stay off
 * --------------------------------------------------------------------- */
/* CHOPCONF bit positions */
#define TMC_CHOPCONF_TOFF_SHIFT   0
#define TMC_CHOPCONF_TOFF_MASK    (0xFu << 0)
#define TMC_CHOPCONF_HSTRT_SHIFT  4
#define TMC_CHOPCONF_HEND_SHIFT   7
#define TMC_CHOPCONF_TBL_SHIFT    15
#define TMC_CHOPCONF_VHIGHFS      (1u << 18)
#define TMC_CHOPCONF_VHIGHCHM     (1u << 19)
#define TMC_CHOPCONF_MRES_SHIFT   24   /* microstep resolution */
#define TMC_CHOPCONF_MRES_MASK    (0xFu << 24)
#define TMC_CHOPCONF_INTPOL       (1u << 28)   /* interpolate to 256 */
#define TMC_CHOPCONF_DEDGE        (1u << 29)
#define TMC_CHOPCONF_DISS2G       (1u << 30)
#define TMC_CHOPCONF_DISS2VS      (1u << 31)

/* MRES values for CHOPCONF[27:24] */
#define TMC_MRES_256   0x0
#define TMC_MRES_128   0x1
#define TMC_MRES_64    0x2
#define TMC_MRES_32    0x3
#define TMC_MRES_16    0x4
#define TMC_MRES_8     0x5
#define TMC_MRES_4     0x6
#define TMC_MRES_2     0x7
#define TMC_MRES_FULL  0x8

/* -----------------------------------------------------------------------
 * IHOLD_IRUN packing helper
 *   bits [4:0]   = IHOLD  (standstill current, 0-31)
 *   bits [12:8]  = IRUN   (run current, 0-31)
 *   bits [19:16] = IHOLDDELAY
 * --------------------------------------------------------------------- */
#define TMC_IHOLD_IRUN(ihold, irun, idelay) \
    (((uint32_t)(ihold) & 0x1Fu)        | \
     (((uint32_t)(irun)  & 0x1Fu) << 8) | \
     (((uint32_t)(idelay)& 0x0Fu) << 16))

#endif /* TMC5160_REGS_H */
