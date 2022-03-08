/*
 * reloj.c
 *
 *  Created on: 13 feb. 2022
 *      Author: pi
 */

#include "reloj.h"
#include <time.h>

// {EstadoIni, FuncCompruebaCondicion, EstadoSig, FuncAccionesSiTransicion}
#define fsm_trans_t  g_fsmTransReloj[] = {{-1, NULL, -1, NULL},{WAIT_TIC, CompruebaTic(), WAIT_TIC, ActualizaReloj()}};
const int DIAS_MESES[2][MAX_MONTH] = {{31,28,31,30,31,30,31,31,30,31,30,31},{31,29,31,30,31,30,31,31,30,31,30,31}};
static TipoRelojShared g_relojSharedVars;

void ResetReloj(TipoReloj *p_reloj){
	TipoCalendario calendario = {DEFAULT_DAY, DEFAULT_MONTH, DEFAULT_YEAR};
	TipoHora hora = {DEFAULT_HOUR, DEFAULT_MIN, DEFAULT_SEC, DEFAULT_TIME_FORMAT};
	p_reloj->calendario = calendario;
	p_reloj->hora = hora;
	p_reloj->timestamp = 0; //default timestamp
	piLock (RELOJ_KEY);
	g_relojSharedVars.flags = 0;
	piUnlock (RELOJ_KEY);
}

int ConfiguraInicializaReloj(TipoReloj *p_reloj){
	ResetReloj(p_reloj);
	tmr_t* temp1 = tmr_new(tmr_actualiza_reloj_isr);
	p_reloj->tmrTic = temp1;
	tmr_startms_periodic(temp1, PRECISION_RELOJ_MS);
	return 0;
}

int CompruebaTic(fsm_t *p_this){
	int result;
	piLock(RELOJ_KEY);
	result = (g_relojSharedVars.flags & FLAG_ACTUALIZA_RELOJ);
	piUnlock(RELOJ_KEY);
	return result;
}

void ActualizaReloj(fsm_t *p_this){
	TipoReloj *p_reloj = (TipoReloj*)(p_this->user_data);
	p_reloj->timestamp += 1;
	ActualizaHora(&p_reloj->hora);
	//si la hora es 00:00:00, hay que actualizar la fecha
	if(p_reloj->hora.hh == 0 && p_reloj->hora.mm == 0 && p_reloj->hora.ss == 0){
		ActualizaFecha(&p_reloj->calendario);
	}
	piLock(RELOJ_KEY);
	g_relojSharedVars.flags = FLAG_RELOJ_ACTUALIZADO;
	piUnlock(RELOJ_KEY);
}

void  tmr_actualiza_reloj_isr (union  sigval  value){
	piLock(RELOJ_KEY);
	g_relojSharedVars.flags = 0x01;
	piUnlock(RELOJ_KEY);
}

void ActualizaFecha(TipoCalendario *p_fecha){
	int diasMes = CalculaDiasMes(p_fecha->MM, p_fecha->yyyy);
	int diaActual = p_fecha->dd;
	int mesActual = p_fecha->MM;
	if(diaActual >= diasMes){
		diaActual = 1;
		if(mesActual >= 12){
			mesActual = 1;
			p_fecha->yyyy++;
		}else{
			mesActual++;
		}
	}else{
		diaActual++;
	}
	p_fecha->MM = mesActual;
	p_fecha->dd = diaActual;
}

void ActualizaHora(TipoHora *p_hora){
	//actualizar segundero
	p_hora->ss = (p_hora->ss+1)%60;
	//actualizar minutero
	if(p_hora->ss == 0) {
		p_hora->mm = (p_hora->mm+1)%60;
		if(p_hora->mm == 0){
			p_hora->hh = p_hora->hh+1;
			if(p_hora->formato == 12 && p_hora->hh >MAX_HOUR_12){
				p_hora->hh = MIN_HOUR_12;
			}
			if(p_hora->formato==24 && p_hora->hh >MAX_HOUR_24){
				p_hora->hh = MIN_HOUR_24;
			}
		}
	}

}

int CalculaDiasMes(int month, int year){
	int bisiesto = EsBisiesto(year);
	return DIAS_MESES[bisiesto][month-1];
}

int EsBisiesto(int year){
	int bisiesto = 0;
	if(((year%4 == 0) && (year%100!=0)) || (year%400 == 0)){
		bisiesto = 1;
	}
	return bisiesto;
}

int SetHora(int horaInt ,TipoHora *p_hora){
	if(horaInt < 0){ return 1;}
	int num_digitos = 0;
	int aux = horaInt;
	while(aux!=0){
		aux /= 10;
		num_digitos++;
	}
	if(num_digitos<1 || num_digitos>4){
		return 2;
	}
	int hora = horaInt/100;
	int minutos = horaInt%100;
	if(hora>MAX_HOUR_24) hora = MAX_HOUR_24;
	if(minutos>MAX_MIN) minutos = MAX_MIN;
	if(p_hora->formato == 12 && hora>12) hora -= 12;
	if(p_hora->formato == 12 && hora == 0) hora = 12;
	p_hora->hh = hora;
	p_hora->mm = minutos;
	p_hora->ss = 0;
	return 0;
}

TipoRelojShared GetRelojSharedVar(){
	piLock(RELOJ_KEY);
	TipoRelojShared copia = g_relojSharedVars;
	piUnlock(RELOJ_KEY);
	return copia;
}

void SetRelojSharedVar(TipoRelojShared value){
	piLock(RELOJ_KEY);
	g_relojSharedVars = value;
	piUnlock(RELOJ_KEY);
}

