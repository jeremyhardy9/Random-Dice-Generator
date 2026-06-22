/*
 * main.c
 *
 *  Created on: Mar 18, 2026
 *  Author: Jeremy Hardy
 *
 * LFSR was used for the random generation due to low overhead and speed
 *
 *
 *
 */
#include "main.h"
#include "stm32f4xx_hal.h"

volatile uint32_t lfsr = 0xACE1u;
volatile uint32_t die1;
volatile uint32_t die2;
int dice_ready = 0;
int Animation_Counter = 0;

/*
 * The following uint8_t global variables below are used for
 * the generation of the dice on the display
 *
 * Each die takes up a 3x2 grid, each block that the MCU writes to
 * must have a specific character displayed on it:
 * empty (nothing), pip for the corners, and the central pip (must have two due to the grid)
 *
 * To be the most efficient I used a dice map, and when our die value is passed to
 * our LCD_2x2 write function, we iterate through it like so: dice_map[die_value][i]
 *
 *
 * */

uint8_t empty[] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};
uint8_t pip_for_corner[] = {0b00000, 0b00000, 0b00000, 0b01110, 0b01110, 0b00000, 0b00000, 0b00000};
uint8_t char_mid_t[] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01110};
uint8_t char_mid_b[] = {0b01110, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};
LCD_CustomChar_map dice_map[7][6] = {
		{LCD_CHAR_BLANK, LCD_CHAR_BLANK, LCD_CHAR_BLANK, LCD_CHAR_BLANK, LCD_CHAR_BLANK, LCD_CHAR_BLANK},
		{LCD_CHAR_BLANK, LCD_CHAR_MID_TOP, LCD_CHAR_BLANK, LCD_CHAR_BLANK, LCD_CHAR_MID_BOT, LCD_CHAR_BLANK},
		{LCD_CHAR_BLANK, LCD_CHAR_BLANK, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_BLANK, LCD_CHAR_BLANK},
		{LCD_CHAR_BLANK, LCD_CHAR_MID_TOP, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_MID_BOT, LCD_CHAR_BLANK},
		{LCD_CHAR_PIP, LCD_CHAR_BLANK, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_BLANK, LCD_CHAR_PIP},
		{LCD_CHAR_PIP, LCD_CHAR_MID_TOP, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_MID_BOT, LCD_CHAR_PIP},
		{LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_PIP, LCD_CHAR_PIP}
};

int main() {
	Hardware_Init();

	LCD_Setup_Characters();

	while(1){
		update_lfsr();
		if (dice_ready){
			__disable_irq();

			LCD_Send_Command(LCD_RESET);
			delay_ms(100);
			LCD_Animation();

			LCD_Display_Die(die1, LCD_DIE1_POS);
			LCD_Display_Die(die2, LCD_DIE2_POS);

			delay_ms(1000);
			dice_ready = 0;

			__enable_irq();
		}
		GPIOA->BSRR = LCD_ALL_PINS_MASK;
	}

	return 0;
}

//Initialization functions
void Hardware_Init() {
	GPIOA_Enable();
	GPIOB_Enable();
	TIM2_ENABLE();
	LCD_init();
}

void LCD_Setup_Characters() {
	LCD_Custom_char(LCD_CHAR_BLANK, empty); delay_ms(1000);
	LCD_Custom_char(LCD_CHAR_PIP, pip_for_corner); delay_ms(1000);
	LCD_Custom_char(LCD_CHAR_MID_TOP, char_mid_t); delay_ms(1000);
	LCD_Custom_char(LCD_CHAR_MID_BOT, char_mid_b); delay_ms(1000);
	LCD_Send_Command(LCD_ROW1_ADD);
}

void LCD_init(void) {
	delay_ms(1000);
	LCD_Send_Command(0x33);
	delay_ms(1000);
	LCD_Send_Command(0x32); //initiate 4 bit mode
	delay_ms(1000);
	LCD_Send_Command(0x28); // set to 4 bit mode
	LCD_Send_Command(0x0C); // Display on, Cursor off, Blink off
	LCD_Send_Command(0x06); // Set Cursor to Move right after righting
	LCD_Send_Command(LCD_RESET); // Clear display
	delay_ms(1000);
}

void GPIOA_Enable() {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	GPIOA->MODER &= ~(GPIO_MODER_MODE4_Msk|GPIO_MODER_MODE5_Msk|GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk|GPIO_MODER_MODE8_Msk|GPIO_MODER_MODE9_Msk);
	GPIOA->MODER |= GPIO_MODER_MODE4_0|GPIO_MODER_MODE5_0|GPIO_MODER_MODE6_0|GPIO_MODER_MODE7_0|GPIO_MODER_MODE8_0|GPIO_MODER_MODE9_0;
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT4_Msk|GPIO_OTYPER_OT5_Msk|GPIO_OTYPER_OT6_Msk|GPIO_OTYPER_OT7_Msk|GPIO_OTYPER_OT8_Msk|GPIO_OTYPER_OT9_Msk);
}

void GPIOB_Enable() {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable peripheral clock for GPIOB
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Enable SYSCFG for interrupts

	GPIOB->MODER &= ~GPIO_MODER_MODE9_Msk; // (00) input mode for PB9
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD9_Msk; // (00) Reset PUPDR configure to No pull-up pull-down
	GPIOB->PUPDR |= GPIO_PUPDR_PUPD9_1; // 01 --> Pull down enable, Active High Logic

	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9_Msk; // Clear configuration for EXTI9
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI9_PB; // Map port B to EXTI9

	EXTI->IMR |= EXTI_IMR_IM9; // Un-mask EXTI9 (enable it)
	EXTI->FTSR &= ~EXTI_FTSR_TR9; // Disable Falling Edge Trigger
	EXTI->RTSR |= EXTI_RTSR_TR9; // Rising Edge Enabled

	NVIC_EnableIRQ(EXTI9_5_IRQn); 	//Enable interrupt in NVIC
}

void TIM2_ENABLE(void) {
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable clock
	/* formula: counter clock frequency = fclk/PSC[15:0]+1 TIMx_PSC
	 clock frequency is 16Mhz.*/

	TIM2->PSC = 16-1; // Each clock cycle is 1us
	TIM2->ARR = 0xFFFFFFFF; // Maximum value of counter --> counter can count for a very long time

	TIM2->EGR |= TIM_EGR_UG; // Update Generation bit on --> forces hardware to take on new settings immiedetley
	TIM2->SR &= ~TIM_SR_UIF_Msk; //clear update flag --> ensure that it will run, CPU checks bit
	TIM2->CR1 |= TIM_CR1_CEN; //counter enabled
}

//called by hardware
void EXTI9_5_IRQHandler() {
	if(EXTI->PR & EXTI_PR_PR9) {
		delay_ms(10);
		//Forces a Re-roll if the result is not a valid die number (1-6)
		do {
			update_lfsr();
			die1 = lfsr & 0x7;
			die2 = (lfsr >> 3) & 0x7;
		} while((die1<1) || (die1 > 6) || (die2 < 1) || (die2 > 6));
	}
	dice_ready = 1;
	EXTI->PR |= EXTI_PR_PR9; // Send a bit to signal that the interrupt has been handled
}

void delay_ms(uint32_t ms) {
	TIM2->CNT = 0;
	//each tick is 1us --> 1000 ticks = 1ms
	while (TIM2->CNT < (1000*ms)) {}
}

void update_lfsr() {
	/* Specific Bits are "tapped" and then Xored with eachother to generate a knew 32 bit number
	 * This ensures that it doesn't repeat for 4.2 billion numbers */
	uint32_t a1 = lfsr & 1UL;
	uint32_t a2 = (lfsr>>1) & 1UL;
	uint32_t a22 = (lfsr>>21) & 1UL;
	uint32_t a32 = (lfsr>>31) & 1UL;
	uint32_t output = a1 ^ a2 ^ a22 ^ a32;
	lfsr = (lfsr >> 1) | ((output << 31));
}

/*
 * pa8 = R, E = pa9, d4-d7 = pa4-pa7
 * R = 1 is to send data on screen
 * E = 1 is to send COMMANDS on screen
 *
 */
void write_to_lcd_display(char c) {
	GPIOA->BSRR = GPIO_BSRR_BS8;
	uint8_t high_nibble = (c>>4)&(LCD_DATA_MASK); //Takes the 4 MSB and turns it into a nibble by masking it with 0000 1111
	uint8_t low_nibble = (c&LCD_DATA_MASK); //Takes the 4 LSB and turns it into a nibble by masking it with 0000 1111

	GPIOA->BSRR = LCD_DATA_PINS_MASK; //Reset data pins (pa4-pa7)
	GPIOA->BSRR = high_nibble << GPIOA_PIN_POS; //(send high nibble to data pins)
	lcd_pulse();

	GPIOA->BSRR = LCD_DATA_PINS_MASK;
	GPIOA->BSRR = low_nibble << GPIOA_PIN_POS;
	lcd_pulse();
}

//PA8 = Reset, PA9 = Enable
void LCD_Send_Command(uint8_t command) {
	GPIOA->BSRR = GPIO_BSRR_BR8; //Makes sure that Rs = 0
	uint8_t high_nibble = (command>>4)&(LCD_DATA_MASK);
	uint8_t low_nibble = (command&LCD_DATA_MASK);

	GPIOA->BSRR = LCD_ALL_PINS_MASK;
	GPIOA->BSRR = high_nibble << GPIOA_PIN_POS;
	lcd_pulse(); //setting Enable to high allows for displaying of characters on screen

	GPIOA->BSRR = LCD_ALL_PINS_MASK;
	GPIOA->BSRR = low_nibble << GPIOA_PIN_POS;
	lcd_pulse();
}

void LCD_Custom_char(uint8_t location, uint8_t *character) {
	location &= (LCD_CUSTOM_CHAR_SLOTS-1);
	uint8_t custom_char_adress = LCD_SET_CUSTOM_CHAR_ADD|location<<3;
	LCD_Send_Command(custom_char_adress);
	for(int i=0; i<CHARACTER_HEIGHT;i++) {
		write_to_lcd_display(character[i]);
	}
}
void LCD_Display_Die(uint8_t dice, uint8_t start_col) {
	LCD_Send_Command(LCD_ROW1_ADD+start_col);
	for (int i = 0;i<3;i++) {
		write_to_lcd_display(dice_map[dice][i]);
	}
	LCD_Send_Command(LCD_ROW2_ADD+start_col);
	for (int i = 3;i<6;i++) {
			write_to_lcd_display(dice_map[dice][i]);
		}
}

void LCD_Animation(void) {
	uint8_t sequence1[4][10] = {{2, 5, 1, 6, 3, 2, 6, 4, 1, 5},
							{2, 1, 3, 4, 3, 4, 5, 3, 1, 1},
							{5, 2, 2, 1, 2, 5, 3, 5, 6, 5},
							{5, 2, 5, 1, 1, 1, 5, 2, 6, 5}};
	uint8_t sequence2[4][10] = {{6, 5, 5, 2, 1, 1, 6, 3, 6, 6},
							{4, 1, 1, 3, 1, 6, 2, 4, 3, 5},
							{3, 4, 5, 2, 6, 4, 5, 3, 6, 6},
							{5, 5, 6, 6, 2, 4, 2, 2, 3, 4},
	};
	for (int i = 0; i < 10; i++) {
		LCD_Display_Die(sequence1[Animation_Counter][i], LCD_DIE1_POS);
		LCD_Display_Die(sequence2[Animation_Counter][i], LCD_DIE2_POS);
		delay_ms(100);
	}
	Animation_Counter++;
	if (Animation_Counter > 3) {
		Animation_Counter = 0;
	}

}

void lcd_pulse(void) {
	GPIOA->BSRR = GPIO_BSRR_BS9;
	delay_ms(1);
	GPIOA->BSRR = GPIO_BSRR_BR9;
	delay_ms(1);
}
