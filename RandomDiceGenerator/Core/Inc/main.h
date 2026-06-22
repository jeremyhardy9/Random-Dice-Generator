#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx_hal.h"

/* LCD Hardware Mapping */
#define LCD_RS_PIN 8
#define LCD_EN_PIN 9
#define LCD_DATA_MASK 0X0F
#define GPIOA_PIN_POS 4

/* Dice Constants and Position Values*/
#define LCD_DIE1_POS 3
#define LCD_DIE2_POS 8

/* LCD Controller Constants*/
#define LCD_RESET 0x01
#define LCD_ROW1_ADD 0x80
#define LCD_ROW2_ADD 0xC0
#define LCD_CUSTOM_CHAR_SLOTS 8
#define LCD_SET_CUSTOM_CHAR_ADD 0x40 //starting address for custom char characters in lcd
#define CHARACTER_HEIGHT 8 //each char is 8 rows high

/* Custom Character Slots*/
typedef enum {
    LCD_CHAR_BLANK = 0,
    LCD_CHAR_PIP,
    LCD_CHAR_MID_TOP,
    LCD_CHAR_MID_BOT,
} LCD_CustomChar_map;

/* Bit-mask Hardware */
#define LCD_ALL_PINS_MASK (GPIO_BSRR_BR4 | GPIO_BSRR_BR5 | GPIO_BSRR_BR6 | \
                           GPIO_BSRR_BR7 | GPIO_BSRR_BR8 | GPIO_BSRR_BR9)
#define LCD_ALL_PINS_ON (GPIO_BSRR_BS4 | GPIO_BSRR_BS5 | GPIO_BSRR_BS6 | \
                           GPIO_BSRR_BS7 | GPIO_BSRR_BS8 | GPIO_BSRR_BS9)
#define LCD_DATA_PINS_MASK (GPIO_BSRR_BR4 | GPIO_BSRR_BR5 | GPIO_BSRR_BR6 | GPIO_BSRR_BR7)


/* Initialization & Hardware */
void TIM2_ENABLE(void);
void Hardware_Init(void);
void GPIOA_Enable(void);
void GPIOB_Enable(void);

// High Level LCD Functions
void LCD_init(void);
void LCD_Setup_Characters(void);
void LCD_Display_Die(uint8_t, uint8_t);
void LCD_Animation(void);
void delay_ms(uint32_t);

// Low level LCD Functions (helpers)
void write_to_lcd_display(char);
void lcd_pulse(void);
void LCD_Send_Command(uint8_t);
void LCD_Custom_char(uint8_t, uint8_t*);

// Logic and Interrupts
void EXTI9_5_IRQHandler(void);
void update_lfsr(void);

#endif /* __MAIN_H */
