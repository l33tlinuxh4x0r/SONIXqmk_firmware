/*
Copyright 2021 Adam Honse <calcprogrammer1@gmail.com>
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Ported to QMK by Stephen Peery <https://github.com/smp4488/>
*/

// Key and LED matrix driver for SN32F260.
// This driver will take full control of CT16B1 and GPIO in MATRIX_ROW_PINS, MATRIX_COL_PINS, LED_MATRIX_ROW_PINS and LED_MATRIX_COL_PINS.
// This file implements a RGB matrix led driver but only the red channel is used. Because the RGB matrix is currently better supported then LED matrix.
//
// The CT16B1 on the SN32F260 can be used as a 23 channel PWM peripheral.
// However PWM cannot be used naively, because a new PWM cycle is started when the previous one ends.
// This is an issue because the key and led matrix is multiplexed and there is some GPIO row and PWM duty cycle reconfiguration needed between PWM cycles.
// The PWM can be stoped and stated again but this has some overhead.
// This driver uses a trick to keep PWM running and have some time in between PWM cycles for the reconfiguration.
//
// While the active duty cycle values are between 0-255, the period is not configured and the implicit wrap-around at 0xFFFF is used.
// This way the timer period is divide into two phases. 0 to 255 is used as active PWM phase, while 256 to 0xFFFF can be used for the reconfiguration.
// The CT16B1 is configured to generate an interupt when the counter reaches 255. In this interrupt the reconfiguration is done.
// The reconfiguration needs some time to execute but not the whole duration from 256 to 0xFFFF.
// So when reconfiguration is done the timer is advanced forward to 0xFFFF.
// One tick later the timer will wrap-around and a new PWM cycle is started.
//
// The final behaviour is exactly what we want, a PWM cycle followed by a short reconfiguration and then the next PWM cycle.

#include <stdint.h>
#include <stdbool.h>
#include <string.h> // memset()
#include <SN32F260.h>
#include "SN32F260_defines.h"
#include "ch.h"
#include "hal.h"
#include "quantum.h"
#include "matrix.h"
#include "debounce.h"
#include "rgb_matrix.h"

#ifndef SN32_CT16B1_IRQ_PRIORITY
#define SN32_CT16B1_IRQ_PRIORITY 0
#endif

// MATRIX_KEY_SAMPLE_DELAY is a delay in arbritray time units, that will delay the sampling of the column inputs, after a row is selected.
// This small delay to let internal pull-up resitors pull-up the input pins.
// The input pin can still be low when the previous row had a pressed button.
#ifndef MATRIX_KEY_SAMPLE_DELAY
// The default value is the result of some experimentation
#define MATRIX_KEY_SAMPLE_DELAY 100
#endif

static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;
static const pin_t led_row_pins[LED_MATRIX_ROWS_HW] = LED_MATRIX_ROW_PINS;
static const pin_t led_col_pins[LED_MATRIX_COLS] = LED_MATRIX_COL_PINS;

static matrix_row_t raw_matrix[MATRIX_ROWS]; //raw values
static matrix_row_t last_matrix[MATRIX_ROWS] = {0};  // raw values
static matrix_row_t matrix[MATRIX_ROWS]; //debounced values

static uint8_t current_row = 0;
static uint8_t led_state[DRIVER_LED_TOTAL];

static inline bool cols_ordered(void);
static inline void load_mrs(int row);
static inline uint32_t used_mrs_mask(void);


static void init(void) {
    // NOP
}

static void flush(void){
    // NOP
}

static void set_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    led_state[index] = r;
}

static void set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    memset(led_state, r, sizeof(led_state));
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = init,
    .flush         = flush,
    .set_color     = set_color,
    .set_color_all = set_color_all,
};

__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}

matrix_row_t matrix_get_row(uint8_t row) { return matrix[row]; }

void matrix_print(void) {}

static void init_pins(void) {

#if(DIODE_DIRECTION == ROW2COL)
    //  Unselect ROWs
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        setPinInputHigh(row_pins[x]);
    }

    // Unselect COLs
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        setPinOutput(col_pins[x]);
        writePinHigh(col_pins[x]);
    }

#elif(DIODE_DIRECTION == COL2ROW)
    //  Unselect ROWs
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        setPinOutput(row_pins[x]);
        writePinHigh(row_pins[x]);
    }

    // Unselect COLs
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        setPinInputHigh(col_pins[x]);
    }
#else
#error DIODE_DIRECTION must be one of COL2ROW or ROW2COL!
#endif

    for (uint8_t x = 0; x < LED_MATRIX_ROWS_HW; x++) {
        setPinOutput(led_row_pins[x]);
        writePinLow(led_row_pins[x]);
    }
}

void matrix_init(void) {
    // initialize key pins
    init_pins();

    // initialize matrix state: all keys off
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        raw_matrix[i] = 0;
        matrix[i]     = 0;
    }

    debounce_init(MATRIX_ROWS);

    matrix_init_quantum();

    // Enable Timer Clock
    SN_SYS1->AHBCLKEN_b.CT16B1CLKEN = 1;

    // Set PWM mode 1 for LED colom pins. PWMCTRL, PWMCTRL1 and PWMCTRL2 don't need to be set, there reset values are zero.
    SN_CT16B1->PWMENB = used_mrs_mask();

    // Set match interrupts and TC rest
    SN_CT16B1->MCTRL3 = SN_CT16B1_MCTRL3_MR23IE_Msk;

    // Match register used to generate interrupt.
    SN_CT16B1->MR23 = 0xFF;

    // Set prescale value, aim for 1.2kHz. Due to overhead the actual freq will be a bit lower, but should still be above 1kHz.
    // Tests with 12MHz and 6 rows, resulted in a scan frequency of 1.1kHz
    SN_CT16B1->PRE = SystemCoreClock / (256 * LED_MATRIX_ROWS_HW * 1200) - 1;

    //Set CT16B1 as the up-counting mode.
    SN_CT16B1->TMRCTRL = SN_CT16B1_TMRCTRL_CRST_Msk;

    // Wait until timer reset done.
    while (SN_CT16B1->TMRCTRL & SN_CT16B1_TMRCTRL_CRST_Msk);

    // Let TC start counting.
    SN_CT16B1->TMRCTRL = SN_CT16B1_TMRCTRL_CEN_Msk;

    NVIC_ClearPendingIRQ(CT16B1_IRQn);
    nvicEnableVector(CT16B1_IRQn, SN32_CT16B1_IRQ_PRIORITY);
}

uint8_t matrix_scan(void) {
    bool matrix_changed = false;
    for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++) {
        for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
            // Determine if the matrix changed state
            if ((last_matrix[row_index] != raw_matrix[row_index])) {
                matrix_changed         = true;
                last_matrix[row_index] = raw_matrix[row_index];
            }
        }
    }

    debounce(raw_matrix, matrix, MATRIX_ROWS, matrix_changed);

    matrix_scan_quantum();

    return matrix_changed;
}

/**
 * @brief   CT16B1 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(SN32_CT16B1_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    // Clear match interrupt status
    SN_CT16B1->IC = SN_CT16B1_IC_MR23IC_Msk;

    // Turn the selected row off
    writePinLow(led_row_pins[current_row]);

    // Disable PWM outputs on column pins
    SN_CT16B1->PWMIOENB = 0;

    // Move to the next row
    current_row++;
    if(current_row >= LED_MATRIX_ROWS_HW) current_row = 0;

    if(current_row == 0) {

#if(DIODE_DIRECTION == ROW2COL)
        // Read the key matrix
        for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
            // Enable the column
            writePinLow(col_pins[col_index]);

            // Small delay to let internal pull-up resitors pull-up the input pins.
            for(int i = 0; i < MATRIX_KEY_SAMPLE_DELAY; i++){ __asm__ volatile("" ::: "memory");  }

            for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
                // Check row pin state
                if (readPin(row_pins[row_index]) == 0) {
                    // Pin LO, set col bit
                    raw_matrix[row_index] |= (MATRIX_ROW_SHIFTER << col_index);
                } else {
                    // Pin HI, clear col bit
                    raw_matrix[row_index] &= ~(MATRIX_ROW_SHIFTER << col_index);
                }
            }

            // Disable the column
            writePinHigh(col_pins[col_index]);
        }

#elif(DIODE_DIRECTION == COL2ROW)
        // Read the key matrix
        for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
            // Enable the row
            writePinLow(row_pins[row_index]);

            // Small delay to let internal pull-up resitors pull-up the input pins.
            for(int i = 0; i < MATRIX_KEY_SAMPLE_DELAY; i++){ __asm__ volatile("" ::: "memory");  }

            // When MATRIX_COL_PINS is ordered from MR0 to MRn, the following optimization is possible.
            // This if should be resolved at compile time.
            if( cols_ordered() ){
                // Fast implementation

                // Read all col pin values at once into a varable. This allows for better code gen.
                // The bit position in col_pin_values matches MR numbering.
                uint32_t col_pin_values = (pal_lld_readport(GPIOA) & 0xFFFF) | ((pal_lld_readport(GPIOD) & 0x3F) << 16);

                // Invert because a pressed key will make a input pin low.
                col_pin_values = ~col_pin_values;

                // Keep bottom MATRIX_COLS bits.
                col_pin_values &= ((1 << MATRIX_COLS) - 1);

                raw_matrix[row_index] = col_pin_values;
            } else {
                // Slow fallback implementation

                matrix_row_t raw = 0;
                matrix_row_t mask = 1;
                for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
                    // Check row pin state
                    if (readPin(col_pins[col_index]) == 0) {
                        raw |= mask;
                    }
                    mask <<= 1;
                }

                raw_matrix[row_index] = raw;
            }

            // Disable the row
            writePinHigh(row_pins[row_index]);
        }
#endif

    }

    // Load MRs with led_state data for the current row
    load_mrs(current_row);

    // Re-enable PWM on column pins
    SN_CT16B1->PWMIOENB = used_mrs_mask();

    // Turn the current row on
    writePinHigh(led_row_pins[current_row]);

    // Advance the timer to just before the wrap-around, that will start a new PWM cycle
    SN_CT16B1->TC = 0xFFFF;

    OSAL_IRQ_EPILOGUE();
}

// Map pin to MR number
static inline int8_t pin_to_mr(pin_t pin){
    switch(pin){
        case A0:  return 0;
        case A1:  return 1;
        case A2:  return 2;
        case A3:  return 3;
        case A4:  return 4;
        case A5:  return 5;
        case A6:  return 6;
        case A7:  return 7;
        case A8:  return 8;
        case A9:  return 9;
        case A10: return 10;
        case A11: return 11;
        case A12: return 12;
        case A13: return 13;
        case A14: return 14;
        case A15: return 15;
        case D0:  return 16;
        case D1:  return 17;
        case D2:  return 18;
        case D3:  return 19;
        case D4:  return 20;
        case D5:  return 21;
        case D8:  return 22;
    }

    // Should not happen!
    return 0;
}

// Mask with use MRs
// This whole function should optimize to a constant.
static inline uint32_t used_mrs_mask(void) {
    // Macro that sets the bit n if MRn is used.
    #define MASK_MR(col)  if(col < LED_MATRIX_COLS) { mask |= 1 << pin_to_mr(led_col_pins[col]); }

    // Manual unroll constructing the mask
    uint32_t mask = 0;
    MASK_MR(0);
    MASK_MR(1);
    MASK_MR(2);
    MASK_MR(3);
    MASK_MR(4);
    MASK_MR(5);
    MASK_MR(6);
    MASK_MR(7);
    MASK_MR(8);
    MASK_MR(9);
    MASK_MR(10);
    MASK_MR(11);
    MASK_MR(12);
    MASK_MR(13);
    MASK_MR(14);
    MASK_MR(15);
    MASK_MR(16);
    MASK_MR(17);
    MASK_MR(18);
    MASK_MR(19);
    MASK_MR(20);
    MASK_MR(21);
    MASK_MR(22);
    return mask;
}

// Given a row, load the MRs with led_state data
static inline void load_mrs(int row) {
    // Convince compiler that MRs can be accesssed as an array. This is done to improve code gen.
    volatile uint32_t (* const MRx)[23] = (volatile uint32_t (*)[23]) &SN_CT16B1->MR0;

    // Marco that load the MR for the given col.
    // It is a NOP if col >= LED_MATRIX_COLS
    // This gets compiled into three instruction, two ldrb and one str.
    // When g_led_config.matrix_co contains a NO_LED (255) entry, led_state[] will be indexed out of bound.
    // So this will load a unknown value into the MR register, but this doesn't matter because there is no led.
    #define LOAD_MR(col)  if(col < LED_MATRIX_COLS) { (*MRx)[pin_to_mr(led_col_pins[col])] = led_state[g_led_config.matrix_co[row][col]]; }

    // Manual unroll the loading of the MRs
    LOAD_MR(0);
    LOAD_MR(1);
    LOAD_MR(2);
    LOAD_MR(3);
    LOAD_MR(4);
    LOAD_MR(5);
    LOAD_MR(6);
    LOAD_MR(7);
    LOAD_MR(8);
    LOAD_MR(9);
    LOAD_MR(10);
    LOAD_MR(11);
    LOAD_MR(12);
    LOAD_MR(13);
    LOAD_MR(14);
    LOAD_MR(15);
    LOAD_MR(16);
    LOAD_MR(17);
    LOAD_MR(18);
    LOAD_MR(19);
    LOAD_MR(20);
    LOAD_MR(21);
    LOAD_MR(22);
}

// Return true if MATRIX_COL_PINS is in MR0 to MRn order
// This whole function should optimize to a constant.
static inline bool cols_ordered(void){
    // Tests if MATRIX_COL_PINS[n] maps to MRn
    #define TEST_ORDER(col) if(col < LED_MATRIX_COLS) { if(col != pin_to_mr(col_pins[col])) return false; }

    // Manual unroll the testing of the order
    TEST_ORDER(0);
    TEST_ORDER(1);
    TEST_ORDER(2);
    TEST_ORDER(3);
    TEST_ORDER(4);
    TEST_ORDER(5);
    TEST_ORDER(6);
    TEST_ORDER(7);
    TEST_ORDER(8);
    TEST_ORDER(9);
    TEST_ORDER(10);
    TEST_ORDER(11);
    TEST_ORDER(12);
    TEST_ORDER(13);
    TEST_ORDER(14);
    TEST_ORDER(15);
    TEST_ORDER(16);
    TEST_ORDER(17);
    TEST_ORDER(18);
    TEST_ORDER(19);
    TEST_ORDER(20);
    TEST_ORDER(21);
    TEST_ORDER(22);
    return true;
}
