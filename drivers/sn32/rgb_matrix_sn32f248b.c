#include "rgb_matrix.h"
#include "rgb_matrix_sn32f24xx.h"

#if !defined(RGB_MATRIX_HUE_STEP)
#    define RGB_MATRIX_HUE_STEP 8
#endif

#if !defined(RGB_MATRIX_SAT_STEP)
#    define RGB_MATRIX_SAT_STEP 16
#endif

#if !defined(RGB_MATRIX_VAL_STEP)
#    define RGB_MATRIX_VAL_STEP 16
#endif

#if !defined(RGB_MATRIX_SPD_STEP)
#    define RGB_MATRIX_SPD_STEP 16
#endif

/*
    COLS key / led
    Y2 transistors PNP driven high
    base      - GPIO
    collector - LED Col pins
    emitter   - VDD

    VDD     GPIO
    (E)     (B)
     |  PNP  |
     |_______|
         |
         |
        (C)
        LED

    ROWS RGB
    Y1 transistors NPN driven low
    base      - GPIO
    collector - LED RGB row pins
    emitter   - GND

        LED
        (C)
         |
         |
      _______
     |  NPN  |
     |       |
    (B)     (E)
    GPIO    GND
*/

static uint8_t chan_order[24] = {0}; // track the channel order
static uint8_t current_row = 0; // LED row scan counter
static uint8_t row_idx = 0; // key row scan counter
static const uint32_t freq = (RGB_MATRIX_HUE_STEP * RGB_MATRIX_SAT_STEP * RGB_MATRIX_VAL_STEP * RGB_MATRIX_SPD_STEP * RGB_MATRIX_LED_PROCESS_LIMIT);
RGB led_state[DRIVER_LED_TOTAL]; // led state buffer
extern matrix_row_t raw_matrix[MATRIX_ROWS]; //raw values
static const pin_t led_row_pins[LED_MATRIX_ROWS_HW] = LED_MATRIX_ROW_PINS; // We expect a R,B,G order here
static const pin_t led_col_pins[LED_MATRIX_COLS] = LED_MATRIX_COL_PINS;
bool enable_pwm = false;

/* PWM configuration structure. We use timer CT16B1 with 24 channels. */
static PWMConfig pwmcfg = {
    freq,          /* PWM clock frequency. */
    256,           /* PWM period (in ticks) 1S (1/10kHz=0.1mS 0.1ms*10000 ticks=1S) */
    NULL,          /* RGB Callback */
    {              /* Default all channels to disabled - Channels will be configured durring init */
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0},
        {PWM_OUTPUT_DISABLED, NULL, 0}
    },
    0/* HW dependent part.*/
};

void rgb_ch_ctrl(PWMConfig *cfg) {
    
    /* Enable PWM function, IOs and select the PWM modes for the LED column pins */
    for(uint8_t i = 0; i < LED_MATRIX_COLS; i++) {
        switch(led_col_pins[i]) {
            // Intentional fall-through for the PWM B-pin mapping
            case B8:
                cfg->channels[0].pfpamsk = 1;
            case A0:
                cfg->channels[0].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[0] = i;
                break;

            case B9:
                cfg->channels[1].pfpamsk = 1;
            case A1:
                cfg->channels[1].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[1] = i;
                break;
            
            case B10:
                cfg->channels[2].pfpamsk = 1;
            case A2:
                cfg->channels[2].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[2] = i;
                break;

            case B11:
                cfg->channels[3].pfpamsk = 1;
            case A3:
                cfg->channels[3].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[3] = i;
                break;

            case B12:
                cfg->channels[4].pfpamsk = 1;
            case A4:
                cfg->channels[4].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[4] = i;
                break;

            case B13:
                cfg->channels[5].pfpamsk = 1;
            case A5:
                cfg->channels[5].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[5] = i;
                break;

            case B14:
                cfg->channels[6].pfpamsk = 1;
            case A6:
                cfg->channels[6].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[6] = i;
                break;

            case B15:
                cfg->channels[7].pfpamsk = 1;
            case A7:
                cfg->channels[7].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[7] = i;
                break;

            case C0:
                cfg->channels[8].pfpamsk = 1;
            case A8:
                cfg->channels[8].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[8] = i;
                break;

            case C1:
                cfg->channels[9].pfpamsk = 1;
            case A9:
                cfg->channels[9].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[9] = i;
                break;

            case C2:
                cfg->channels[10].pfpamsk = 1;
            case A10:
                cfg->channels[10].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[10] = i;
                break;

            case C3:
                cfg->channels[11].pfpamsk = 1;
            case A11:
                cfg->channels[11].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[11] = i;
                break;

            case C4:
                cfg->channels[12].pfpamsk = 1;
            case A12:
                cfg->channels[12].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[12] = i;
                break;

            case C5:
                cfg->channels[13].pfpamsk = 1;
            case A13:
                cfg->channels[13].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[13] = i;
                break;

            case C6:
                cfg->channels[14].pfpamsk = 1;
            case A14:
                cfg->channels[14].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[14] = i;
                break;

            case C7:
                cfg->channels[15].pfpamsk = 1;
            case A15:
                cfg->channels[15].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[15] = i;
                break;

            case C8:
                cfg->channels[16].pfpamsk = 1;
            case B0:
                cfg->channels[16].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[16] = i;
                break;

            case C9:
                cfg->channels[17].pfpamsk = 1;
            case B1:
                cfg->channels[17].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[17] = i;
                break;

            case C10:
                cfg->channels[18].pfpamsk = 1;
            case B2:
                cfg->channels[18].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[18] = i;
                break;

            case C11:
                cfg->channels[19].pfpamsk = 1;
            case B3:
                cfg->channels[19].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[19] = i;
                break;

            case C12:
                cfg->channels[20].pfpamsk = 1;
            case B4:
                cfg->channels[20].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[20] = i;
                break;

            case C13:
                cfg->channels[21].pfpamsk = 1;
            case B5:
                cfg->channels[21].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[21] = i;
                break;

            case C14:
                cfg->channels[22].pfpamsk = 1;
            case B6:
                cfg->channels[22].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[22] = i;
                break;

            case C15:
                cfg->channels[23].pfpamsk = 1;
            case B7:
                cfg->channels[23].mode = PWM_OUTPUT_ACTIVE_LOW;
                chan_order[23] = i;
                break;
        }
    }

}
void rgb_callback(PWMDriver *pwmp);

void shared_matrix_rgb_enable(void) {
    pwmcfg.callback = rgb_callback;
    pwmEnablePeriodicNotification(&PWMD1);
}

void shared_matrix_rgb_disable_pwm(void) {
    // Disable PWM outputs on column pins
    for(uint8_t i=0;i<24;i++){
        if (&pwmcfg.channels[i].mode != PWM_OUTPUT_DISABLED){
            pwmDisableChannel(&PWMD1,i);
        }
    }
}

void shared_matrix_rgb_disable_leds(void) {
    // Disable LED outputs on row pins
    for (uint8_t x = 0; x < LED_MATRIX_ROWS_HW; x++) {
        writePinLow(led_row_pins[x]);
    }
}

void update_pwm_channels(PWMDriver *pwmp, uint8_t last_row) {
    for(uint8_t i=0; i<24; i++){
        if (&pwmcfg.channels[i].mode != PWM_OUTPUT_DISABLED){
            // Scan the key matrix
            #if(DIODE_DIRECTION == ROW2COL)
                pwmDisableChannelI(pwmp,chan_order[i]);
                matrix_scan_keys(raw_matrix,chan_order[i]);
            #endif
            uint8_t led_index = g_led_config.matrix_co[row_idx][chan_order[i]];
            // Check if we need to enable RGB output
            if (led_state[led_index].b != 0) enable_pwm |= true;
            if (led_state[led_index].g != 0) enable_pwm |= true;
            if (led_state[led_index].r != 0) enable_pwm |= true;
            switch(current_row % LED_MATRIX_ROW_CHANNELS) {
            case 0:
                    if(enable_pwm) pwmEnableChannelI(pwmp,chan_order[i],led_state[led_index].b);
                break;
            case 1:
                    if(enable_pwm) pwmEnableChannelI(pwmp,chan_order[i],led_state[led_index].g);
                break;
            case 2:
                    if(enable_pwm) pwmEnableChannelI(pwmp,chan_order[i],led_state[led_index].r);
                break;
            default:
                ;
            }
        }
    }
}
void rgb_callback(PWMDriver *pwmp) {
    // Disable the interrupt
    pwmDisablePeriodicNotification(pwmp);
    uint8_t last_row = current_row;
    // Advance to the next led row
    current_row++;
    if(current_row >= LED_MATRIX_ROWS_HW) current_row = 0;
    // Advance to the next key row
    if(current_row % LED_MATRIX_ROW_CHANNELS == 2) row_idx++;
    if(row_idx >= LED_MATRIX_ROWS) row_idx = 0;
    chSysLockFromISR();
    shared_matrix_rgb_disable_leds();
    #if(DIODE_DIRECTION == COL2ROW)
        shared_matrix_rgb_disable_pwm();
        matrix_scan_keys(raw_matrix, row_idx);
    #endif
    update_pwm_channels(pwmp, last_row);
    if(enable_pwm) writePinHigh(led_row_pins[current_row]);
    chSysUnlockFromISR();
    // Advance the timer to just before the wrap-around, that will start a new PWM cycle
    pwm_lld_change_counter(pwmp, 0xFFFF);
    // Enable the interrupt
    pwmEnablePeriodicNotification(pwmp);
}

void SN32F24XX_init(void) {
    for (uint8_t x = 0; x < LED_MATRIX_ROWS_HW; x++) {
        setPinOutput(led_row_pins[x]);
        writePinLow(led_row_pins[x]);
    }
    rgb_ch_ctrl(&pwmcfg);
    pwmStart(&PWMD1, &pwmcfg);
    shared_matrix_rgb_enable();
}

static void flush(void) {} // Due to the way we do PWM, every cycle is a flush

void SN32F24XX_set_color(int index, uint8_t r, uint8_t g, uint8_t b) {
            led_state[index].r = r;
            led_state[index].b = b;
            led_state[index].g = g;
}

void SN32F24XX_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    for (int i=0; i<DRIVER_LED_TOTAL; i++) {
        SN32F24XX_set_color(i, r, g, b);
    }
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = SN32F24XX_init,
    .flush         = flush,
    .set_color     = SN32F24XX_set_color,
    .set_color_all = SN32F24XX_set_color_all,
};