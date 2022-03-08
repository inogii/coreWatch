/*
 * reloj.h
 *
 *  Created on: 13 feb. 2022
 *      Author: pi
 */

#ifndef RELOJ_H_
#define RELOJ_H_
// INCLUDES
#include "systemConfig.h"
#include "util.h"
// DEFINES Y ENUMS
enum FSM_ESTADOS_RELOJ{
	WAIT_TIC
};
// FLAGS FSM
#define FLAG_ACTUALIZA_RELOJ 0x01
#define FLAG_RELOJ_ACTUALIZADO 0x02
// DECLARACION ESTRUCTURAS
typedef  struct {int dd;int MM;int  yyyy;} TipoCalendario;
typedef  struct {int hh;int mm;int ss;int  formato;} TipoHora;
typedef  struct {int  timestamp;TipoHora  hora;TipoCalendario  calendario;tmr_t* tmrTic;} TipoReloj;
typedef  struct {int  flags;} TipoRelojShared;
// DECLARACION VARIABLES
//asociadas a TipoCalendario
#define MIN_DAY 1
#define MIN_MONTH 1
#define MAX_MONTH 12
#define MIN_YEAR 1970
//asociadas a TipoReloj
#define MIN_HOUR_12 1
#define MIN_HOUR_24 0
#define MAX_HOUR_12 12
#define MAX_HOUR_24 23
#define MAX_MIN 59
//array de tabla de transiciones de fsm
extern  fsm_trans_t  g_fsmTransReloj [];
//array de numero de dias de cada mes para años bisiestos y no bisiestos
extern const int DIAS_MESES[2][MAX_MONTH];
// DEFINICION VARIABLES
#define DEFAULT_DAY 31
#define DEFAULT_MONTH 12
#define DEFAULT_YEAR 2020
#define DEFAULT_HOUR 23
#define DEFAULT_MIN 59
#define DEFAULT_SEC 58
#define DEFAULT_TIME_FORMAT 24
#define PRECISION_RELOJ_MS 1000
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
int ConfiguraInicializaReloj (TipoReloj *p_reloj);
void ResetReloj(TipoReloj *p_reloj);
// FUNCIONES PROPIAS
void ActualizaFecha(TipoCalendario *p_fecha);
void ActualizaHora(TipoHora *p_hora);
int CalculaDiasMes(int month, int year);
int EsBisiesto(int year);
TipoRelojShared GetRelojSharedVar();
int SetFecha(int nuevaFecha, TipoCalendario *p_fecha);
int SetFormato(int nuevoFormato, TipoHora *p_hora);
int SetHora(int nuevaHora, TipoHora *p_hora);
void SetRelojSharedVar(TipoRelojShared value);
// FUNCIONES DE ENTRADA DE LA MAQUINA DE ESTADOS
int CompruebaTic(fsm_t *p_this);
// FUNCIONES DE SALIDA DE LA MAQUINA DE ESTADOS
void ActualizaReloj(fsm_t *p_this);
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
void tmr_actualiza_reloj_isr(union sigval value);
// FUNCIONES LIGADAS A THREADS ADICIONALES

#endif /* RELOJ_H_ */
