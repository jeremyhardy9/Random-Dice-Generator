# Random-Dice-Generator

A bare-metal C project for the STM32F4 microcontroller that simulates rolling a pair of dice. It features custom graphical dice faces generated on a character LCD display, rolling animations, and low-overhead pseudo-random number generation using a Linear Feedback Shift Register (LFSR).

To find main.c:  Core->Src->main.c

## Features

* **Hardware Interrupt Trigger:** Pressing a button connected to `PB9` triggers an external interrupt (`EXTI9_5`), prompting a new dice roll.
* **Custom LCD Graphics:** Uses custom CGRAM character generation to render realistic 3x2 block dice pips on a standard 16x2/20x2 character LCD display.
* **LFSR Pseudo-Random Generation:** Implements a 32-bit Linear Feedback Shift Register (LFSR) for incredibly fast, low-overhead random number generation with a maximum period of over 4.2 billion states.
* **Roll Animations:** Cycles through random sequences to simulate the visual effect of spinning/rolling dice before revealing the final result.
* **Bare-Metal Architecture:** Written using Direct Register Access via the STM32 CMSIS headers for ultra-lean performance without heavy HAL overhead (utilizing HAL only for foundational definitions).

---

## Hardware & Pin Mapping

The project assumes an **STM32F411RE series microcontroller** wired up to a standard Hitachi HD44780-compatible LCD character display in **4-bit mode**.

### GPIO Configuration

| STM32 Pin | Function | Description |
| :--- | :--- | :--- |
| **PA4 - PA7** | LCD Data | Connected to LCD D4 - D7 pins (4-bit data mode) |
| **PA8** | LCD RS | Register Select pin (`0` = Command, `1` = Data) |
| **PA9** | LCD E | Enable pin (pulsed to latch data) |
| **PB9** | Roll Button | Input pin mapped to `EXTI9`, configured with an internal **Pull-Down** resistor (Active-High Logic) |

---

## Dice Rendering Logic

Standard character displays do not natively support graphical dice. To solve this, the firmware breaks each die down into a **3x2 character grid** and designs custom pixel-map arrays to build the pips:

* `pip_for_corner`: Renders standard pips for the outer corners.
* `char_mid_t` & `char_mid_b`: Renders the center pip split seamlessly between the top and bottom character rows.
* `dice_map`: A 2D lookup table that maps any dice value `[1-6]` to the 6 character blocks needed to display it on screen instantly.

---

## Deep Dive into Key Components

### 1. Pseudo-Random Number Generation (LFSR)
The LFSR taps bits 0, 1, 21, and 31 to maintain excellent statistical randomness without requiring a heavy math or hardware RNG library:
```c
uint32_t a1 = lfsr & 1UL;
uint32_t a2 = (lfsr>>1) & 1UL;
uint32_t a22 = (lfsr>>21) & 1UL;
uint32_t a32 = (lfsr>>31) & 1UL;
uint32_t output = a1 ^ a2 ^ a22 ^ a32;
lfsr = (lfsr >> 1) | ((output << 31));
