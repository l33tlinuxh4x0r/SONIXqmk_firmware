#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SN32F240B.h>
#include "ch.h"
#include "hal.h"
#include "CT16.h"

#include "color.h"
#include "wait.h"
#include "util.h"
#include "matrix.h"
#include "debounce.h"
#include "quantum.h"

#define I2C_SCL D6
#define I2C_SDA D5

#define I2C_SCL_IN readPin(I2C_SCL)
#define I2C_SDA_IN readPin(I2C_SDA)

#define I2C_SCL_HI setPinInputHigh(I2C_SCL)
#define I2C_SCL_LO do{setPinOutput(I2C_SCL);writePinLow(I2C_SCL);}while(0)
#define I2C_SCL_HIZ setPinInputHigh(I2C_SCL)

#define I2C_SDA_HI setPinInputHigh(I2C_SDA)
#define I2C_SDA_LO do{setPinOutput(I2C_SDA);writePinLow(I2C_SDA);}while(0)
#define I2C_SDA_HIZ setPinInputHigh(I2C_SDA)

enum
{
    I2C_STATE_INACTIVE          = 0,
    I2C_STATE_START_COND        = 1,
    I2C_STATE_SEND_ADDR_RW      = 2,
    I2C_STATE_READ_ACK          = 3,
    I2C_STATE_WRITE_BYTE        = 4,
    I2C_STATE_READ_BYTE         = 5,
    I2C_STATE_STOP_COND         = 6,
};

static uint8_t  i2c_state   = 0;
static uint8_t  i2c_byte_ct = 0;
static uint8_t  i2c_addr_rw = 0;
static uint8_t* i2c_data_ptr;
static uint8_t  i2c_data_byte[2] = {0};
static uint8_t  i2c_data_idx = 0;
static uint8_t  i2c_tx_byte = 0;

void i2cInit(void)
{
    // Enable Timer Clock
    SN_SYS1->AHBCLKEN_b.CT16B0CLKEN = 1;

    // Set match interrupts and TC rest
    SN_CT16B0->MCTRL = (mskCT16_MR0IE_EN | mskCT16_MR0RST_EN);

    // COL match register
    SN_CT16B0->MR0 = 0x7F;

    // Set prescale value
    SN_CT16B0->PRE = 0x00;

    //Set CT16B0 as the up-counting mode.
	SN_CT16B0->TMRCTRL = (mskCT16_CRST);

    // Wait until timer reset done.
    while (SN_CT16B0->TMRCTRL & mskCT16_CRST);

    // Let TC start counting.
    SN_CT16B0->TMRCTRL |= mskCT16_CEN_EN;

    NVIC_ClearPendingIRQ(CT16B0_IRQn);
    nvicEnableVector(CT16B0_IRQn, 3);

    I2C_SCL_HIZ;
    I2C_SDA_HIZ;
}

static inline void process_i2c_bit(void)
{
if(i2c_tx_byte & 0x80)
{
    I2C_SDA_HIZ;
}
else
{
    I2C_SDA_LO;
}

i2c_tx_byte = i2c_tx_byte << 1;

I2C_SCL_HIZ;
I2C_SCL_LO;
}

static inline void i2c_step_state(void)
{
    switch(i2c_state)
    {
        case I2C_STATE_INACTIVE:
            break;

        case I2C_STATE_START_COND:
            I2C_SDA_LO;
            I2C_SCL_LO;

            i2c_state = I2C_STATE_SEND_ADDR_RW;

            i2c_tx_byte = i2c_addr_rw;
            i2c_data_idx = 0;
            break;

        case I2C_STATE_SEND_ADDR_RW:
        case I2C_STATE_WRITE_BYTE:
            process_i2c_bit();
            process_i2c_bit();
            process_i2c_bit();
            process_i2c_bit();
            process_i2c_bit();
            process_i2c_bit();
            process_i2c_bit();
            process_i2c_bit();

            if(i2c_state == I2C_STATE_WRITE_BYTE)
            {
                i2c_byte_ct = i2c_byte_ct - 1;
                i2c_data_idx = i2c_data_idx + 1;
            }

            i2c_state = I2C_STATE_READ_ACK;
            break;

        case I2C_STATE_READ_ACK:
            I2C_SDA_HIZ;
            I2C_SCL_HIZ;
            if(!I2C_SDA_IN)
            {
                // Slave did ACK, move on to data
                if(i2c_byte_ct > 0)
                {
                    i2c_state = I2C_STATE_WRITE_BYTE;
                    i2c_tx_byte = i2c_data_ptr[i2c_data_idx];
                }
                else
                {
                    i2c_state = I2C_STATE_STOP_COND;
                }
            }
            else
            {
                // Slave did not ACK, send stop condition
                i2c_state = I2C_STATE_STOP_COND;
            }

            I2C_SDA_LO;
            I2C_SCL_LO;
            break;

        case I2C_STATE_STOP_COND:
            I2C_SCL_HIZ;
            I2C_SDA_HIZ;
            i2c_state = I2C_STATE_INACTIVE;
            break;
    }
}

/**
 * @brief   CT16B0 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(SN32_CT16B0_HANDLER) {

    OSAL_IRQ_PROLOGUE();
    
    SN_CT16B0->IC = mskCT16_MR0IC; // Clear match interrupt status

    //Set CT16B1 as the up-counting mode.
    SN_CT16B0->TMRCTRL = (mskCT16_CRST);

    // Wait until timer reset done.
    while (SN_CT16B0->TMRCTRL & mskCT16_CRST);

    for(int i = 0; i < 32; i++)
    {
        i2c_step_state();

        if(i2c_state == I2C_STATE_INACTIVE)
        {
            break;
        }
    }

    // Set match interrupts and TC rest
    SN_CT16B0->MCTRL = (mskCT16_MR0IE_EN | mskCT16_MR0STOP_EN);
    
    if(i2c_state != I2C_STATE_INACTIVE)
    {
        // Let TC start counting.
        SN_CT16B0->TMRCTRL |= mskCT16_CEN_EN;
    }

    OSAL_IRQ_EPILOGUE();
}

bool i2cBusWriteByte(int value)
{
    if(i2c_state == I2C_STATE_INACTIVE)
    {
        i2c_state = I2C_STATE_START_COND;
        i2c_addr_rw = (uint8_t)0xE8;
        i2c_data_byte[0] = value;

        i2c_data_ptr = i2c_data_byte;
        i2c_byte_ct = 1;

        // Let TC start counting.
        SN_CT16B0->TMRCTRL |= mskCT16_CEN_EN;
    }

    return 0;
}

char i2cWriteBuf(uint8_t devid, uint8_t* data, uint8_t len)
{   
    if(i2c_state != I2C_STATE_INACTIVE)
    {
        return 0;
    }
    
    i2c_state = I2C_STATE_START_COND;
    i2c_addr_rw = devid;

    i2c_data_ptr = data;
    i2c_byte_ct = len;

    // Let TC start counting.
    SN_CT16B0->TMRCTRL |= mskCT16_CEN_EN;

    return 1;
}

char i2cWriteReg(uint8_t devid, uint8_t reg, uint8_t data)
{
    if(i2c_state != I2C_STATE_INACTIVE)
    {
        return 0;
    }
    
    i2c_state = I2C_STATE_START_COND;
    i2c_addr_rw = devid;
    i2c_data_byte[0] = reg;
    i2c_data_byte[1] = data;

    i2c_data_ptr = i2c_data_byte;
    i2c_byte_ct = 2;

    // Let TC start counting.
    SN_CT16B0->TMRCTRL |= mskCT16_CEN_EN;

    return 1;
}

enum
{
    REG_CONFIGURE_COMMAND       = 0xFD,
    REG_FUNC_CONFIGURATION      = 0x00,
    REG_FUNC_PICTURE_DISPLAY    = 0x01,
    REG_FUNC_DISPLAY_OPTION     = 0x05,
    REG_FUNC_AUDIO_SYNC         = 0x06,
    REG_FUNC_BREATH_CONTROL_1   = 0x08,
    REG_FUNC_BREATH_CONTROL_2   = 0x09,
    REG_FUNC_SHUTDOWN           = 0x0A,
    REG_FUNC_AUDIO_GAIN_CONTROL = 0x0B,
    REG_FUNC_STAGGERED_DELAY    = 0x0D,
    REG_FUNC_SLEW_RATE_CONTROL  = 0x0E,
    REG_FUNC_CURRENT_CONTROL    = 0x0F,
    REG_FUNC_OPEN_SHORT_TEST_1  = 0x10,
    REG_FUNC_OPEN_SHORT_TEST_2  = 0x11,
    REG_FUNC_ADC_OUTPUT         = 0x13,
    REG_FUNC_VAF_1              = 0x14,
    REG_FUNC_VAF_2              = 0x15,
    REG_FUNC_THERMAL_DETECTION  = 0x17,
};

enum
{
    PAGE_FRAME_1                = 0x00,
    PAGE_FRAME_2                = 0x01,
    PAGE_FUNCTION               = 0x0B,
    PAGE_DETECTION              = 0x0C,
    PAGE_VAF                    = 0x0D,
};

void rgbInit(uint8_t devid, volatile LED_TYPE* states)
{
    static unsigned char led_val[49];
    static unsigned short state = 0;
    unsigned char ret_val = 0;

    switch(state)
    {
        case 0:
            ret_val = i2cWriteReg(devid, REG_CONFIGURE_COMMAND,       PAGE_FUNCTION);
            break;
        case 1:
            ret_val = i2cWriteReg(devid, REG_FUNC_SHUTDOWN,           0x01);
            break;
        case 2:
            ret_val = i2cWriteReg(devid, REG_FUNC_CONFIGURATION,      0x00);
            break;
        case 3:
            ret_val = i2cWriteReg(devid, REG_FUNC_PICTURE_DISPLAY,    0x10);
            break;
        case 4:
            ret_val = i2cWriteReg(devid, REG_FUNC_DISPLAY_OPTION,     0x00);
            break;
        case 5:
            ret_val = i2cWriteReg(devid, REG_FUNC_AUDIO_SYNC,         0x00);
            break;
        case 6:
            ret_val = i2cWriteReg(devid, REG_FUNC_BREATH_CONTROL_1,   0x00);
            break;
        case 7:
            ret_val = i2cWriteReg(devid, REG_FUNC_BREATH_CONTROL_2,   0x00);
            break;
        case 8:
            ret_val = i2cWriteReg(devid, REG_FUNC_AUDIO_GAIN_CONTROL, 0x00);
            break;
        case 9:
            ret_val = i2cWriteReg(devid, REG_FUNC_STAGGERED_DELAY,    0x0F);
            break;
        case 10:
            ret_val = i2cWriteReg(devid, REG_FUNC_SLEW_RATE_CONTROL,  0x01);
            break;
        case 11:
            ret_val = i2cWriteReg(devid, REG_FUNC_VAF_1,              68);
            break;
        case 12:
            ret_val = i2cWriteReg(devid, REG_FUNC_VAF_2,              128);
            break;
        case 13:
            ret_val = i2cWriteReg(devid, REG_FUNC_CURRENT_CONTROL,    153);
            break;
        case 14:
            ret_val = i2cWriteReg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_1);
            break;
        case 15:
            led_val[0] = 0x00;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = 0x00;
                led_val[led_id + 1 + 0x10] = 0x00;
            }

            //Enable Columns 0-2 for the first 13 LEDs
            led_val[0x01] = 0b11111000;
            led_val[0x02] = 0b11111111;

            led_val[0x03] = 0b11111000;
            led_val[0x04] = 0b11111111;

            led_val[0x05] = 0b11111000;
            led_val[0x06] = 0b11111111;

            //Enable Columns 3-5 for the second 13 LEDs
            led_val[0x07] = 0b11000111;
            led_val[0x08] = 0b11111111;

            led_val[0x09] = 0b11000111;
            led_val[0x0A] = 0b11111111;

            led_val[0x0B] = 0b11000111;
            led_val[0x0C] = 0b11111111;

            //Enable Column 6 for the third 13 LEDs (Red and Blue channels)
            led_val[0x0D] = 0b00111111;
            led_val[0x0E] = 0b11111110;
            
            led_val[0x0F] = 0b00111111;
            led_val[0x10] = 0b11111110;

            ret_val = i2cWriteBuf(devid, led_val, 33);
            break;
        case 16:
             ret_val = i2cWriteReg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_2);
             break;
        case 17:
            led_val[0] = 0x00;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = 0x00;
                led_val[led_id + 1 + 0x10] = 0x00;
            }

            //Enable Column 6 for the third 13 LEDs (Green channel)
            led_val[0x01] = 0b00111111;
            led_val[0x02] = 0b11111110;

            //Enable Columns 7-9 for the fourth LEDs
            led_val[0x03] = 0b10000000;
            led_val[0x04] = 0b11111001;

            led_val[0x05] = 0b10000000;
            led_val[0x06] = 0b11111001;

            led_val[0x07] = 0b10000000;
            led_val[0x08] = 0b11111001;

            //Enable Columns 10-12 for the fifth LEDs
            led_val[0x09] = 0b00000000;
            led_val[0x0A] = 0b11001000;

            led_val[0x0B] = 0b00000000;
            led_val[0x0C] = 0b11001000;

            led_val[0x0D] = 0b00000000;
            led_val[0x0E] = 0b11001000;

            ret_val = i2cWriteBuf(devid, led_val, 33);
            break;









        case 18:
            ret_val = i2cWriteReg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_1);
            break;
        // Column 0-2 LEDs starting at address 0x20, RBG order
        case 19:
            led_val[0] = 0x20;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].r;
                led_val[led_id + 1 + 0x10] = states[led_id].b;
                led_val[led_id + 1 + 0x20] = states[led_id].g;
            }

            ret_val = i2cWriteBuf(devid, led_val, 49);
            break;
        // Column 3-5 LEDs starting at address 0x50, RBG order
        case 20:
            led_val[0] = 0x50;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].r;
                led_val[led_id + 1 + 0x10] = states[led_id].b;
                led_val[led_id + 1 + 0x20] = states[led_id].g;
            }

            ret_val = i2cWriteBuf(devid, led_val, 49);
            break;
        // Column 6-8 LEDs (Red and Blue channels) starting at address 0x80, RBG order
        case 21:
            led_val[0] = 0x80;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].r;
                led_val[led_id + 1 + 0x10] = states[led_id].b;
            }

            ret_val = i2cWriteBuf(devid, led_val, 33);
            break;
        case 22:
            ret_val = i2cWriteReg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_2);
            break;
        // Column 6-8 LEDs (Green channel) starting at address 0x20, RBG order
        case 23:
            led_val[0] = 0x20;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].g;
            }

            ret_val = i2cWriteBuf(devid, led_val, 17);
            break;
        // Column 7-9 LEDs starting at address 0x30, RBG order
        case 24:
            led_val[0] = 0x30;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].r;
                led_val[led_id + 1 + 0x10] = states[led_id].b;
                led_val[led_id + 1 + 0x20] = states[led_id].g;
            }

            ret_val = i2cWriteBuf(devid, led_val, 49);
            break;
        // Column 10-12 LEDs starting at address 0x60, RBG order
        case 25:
            led_val[0] = 0x60;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].r;
                led_val[led_id + 1 + 0x10] = states[led_id].b;
                led_val[led_id + 1 + 0x20] = states[led_id].g;
            }

            ret_val = i2cWriteBuf(devid, led_val, 49);
            break;
        //case 24:
        //    ret_val = i2cWriteReg(devid, REG_CONFIGURE_COMMAND,       PAGE_FUNCTION);
        //    break;
        //case 25:
        //    ret_val = i2cWriteReg(devid, REG_FUNC_SHUTDOWN,           0x01);
        //    break;

        default:
            ret_val = 1;
            break;

        case 26:
            ret_val = 0;
            state = 18;
            break;
    }
    if(ret_val == 1)
    {
        state++;
    }
}