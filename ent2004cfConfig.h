#ifndef _ENT2004_CF_CONFIG_H_
#define _ENT2004_CF_CONFIG_H_

// Entorno emulado o laboratorio:
#define ENTORNO_EMULADO 0 // NO MODIFICAR!
#define ENTORNO_LABORATORIO 1 // NO MODIFICAR!

/* Setup de perifericos disponibles:
 * 	Teclado TL04 + LED display 7x10
 * 	Teclado TL04 + LCD 2x12
 * */
#define SETUP_MATRIZ_LED 0 // NO MODIFICAR!
#define SETUP_LCD 1  // NO MODIFICAR!

/* Alumno elige el entorno y el setup de perifericos:
 * 	ENTORNO_EMULADO  o ENTORNO_LABORATORIO
 * 	SETUP_MATRIZ_LED o SETUP_LCD
 * */
#define ENTORNO ENTORNO_LABORATORIO
#define SETUP SETUP_LCD


// CLAVES PARA MUTEX
/* 'key'(s) que podemos bloquear (lock) y desbloquear (unlock). Sus valores pueden ir de
 * 0 a 3 (sin repetirse). Solo 4 posibles mutex. Esto se interpreta internamente como un
 * "pthread_mutex" por la libreria wiringPi.
 * */
#define STD_IO_LCD_BUFFER_KEY 0
#define RELOJ_KEY 1
#define KEYBOARD_KEY 2
#define SYSTEM_KEY 3


// GPIOs para el teclado, display de LEDs y el LCD
#define GPIO_KEYBOARD_COL_1 0
#define GPIO_KEYBOARD_COL_2 1
#define GPIO_KEYBOARD_COL_3 2
#define GPIO_KEYBOARD_COL_4 3
#define GPIO_KEYBOARD_ROW_1 5
#define GPIO_KEYBOARD_ROW_2 6
#define GPIO_KEYBOARD_ROW_3 12
#define GPIO_KEYBOARD_ROW_4 13

#if SETUP == SETUP_MATRIZ_LED
#define GPIO_LED_DISPLAY_COL_1 0
#define GPIO_LED_DISPLAY_COL_2 0
#define GPIO_LED_DISPLAY_COL_3 0
#define GPIO_LED_DISPLAY_COL_4 0
#define GPIO_LED_DISPLAY_ROW_1 0
#define GPIO_LED_DISPLAY_ROW_2 0
#define GPIO_LED_DISPLAY_ROW_3 0
#define GPIO_LED_DISPLAY_ROW_4 0
#define GPIO_LED_DISPLAY_ROW_5 0
#define GPIO_LED_DISPLAY_ROW_6 0
#define GPIO_LED_DISPLAY_ROW_7 0

#elif SETUP == SETUP_LCD
#define GPIO_LCD_D0 11
#define GPIO_LCD_D1 14
#define GPIO_LCD_D2 17
#define GPIO_LCD_D3 18
#define GPIO_LCD_D4 22
#define GPIO_LCD_D5 23
#define GPIO_LCD_D6 24
#define GPIO_LCD_D7 25
#define GPIO_LCD_RS 8
#define GPIO_LCD_EN 10

#endif /* SETUP */

#endif /* ENT2004_CF_CONFIG_H_ */
