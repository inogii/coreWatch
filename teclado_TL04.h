#ifndef _TECLADO_TL04_H_
#define _TECLADO_TL04_H_

// INCLUDES
// Propios:
#include "systemConfig.h"

// DEFINES Y ENUMS
// Refresco de excitacion de columnas (milisegundos)
#define TIMEOUT_COLUMNA_TECLADO_MS	10

// Intervalo de guarda anti-rebotes (milisegundos)
// ATENCION: Valor a modificar por el alumno
#define	DEBOUNCE_TIME 200

enum FSM_EXCITACION_TECLADO {
	TECLADO_ESPERA_COLUMNA
};

// FLAGS FSM DEL TECLADO MATRICIAL
// ATENCION: Valores a modificar por el alumno
#define FLAG_TIMEOUT_COLUMNA_TECLADO  	0x01
#define FLAG_TECLA_PULSADA 				0x02

#define NUM_COLUMNAS_TECLADO 	4
#define NUM_FILAS_TECLADO 		4

enum COLUMNAS_TECLADO {
	COLUMNA_1,
	COLUMNA_2,
	COLUMNA_3,
	COLUMNA_4,
};

enum FILAS_TECLADO {
	FILA_1,
	FILA_2,
	FILA_3,
	FILA_4
};


//------------------------------------------------------
// DECLARACIÓN ESTRUCTURAS
typedef struct {
	int columnas[NUM_COLUMNAS_TECLADO]; // Array con los valores BCM de los pines GPIO empleados para cada columna
	int filas[NUM_FILAS_TECLADO]; // Array con los valores BCM de los pines GPIO empleados para cada fila
	tmr_t* tmr_duracion_columna; // Temporizador responsable de medir el tiempo de activacion de cada columna
} TipoTeclado;

/* Estructura que expone la librería para comunicar eventos o el estado del teclado.
 *  La variable que se defina de este tipo ha de ser global, porque a sus elementos
 *  acceden ISRs. Deberá ser static para que solo sea accesible desde dentro de la librería. */
typedef struct {
	int flags; // Flags de eventos del teclado. Está en variable global porque ha de acceder a ella las ISR del teclado.
	int debounceTime[NUM_FILAS_TECLADO]; // Array de variables auxiliares para la implementación de mecanismos anti-rebotes para cada entrada de interrupción
	int columnaActual; // Variable que almacena el valor de la columna que esta activa
	char teclaDetectada; // Variable que almacena el carater que se corresponde con la tecla de la fila/columna pulsada
} TipoTecladoShared;

//------------------------------------------------------
// DECLARACIÓN VARIABLES
extern fsm_trans_t g_fsmTransExcitacionColumnas[];

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
void ConfiguraInicializaTeclado(TipoTeclado *p_teclado);

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
void ActualizaExcitacionTecladoGPIO(int columna);
TipoTecladoShared GetTecladoSharedVar();
void SetTecladoSharedVar(TipoTecladoShared value);

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTimeoutColumna(fsm_t* p_this);

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void TecladoExcitaColumna(fsm_t* p_this);

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void teclado_fila_1_isr(void);
void teclado_fila_2_isr(void);
void teclado_fila_3_isr(void);
void teclado_fila_4_isr(void);
void timer_duracion_columna_isr(union sigval value);

#endif /* _TECLADO_TL04_H_ */
