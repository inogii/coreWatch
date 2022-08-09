/*
 * reloj.c
 *
 *  Created on: 13 feb. 2022
 *      Author: pi
 */

#include "reloj.h"
#include <time.h>

// {EstadoIni, FuncCompruebaCondicion, EstadoSig, FuncAccionesSiTransicion}
fsm_trans_t  g_fsmTransReloj[] = {{WAIT_TIC, CompruebaTic, WAIT_TIC, ActualizaReloj},{-1, NULL, -1, NULL}};
const int DIAS_MESES[2][MAX_MONTH] = {{31,28,31,30,31,30,31,31,30,31,30,31},{31,29,31,30,31,30,31,31,30,31,30,31}};
static TipoRelojShared g_relojSharedVars;

void ResetReloj(TipoReloj *p_reloj){
	//Obtenemos fecha y hora actual
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	int horaActual = timeinfo->tm_hour;
	int minActual = timeinfo->tm_min;
	int segActual = timeinfo->tm_sec;
	int diaActual = timeinfo->tm_mday;
	int mesActual = timeinfo->tm_mon +1; //REFERENCIADO DE 0 A 11
	int yearActual = timeinfo->tm_year + 1900; //el year obtenido así es referenciado a 1900
	int weekDay = timeinfo->tm_wday;
	printf("Weekday: %d", weekDay);
	TipoCalendario calendario = {diaActual, mesActual, yearActual, weekDay};
	TipoHora hora = {horaActual, minActual, segActual, DEFAULT_TIME_FORMAT};
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
	//00 01 10 11
	//ENTRAS CON flags = 01
	//SALES CON flags = 10
	piLock(RELOJ_KEY);
		g_relojSharedVars.flags = FLAG_TIME_ACTUALIZADO;
	piUnlock(RELOJ_KEY);


#if VERSION == 1
		int hora = p_reloj->hora.hh;
		int min = p_reloj->hora.mm;
		int seg = p_reloj->hora.ss;
		int dia = p_reloj->calendario.dd;
		int mes = p_reloj->calendario.MM;
		int year = p_reloj->calendario.yyyy;
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("%d:%d:%d", hora, min, seg);
		printf(" del %d/%d/%d\n", dia, mes, year);
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
#endif
}

void  tmr_actualiza_reloj_isr (union  sigval  value){
	piLock(RELOJ_KEY);
		g_relojSharedVars.flags = FLAG_ACTUALIZA_RELOJ;
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
	int weekDay = p_fecha->weekDay%6;
	weekDay++;
	p_fecha->weekDay = weekDay;
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
			if(p_hora->hh > MAX_HOUR_24){
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
	//obtenemos los valores de hora y minutos introducidos
	int hora = horaInt/100;
	int minutos = horaInt%100;
	//si los mins exceden el maximo, se asigna el valor max_min
	if(minutos>MAX_MIN) minutos = MAX_MIN;
	if(hora>MAX_HOUR_24) hora = MAX_HOUR_24;
	//Asignación de valores
	p_hora->hh = hora;
	p_hora->mm = minutos;
	p_hora->ss = 0;
	return 0;
}

int SetDia(int dia, TipoCalendario *p_calendario){
	//El dia introducido no puede ser 0 ni inferior
	if(dia <= 0){
		piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Introduzca un día válido");
			fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		return 1;
	}
	//Comprobamos que se han introducido 1 o 2 digitos
	int num_digitos = 0;
	int aux = dia;
	while(aux!=0){
		aux /= 10;
		num_digitos++;
	}
	if(num_digitos < 1|| num_digitos>2){
		piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Introduzca un dia de maximo 2 digitos");
			fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		return 2;
	}
	int mesActual = p_calendario->MM;
	int yearActual = p_calendario->yyyy;
	int maxDiaMes = CalculaDiasMes(mesActual, yearActual);
	if(dia > maxDiaMes){
		dia = maxDiaMes;
	}
	p_calendario->dd = dia;
	return 0;

}

int SetMes(int mes, TipoCalendario *p_calendario){
	//El día introducido no puede ser 0 ni inferior
	if(mes <= 0){
		piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Introduzca un mes valido");
			fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		return 1;
	}
	//Comprobamos que se han introducido 1 o 2 dígitos
	int num_digitos = 0;
	int aux = mes;
	while(aux!=0){
		aux /= 10;
		num_digitos++;
	}
	if(num_digitos < 1|| num_digitos>2){
		piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Introduzca un mes de maximo 2 digitos");
			fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		return 2;
	}
	if(mes > MAX_MONTH){
		mes = MAX_MONTH;
	}
	p_calendario->MM = mes;
	return 0;
}

int SetYear(int year, TipoCalendario *p_calendario){
	//El día introducido no puede ser 0 ni inferior
	if(year <= 0){
		piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Introduzca un year d.C.");
			fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		return 1;
	}
	if(year < MIN_YEAR){
		year = MIN_YEAR;
	}
	p_calendario->yyyy = year;
	return 0;
}

int SetFormat(int formato, TipoHora *p_hora){
	if(formato != 12 && formato != 24){
		formato = 24;
	}
	p_hora->formato = formato;
	return 0;
}

int CalculaDiaSemana(int dia, int mes, int year ){
	// int64_t res = CalculaSegundosEpoch(dia, mes, year);
	int res = CalculaSegundosEpoch(dia, mes, year);
	time_t segs = (time_t)(res);
	struct tm * timeinfo;
	timeinfo = localtime(&segs);
	printf("%d", res);
	printf("%s", asctime(timeinfo));
	int weekDay = timeinfo->tm_wday;
	return weekDay;
}

int64_t CalculaSegundosEpoch(int dia, int mes, int year){
	int esbisiesto = EsBisiesto(year);
	int64_t bisiestos = CalculaBisiestosDesdeEpochHasta(year);
	int64_t yearToDays = bisiestos*366 + (year-1970-bisiestos)*365;
	int64_t monthToDays =0;
	int i;
	for(i=0; i<mes-1; i++){
		monthToDays += DIAS_MESES[esbisiesto][i];
	}
	int64_t totalDiasEnSeg = 0;
	totalDiasEnSeg = ((dia-1)+monthToDays+yearToDays)*24*3600;
	return totalDiasEnSeg;
}

int CalculaBisiestosDesdeEpochHasta(int year){
	int i;
	int cont;
	for(i = 1970; i<year; i++){
		cont += EsBisiesto(i);
	}
	return cont;
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

