#include "coreWatch.h"
#include <stdio.h>
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
// Wait until next_activation (absolute time)
// Necesita de la función "delay" de WiringPi.
void DelayUntil(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}


//------------------------------------------------------
// MAIN
//------------------------------------------------------
int main() {
	unsigned int next;
#if VERSION <= 1
	TipoReloj relojPrueba;
	ConfiguraInicializaReloj(&relojPrueba);
	ActualizaHora(&relojPrueba.hora);
	printf("%d:%d:%d\n", relojPrueba.hora.hh, relojPrueba.hora.mm, relojPrueba.hora.ss);
	ActualizaHora(&relojPrueba.hora);
	printf("%d:%d:%d\n", relojPrueba.hora.hh, relojPrueba.hora.mm, relojPrueba.hora.ss);
	relojPrueba.hora.formato = 12;
	SetHora(1290,&relojPrueba.hora);
	printf("%d:%d:%d\n", relojPrueba.hora.hh, relojPrueba.hora.mm, relojPrueba.hora.ss);
	printf("%d/%d/%d\n", relojPrueba.calendario.dd, relojPrueba.calendario.MM, relojPrueba.calendario.yyyy);
	ActualizaFecha(&relojPrueba.calendario);
	printf("%d/%d/%d\n", relojPrueba.calendario.dd, relojPrueba.calendario.MM, relojPrueba.calendario.yyyy);
	ActualizaFecha(&relojPrueba.calendario);
	printf("%d/%d/%d\n", relojPrueba.calendario.dd, relojPrueba.calendario.MM, relojPrueba.calendario.yyyy);
#endif
//	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(relojPrueba));
//	next = millis();
//	while(1){
//		next+=CLK_MS;
//		DelayUntil(next);
//	}
}
