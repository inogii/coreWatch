#include "teclado_TL04.h"

const char tecladoTL04[NUM_FILAS_TECLADO][NUM_COLUMNAS_TECLADO] = {
		{'1', '2', '3', 'C'},
		{'4', '5', '6', 'D'},
		{'7', '8', '9', 'E'},
		{'A', '0', 'B', 'F'}
};

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t g_fsmTransExcitacionColumnas[] = {
		{ TECLADO_ESPERA_COLUMNA, CompruebaTimeoutColumna, TECLADO_ESPERA_COLUMNA, TecladoExcitaColumna },
		{-1, NULL, -1, NULL },
};

static TipoTecladoShared g_tecladoSharedVars;
typedef void(*func_type)(void);
static func_type interrupciones[4] = {teclado_fila_1_isr, teclado_fila_2_isr, teclado_fila_3_isr, teclado_fila_4_isr};
static int dbtimeinitial[4] = {0,0,0,0};
static int columnaActual = 0;

//------------------------------------------------------
// FUCNIONES DE INICIALIZACION DE LAS VARIABLES ESPECIFICAS
//------------------------------------------------------
void ConfiguraInicializaTeclado(TipoTeclado *p_teclado) {
// A completar por el alumno...

	// Inicializacion de elementos de la variable global de tipo TipoTecladoShared:
	// 1. Valores iniciales de todos los "debounceTime"
	// 2. Valores iniciales de todos "columnaActual", "teclaDetectada" y "flags"
	g_tecladoSharedVars.flags = 0;
	memcpy(g_tecladoSharedVars.debounceTime, dbtimeinitial, sizeof(dbtimeinitial));
	g_tecladoSharedVars.columnaActual = 0;
	g_tecladoSharedVars.teclaDetectada = 0;

	// Inicializacion de elementos de la estructura TipoTeclado:
	// Inicializacion del HW:
	// 1. Configura GPIOs de las columnas:
	// 	  (i) Configura los pines y (ii) da valores a la salida
	int i;
	for(i = 0; i<NUM_COLUMNAS_TECLADO; i++){
		pinMode(p_teclado->columnas[i], OUTPUT);
		pinMode(p_teclado->filas[i], INPUT);
		pullUpDnControl(p_teclado->filas[i], PUD_DOWN);
		wiringPiISR(p_teclado->filas[i], INT_EDGE_RISING, interrupciones[i]);
	}
	// 2. Configura GPIOs de las filas:
	// 	  (i) Configura los pines y (ii) asigna ISRs (y su polaridad)
	//
	// Inicializacion del temporizador:
	// 3. Crear y asignar temporizador de excitacion de columnas
	tmr_t* temp1 = tmr_new(timer_duracion_columna_isr);
	p_teclado->tmr_duracion_columna = temp1;
	// 4. Lanzar temporizador
	tmr_startms(temp1, TIMEOUT_COLUMNA_TECLADO_MS);
}

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
/* Getter y setters de variables globales */
TipoTecladoShared GetTecladoSharedVar() {
	TipoTecladoShared result;
	piLock(KEYBOARD_KEY);
		result = g_tecladoSharedVars;
	piUnlock(KEYBOARD_KEY);
	return result;
}
void SetTecladoSharedVar(TipoTecladoShared value) {
	piLock(KEYBOARD_KEY);
		g_tecladoSharedVars = value;
	piUnlock(KEYBOARD_KEY);
}

void ActualizaExcitacionTecladoGPIO(int columna) {
	// ATENCION: Evitar que este mas de una columna activa a la vez.

	switch(columna){
	case 0:
		digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
		digitalWrite(GPIO_KEYBOARD_COL_1, HIGH);
		break;
	case 1:
		digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
		digitalWrite(GPIO_KEYBOARD_COL_2, HIGH);
		break;
	case 2:
		digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
		digitalWrite(GPIO_KEYBOARD_COL_3, HIGH);
		break;
	case 3:
		digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
		digitalWrite(GPIO_KEYBOARD_COL_4, HIGH);
		break;
	}
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTimeoutColumna(fsm_t* p_this) {
	int result = 0;
	piLock(KEYBOARD_KEY);
		result = g_tecladoSharedVars.flags & FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
	return result;
}


//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LAS MAQUINAS DE ESTADOS
//------------------------------------------------------
void TecladoExcitaColumna(fsm_t* p_this) {
	TipoTeclado *p_teclado = (TipoTeclado*)(p_this->user_data);

	// 1. Actualizo que columna SE VA a excitar
	// 2. Ha pasado el timer y es hora de excitar la siguiente columna:
	//    (i) Llamada a ActualizaExcitacionTecladoGPIO con columna A ACTIVAR como argumento
	// 3. Actualizar la variable flags
	// 4. Manejar el temporizador para que vuelva a avisarnos

	columnaActual++;
	columnaActual = columnaActual%4;
	ActualizaExcitacionTecladoGPIO(columnaActual);
	piLock(KEYBOARD_KEY);
		g_tecladoSharedVars.flags = g_tecladoSharedVars.flags & (~FLAG_TIMEOUT_COLUMNA_TECLADO);
	piUnlock(KEYBOARD_KEY);
	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO_MS);

}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void teclado_fila_1_isr(void) {
	// 1. Comprobar si ha pasado el tiempo de guarda de anti-rebotes
	int now = millis();
	if(now < g_tecladoSharedVars.debounceTime[0]){
		g_tecladoSharedVars.debounceTime[0] = millis() + DEBOUNCE_TIME;
		return;
	}
	// 2. Atender a la interrupcion:
	// 	  (i) Guardar la tecla detectada en g_tecladoSharedVars
	//    (ii) Activar flag para avisar de que hay una tecla pulsada
	piLock(KEYBOARD_KEY);
		g_tecladoSharedVars.teclaDetectada = tecladoTL04[0][columnaActual];
		g_tecladoSharedVars.flags = g_tecladoSharedVars.flags | FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);

	// 3. Actualizar el tiempo de guarda del anti-rebotes

	g_tecladoSharedVars.debounceTime[0] = now + DEBOUNCE_TIME;
}

void teclado_fila_2_isr(void) {
	int now = millis();
	if(now < g_tecladoSharedVars.debounceTime[1]){
		g_tecladoSharedVars.debounceTime[1] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[1][columnaActual];
	g_tecladoSharedVars.flags = g_tecladoSharedVars.flags | FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	g_tecladoSharedVars.debounceTime[1] = now + DEBOUNCE_TIME;
}

void teclado_fila_3_isr(void) {
	int now = millis();
	if(now < g_tecladoSharedVars.debounceTime[2]){
		g_tecladoSharedVars.debounceTime[2] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[2][columnaActual];
	g_tecladoSharedVars.flags = g_tecladoSharedVars.flags | FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	g_tecladoSharedVars.debounceTime[2] = now + DEBOUNCE_TIME;
}

void teclado_fila_4_isr (void) {
	int now = millis();
	if(now < g_tecladoSharedVars.debounceTime[3]){
		g_tecladoSharedVars.debounceTime[3] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[3][columnaActual];
	g_tecladoSharedVars.flags = g_tecladoSharedVars.flags | FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	g_tecladoSharedVars.debounceTime[3] = now + DEBOUNCE_TIME;
}

void timer_duracion_columna_isr(union sigval value) {
	// Simplemente avisa que ha pasado el tiempo para excitar la siguiente columna
	piLock(KEYBOARD_KEY);
		g_tecladoSharedVars.flags = g_tecladoSharedVars.flags | FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);

}
