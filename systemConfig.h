#ifndef _SYSTEMCONFIG_H_
#define _SYSTEMCONFIG_H_
//------------------------------------------------------
// INCLUDES
// Del sistema
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

// Propios
#include "ent2004cfConfig.h"  // Entorno emulado o de laboratorio. GPIOs.
#include "fsm.h"
#include "tmr.h"

#if ENTORNO == ENTORNO_LABORATORIO
#include "kbhit.h" // Para poder detectar teclas pulsadas sin bloqueo y leer las teclas pulsadas
#include <wiringPi.h>
#include <lcd.h>
#elif ENTORNO == ENTORNO_EMULADO
#include "pseudoWiringPi.h"
#include "pseudoWiringPiDev.h"
#endif

// DEFINES Y ENUMS
#define VERSION	4  // Version del sistema (1, 2, 3, 4...)
#define CLK_MS	10  // Actualizacion de las maquinas de estado (ms)


// DECLARACIÓN DE VARIABLES


// DEFINICIÓN DE VARIABLES


#endif /* SYSTEMCONFIG_H_ */
