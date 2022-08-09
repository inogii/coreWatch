#include "coreWatch.h"
#include <stdio.h>
#include <string.h>
//------------------------------------------------------
// VARIABLES GLOBALES
//------------------------------------------------------
static TipoCoreWatch g_coreWatch;
static int defaultPassword = 1234;
static int defaultHoraAlarma = 0;
static int defaultAlarmaActivada = 0;
static int g_flagsCoreWatch;
fsm_trans_t g_fsmTransCoreWatch[] = {
			//START
			{START, CompruebaSetupDone, STAND_BY, Start},
			//STAND_BY
			//{STAND_BY, CompruebaReset, STAND_BY, Reset},
			{STAND_BY,CompruebaSetCancelNewTime, PASSWORD, PrepareEnterPassword},
			{STAND_BY,CompruebaSetCancelNewDate, PASSWORD, PrepareEnterPassword},
			{STAND_BY,CompruebaSetCancelNewAlarma, PASSWORD, PrepareEnterPassword},
			{STAND_BY,CompruebaSetPassword, PASSWORD, PrepareEnterPassword},
			{STAND_BY,CompruebaHoraAlarma, ALARMA, SuenaAlarma},
			{STAND_BY,CompruebaSwitchFormat, STAND_BY, SwitchFormat},
			{STAND_BY, CompruebaTimeActualizado, STAND_BY, ShowTime},
			//PASSWORD
			{PASSWORD, CompruebaDigitoPulsado, PASSWORD, EnterPassword},
			{PASSWORD, CompruebaPasswordTime, SET_TIME, PrepareSetNewTime},
			{PASSWORD, CompruebaPasswordDate, SET_DATE, PrepareSetNewDate},
			{PASSWORD, CompruebaPasswordAlarma, SET_ALARMA, PrepareSetAlarma},
			{PASSWORD, CompruebaPasswordPassword, SET_PASSWORD, PrepareSetNewPassword},
			{PASSWORD, CompruebaTimeoutPassword, STAND_BY, CancelEnterPassword},
			//SET_PASSWORD
			{SET_PASSWORD, CompruebaSetPassword, STAND_BY, CancelSetNewPassword},
			{SET_PASSWORD, CompruebaNewPasswordIsReady, STAND_BY, SetNewPassword},
			{SET_PASSWORD, CompruebaDigitoPulsado, SET_PASSWORD, EnterNewPassword},
			//SET_TIME
			{SET_TIME, CompruebaSetCancelNewTime, STAND_BY, CancelSetNewTime},
			{SET_TIME, CompruebaNewTimeIsReady, STAND_BY, SetNewTime},
			{SET_TIME, CompruebaDigitoPulsado, SET_TIME, ProcesaDigitoTime},
			//SET_DATE
			{SET_DATE, CompruebaNewDateIsReady, STAND_BY, SetNewDate},
			{SET_DATE, CompruebaSetCancelNewDate, STAND_BY, CancelSetNewDate},
			{SET_DATE, CompruebaDigitoPulsado, SET_DATE, ProcesaDigitoDate},
			//SET_ALARMA
			{SET_ALARMA, CompruebaSetCancelNewAlarma, STAND_BY, CancelSetNewAlarma},
			{SET_ALARMA, CompruebaNewAlarmaIsReady, STAND_BY, SetNewAlarma},
			{SET_ALARMA, CompruebaDigitoPulsado, SET_ALARMA, ProcesaDigitoAlarma},
			//ALARMA
			{ALARMA, CompruebaTimeoutAlarma, STAND_BY, ExitAlarma},
			{-1, NULL, -1, NULL}};
fsm_trans_t g_fsmTransDeteccionComandos[] = {
		{WAIT_COMMAND,CompruebaTeclaPulsada ,WAIT_COMMAND, ProcesaTeclaPulsada},
		{-1, NULL, -1, NULL}};
//am = 0 pm = 1
const int hora_ampm[2][24] = {
		{12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};
const char diasemana[7] = {'D', 'L', 'M', 'X', 'J', 'V', 'S'};

int rows = 2;
int cols = 12;
int bits = 8;
int rs = GPIO_LCD_RS;
int strb = GPIO_LCD_EN;
int d0 = GPIO_LCD_D0;
int d1 = GPIO_LCD_D1;
int d2 = GPIO_LCD_D2;
int d3 = GPIO_LCD_D3;
int d4 = GPIO_LCD_D4;
int d5 = GPIO_LCD_D5;
int d6 = GPIO_LCD_D6;
int d7 = GPIO_LCD_D7;


static int arrayFilas[4] = {GPIO_KEYBOARD_ROW_1, GPIO_KEYBOARD_ROW_2, GPIO_KEYBOARD_ROW_3, GPIO_KEYBOARD_ROW_4};
static int arrayColumnas[4] = {GPIO_KEYBOARD_COL_1, GPIO_KEYBOARD_COL_2, GPIO_KEYBOARD_COL_3, GPIO_KEYBOARD_COL_4};
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
// Wait until next_activation (absolute time)
// Necesita de la funci�n "delay" de WiringPi.
void DelayUntil(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}

int min(int a, int b){
	return (a > b) ? b : a;
}

int max(int a, int b){
	return (a > b) ? a : b;
}

int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema){
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = 0;
	piUnlock(SYSTEM_KEY);
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
	p_sistema->tempDate = 0;
	p_sistema->digitosGuardadosDate = 0;
	p_sistema->password.tempPassword = 0;
	p_sistema->password.digitosGuardadosPassword = 0;
	p_sistema->password.number = defaultPassword;
	p_sistema->alarma.alarmaActivada = defaultAlarmaActivada;
	SetHora(defaultHoraAlarma, &p_sistema->alarma.hora);
	int configReloj = ConfiguraInicializaReloj(&p_sistema->reloj);
	if(configReloj!=0){return 1;}
	memcpy(p_sistema->teclado.filas, arrayFilas, sizeof(arrayFilas));
	memcpy(p_sistema->teclado.columnas, arrayColumnas, sizeof(arrayColumnas));
	int setupgpio = wiringPiSetupGpio();
	if(setupgpio != 0) return 1;
	ConfiguraInicializaTeclado(&p_sistema->teclado);
	#if VERSION == 4
	p_sistema->lcdId = lcdInit(rows, cols, bits, rs, strb, d0, d1, d2, d3, d4, d5, d6, d7);
	if(p_sistema->lcdId == -1) return 1;
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdDisplay(p_sistema->lcdId, 1);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0,0);
	lcdPrintf(p_sistema->lcdId, "    Hii!" );
	lcdPosition(p_sistema->lcdId, 0,1);
	lcdPrintf(p_sistema->lcdId, "  /(^o^)|" );
	delay(500);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0,0);
	lcdPrintf(p_sistema->lcdId, "    Hii!" );
	lcdPosition(p_sistema->lcdId, 0,1);
	lcdPrintf(p_sistema->lcdId, "  |(^O^)/" );
	delay(500);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0,0);
	lcdPrintf(p_sistema->lcdId, "    Hii!" );
	lcdPosition(p_sistema->lcdId, 0,1);
	lcdPrintf(p_sistema->lcdId, "  /(^o^)|" );
	delay(500);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0,0);
	lcdPrintf(p_sistema->lcdId, "    Hii!" );
	lcdPosition(p_sistema->lcdId, 0,1);
	lcdPrintf(p_sistema->lcdId, "  |(^O^)/" );
	delay(500);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
	return 0;
}

//FUNCIONES COMPROBACION

int EsNumero(char value){
	int valorAscii = (int)value;
	if(valorAscii>=48 && valorAscii<=57){
		return 1;
	}
	return 0;
}

int CompruebaDigitoPulsado(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_DIGITO_PULSADO;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaNewTimeIsReady(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_NEW_TIME_IS_READY;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaReset(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_RESET;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaSetCancelNewTime(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_TIME;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaSetupDone(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaTimeActualizado(fsm_t* p_this){
	TipoRelojShared sharedvars = GetRelojSharedVar();
	int shv = sharedvars.flags;
	int res = shv & FLAG_TIME_ACTUALIZADO;
	if(res!=0){return 1;}
	return 0;
}

int CompruebaSetCancelNewDate(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_DATE;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaNewDateIsReady(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_NEW_DATE_IS_READY;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaSwitchFormat(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_SWITCH_FORMAT;
	piUnlock(SYSTEM_KEY);
	if(res!=0){return 1;}
	return 0;
}

int CompruebaPasswordTime(fsm_t* this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_PASSWORD_TIME;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaPasswordDate(fsm_t* this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_PASSWORD_DATE;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaPasswordAlarma(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
	res = g_flagsCoreWatch & FLAG_PASSWORD_ALARMA;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaTeclaPulsada(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		TipoTecladoShared ts = GetTecladoSharedVar();
		res = ts.flags & FLAG_TECLA_PULSADA;
	piUnlock(SYSTEM_KEY);
	return res;
}

int CompruebaHoraAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	int res=0;
	piLock(RELOJ_KEY);
	if(p_sistema->alarma.alarmaActivada==1){
		int horaalarma = p_sistema->alarma.hora.hh;
		int minalarma = p_sistema->alarma.hora.hh;
		int horasis = p_sistema->reloj.hora.hh;
		int minsis = p_sistema->reloj.hora.hh;
		int segsis = p_sistema->reloj.hora.ss;
		if(horaalarma == horasis && minsis == minalarma && segsis == 0){
			res = 1;
		}
	}
	piUnlock(RELOJ_KEY);
	return res;
}

int CompruebaTimeoutAlarma(fsm_t* this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_TIMEOUT_ALARMA;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaSetCancelNewAlarma(fsm_t* this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_SET_ALARMA;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaNewAlarmaIsReady(fsm_t* this){
	int res;
	piLock(SYSTEM_KEY);
	res = g_flagsCoreWatch & FLAG_NEW_ALARMA_IS_READY;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaTimeoutPassword(fsm_t* this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_TIMEOUT_PASSWORD;
	piUnlock(SYSTEM_KEY);
	if(res!= 0)return 1;
	return 0;
}

int CompruebaSetPassword(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_SET_PASSWORD;
	piUnlock(SYSTEM_KEY);
	if(res!=0)return 1;
	return 0;
}

int CompruebaPasswordPassword(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
		res = g_flagsCoreWatch & FLAG_PASSWORD_PASSWORD;
	piUnlock(SYSTEM_KEY);
	if(res!=0)return 1;
	return 0;
}

int CompruebaNewPasswordIsReady(fsm_t* p_this){
	int res;
	piLock(SYSTEM_KEY);
	res = g_flagsCoreWatch & FLAG_NEW_PASSWORD_IS_READY;
	piUnlock(SYSTEM_KEY);
	if(res!=0)return 1;
	return 0;
}

//FUNCION PROCESAR TECLAS

void ProcesaTeclaPulsada(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	char teclaPulsada;
	piLock(SYSTEM_KEY);
		TipoTecladoShared ts = GetTecladoSharedVar();
		//limpiar el flag
		ts.flags = ts.flags & (~FLAG_TECLA_PULSADA);
		SetTecladoSharedVar(ts);
		teclaPulsada = ts.teclaDetectada;
	piUnlock(SYSTEM_KEY);

//	if(teclaPulsada == (int)TECLA_RESET){
//		//activa flag reset
//		piLock(SYSTEM_KEY);
//		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_RESET;
//		piUnlock(SYSTEM_KEY);
	if(teclaPulsada == (int)TECLA_PASSWORD){
		//activa flag set password
		piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_SET_PASSWORD;
		piUnlock(SYSTEM_KEY);
	}else if(teclaPulsada== (int)TECLA_SET_CANCEL_TIME){
		//activa flag set cancel time
		piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_SET_CANCEL_NEW_TIME;
		piUnlock(SYSTEM_KEY);
	}else if(teclaPulsada== (int)TECLA_SET_CANCEL_DATE){
		//activa flag set cancel date
		piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_SET_CANCEL_NEW_DATE;
		piUnlock(SYSTEM_KEY);
	}else if(teclaPulsada== (int)TECLA_SWITCH_FORMAT){
		//activa flag set cancel date
		piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_SWITCH_FORMAT;
		piUnlock(SYSTEM_KEY);
	}else if(teclaPulsada== (int)TECLA_ALARMA){
		//activa flag set cancel date
		piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_SET_ALARMA;
		piUnlock(SYSTEM_KEY);
	}else if(EsNumero(teclaPulsada)){
		//activa flag digito pulsado y guarda el digito
		piLock(SYSTEM_KEY);
		g_coreWatch.digitoPulsado = (int)(teclaPulsada) - 48;
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_DIGITO_PULSADO;
		piUnlock(SYSTEM_KEY);
	}else if(teclaPulsada == TECLA_EXIT){
		//sale del sistema
		#if VERSION<=3
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\rSaliendo del sistema\n");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		#endif
		#if VERSION==4
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0,0);
		lcdPrintf(p_sistema->lcdId, "    Bye!" );
		lcdPosition(p_sistema->lcdId, 0,1);
		lcdPrintf(p_sistema->lcdId, "  /(^o^)|" );
		delay(500);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0,0);
		lcdPrintf(p_sistema->lcdId, "    Bye!" );
		lcdPosition(p_sistema->lcdId, 0,1);
		lcdPrintf(p_sistema->lcdId, "  |(^O^)/" );
		delay(500);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0,0);
		lcdPrintf(p_sistema->lcdId, "    Bye!" );
		lcdPosition(p_sistema->lcdId, 0,1);
		lcdPrintf(p_sistema->lcdId, "  /(^o^)|" );
		delay(500);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0,0);
		lcdPrintf(p_sistema->lcdId, "    Bye!" );
		lcdPosition(p_sistema->lcdId, 0,1);
		lcdPrintf(p_sistema->lcdId, "  |(^O^)/" );
		delay(500);
		lcdDisplay(p_sistema->lcdId, 0);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		#endif

		exit(0);
	}else if((teclaPulsada != '\n') && (teclaPulsada != '\r') && (teclaPulsada != 0xA)){
		#if VERSION<=3
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\rTecla desconocida\n");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		#endif
	}

}

//FUNCIONES PASSWORD

void PrepareEnterPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->password.digitosGuardadosPassword = 0;
	p_sistema->password.tempPassword = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_PASSWORD_DATE);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_PASSWORD_TIME);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_PASSWORD_ALARMA);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_PASSWORD_PASSWORD);
	piUnlock(SYSTEM_KEY);
#if VERSION==4
	if(g_flagsCoreWatch & FLAG_SET_PASSWORD){
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "CHANGING");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "PASSWORD");
		delay(1000);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "INTRODUCE");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "CURRENT");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}else{
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "ENTER");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "PASSWORD");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}

#endif
//	piLock(SYSTEM_KEY);
//	tmr_t* tmrpassword = tmr_new(tmr_timeout_password);
//	p_sistema->password.tmrPassword= tmrpassword;
//	tmr_startms(p_sistema->password.tmrPassword, TIMEOUT_PASSWORD);
//	piUnlock(SYSTEM_KEY);
}

void EnterPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	int tempPassword = p_sistema->password.tempPassword;
	int digitosGuardadosPassword = p_sistema->password.digitosGuardadosPassword;
	int ultimoDigito = g_coreWatch.digitoPulsado;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);
	if(digitosGuardadosPassword < 4){
		tempPassword = tempPassword*10 + ultimoDigito;
		digitosGuardadosPassword++;
	}
	if(digitosGuardadosPassword == 4){
		if(tempPassword == p_sistema->password.number){
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
			piUnlock(SYSTEM_KEY);
			piLock(SYSTEM_KEY);
			if(g_flagsCoreWatch & FLAG_SET_PASSWORD){
				g_flagsCoreWatch = g_flagsCoreWatch | FLAG_PASSWORD_PASSWORD;
			}
			if(g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_TIME){
				g_flagsCoreWatch = g_flagsCoreWatch | FLAG_PASSWORD_TIME;
			}
			if(g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_DATE){
				g_flagsCoreWatch = g_flagsCoreWatch | FLAG_PASSWORD_DATE;
			}
			if(g_flagsCoreWatch & FLAG_SET_ALARMA){
				g_flagsCoreWatch = g_flagsCoreWatch | FLAG_PASSWORD_ALARMA;
			}
			piUnlock(SYSTEM_KEY);
		}else{
			digitosGuardadosPassword =0;
			tempPassword = 0;
			piLock(STD_IO_LCD_BUFFER_KEY);
			lcdClear(p_sistema->lcdId);
			lcdPosition(p_sistema->lcdId, 0, 0);
			lcdPrintf(p_sistema->lcdId, "WRONG PASSWORD!");
			lcdPosition(p_sistema->lcdId, 0, 1);
			lcdPrintf(p_sistema->lcdId, "TRY AGAIN");
			delay(1000);
			lcdClear(p_sistema->lcdId);
			lcdPosition(p_sistema->lcdId, 0, 0);
			lcdPrintf(p_sistema->lcdId, "ENTER");
			lcdPosition(p_sistema->lcdId, 0, 1);
			lcdPrintf(p_sistema->lcdId, "PASSWORD");
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	}
	p_sistema->password.tempPassword = tempPassword;
	p_sistema->password.digitosGuardadosPassword = digitosGuardadosPassword;
}

void tmr_timeout_password(union sigval value) {
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_TIMEOUT_PASSWORD;
	piUnlock(SYSTEM_KEY);
}

void CancelEnterPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->password.tempPassword = 0;
    p_sistema->password.digitosGuardadosPassword = 0;
    piLock(SYSTEM_KEY);
   		g_flagsCoreWatch = g_flagsCoreWatch & ~FLAG_TIMEOUT_PASSWORD;
   		g_flagsCoreWatch = g_flagsCoreWatch & ~FLAG_SET_CANCEL_NEW_DATE;
   		g_flagsCoreWatch = g_flagsCoreWatch & ~FLAG_SET_CANCEL_NEW_TIME;
   		g_flagsCoreWatch = g_flagsCoreWatch & ~FLAG_SET_ALARMA;
    piUnlock(SYSTEM_KEY);
}

void PrepareSetNewPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_SET_PASSWORD);
	g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_NEW_PASSWORD_IS_READY);
	piUnlock(SYSTEM_KEY);
	p_sistema->password.tempNewPassword = 0;
	p_sistema->password.tempNewPasswordRepeat = 0;
	p_sistema->password.digitosGuardadosPassword = 0;
	#if VERSION==4
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "INTRODUCE");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "NEW");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void EnterNewPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	int tempNewPassword = p_sistema->password.tempNewPassword;
	int tempNewPasswordRepeat = p_sistema->password.tempNewPasswordRepeat;
	int digitosGuardadosPassword = p_sistema->password.digitosGuardadosPassword;
	int ultimoDigito = g_coreWatch.digitoPulsado;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);
	if(digitosGuardadosPassword < 4){
		tempNewPassword = tempNewPassword*10 + ultimoDigito;
		digitosGuardadosPassword++;
	}
	if(digitosGuardadosPassword >= 5 && digitosGuardadosPassword < 9){
		tempNewPasswordRepeat = tempNewPasswordRepeat*10 + ultimoDigito;
		digitosGuardadosPassword++;
	}
	if(digitosGuardadosPassword == 9){
		if(tempNewPassword == tempNewPasswordRepeat){
			piLock(STD_IO_LCD_BUFFER_KEY);
			lcdClear(p_sistema->lcdId);
			lcdPosition(p_sistema->lcdId, 0, 0);
			lcdPrintf(p_sistema->lcdId, "PASSWORD");
			lcdPosition(p_sistema->lcdId, 0, 1);
			lcdPrintf(p_sistema->lcdId, "CHANGED");
			delay(1000);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
			//FLAGS
			piLock(SYSTEM_KEY);
				g_flagsCoreWatch = g_flagsCoreWatch | FLAG_NEW_PASSWORD_IS_READY;
			piUnlock(SYSTEM_KEY);
		}else{
			piLock(STD_IO_LCD_BUFFER_KEY);
			lcdClear(p_sistema->lcdId);
			lcdPosition(p_sistema->lcdId, 0, 0);
			lcdPrintf(p_sistema->lcdId, "NOT MATCHING");
			lcdPosition(p_sistema->lcdId, 0, 1);
			lcdPrintf(p_sistema->lcdId, "TRY AGAIN");
			delay(1000);
			lcdClear(p_sistema->lcdId);
			lcdPosition(p_sistema->lcdId, 0, 0);
			lcdPrintf(p_sistema->lcdId, "INTRODUCE");
			lcdPosition(p_sistema->lcdId, 0, 1);
			lcdPrintf(p_sistema->lcdId, "NEW");
			piUnlock(STD_IO_LCD_BUFFER_KEY);
			digitosGuardadosPassword = 0;
			tempNewPassword = 0;
			tempNewPasswordRepeat = 0;
		}
	}

	if(digitosGuardadosPassword == 4){
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "REPEAT");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "PLEASE");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		digitosGuardadosPassword++;
	}

	p_sistema->password.tempNewPassword = tempNewPassword;
	p_sistema->password.tempNewPasswordRepeat = tempNewPasswordRepeat;
	p_sistema->password.digitosGuardadosPassword = digitosGuardadosPassword;
}

void CancelSetNewPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_DIGITO_PULSADO);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_SET_PASSWORD);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_NEW_PASSWORD_IS_READY);
	piUnlock(SYSTEM_KEY);
	p_sistema->password.tempNewPassword = 0;
	p_sistema->password.tempNewPasswordRepeat = 0;
	p_sistema->password.digitosGuardadosPassword = 0;
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "SET_PASSWORD");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "CANCELLED");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
}

void SetNewPassword(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->password.number = p_sistema->password.tempNewPassword;
	p_sistema->password.tempNewPassword = 0;
	p_sistema->password.tempNewPasswordRepeat = 0;
	p_sistema->password.digitosGuardadosPassword = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_NEW_PASSWORD_IS_READY);
	piUnlock(SYSTEM_KEY);
}

//FUNCIONES STANDBY

void Start(fsm_t* p_this){
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_SETUP_DONE);
	piUnlock(SYSTEM_KEY);
}

void ShowTime(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	TipoRelojShared sharedvars = GetRelojSharedVar();
	sharedvars.flags = sharedvars.flags & (~ FLAG_TIME_ACTUALIZADO);
	SetRelojSharedVar(sharedvars);
	TipoReloj reloj_sistema = p_sistema->reloj;
	int hora = reloj_sistema.hora.hh;
	int min = reloj_sistema.hora.mm;
	int seg = reloj_sistema.hora.ss;
	int dia = reloj_sistema.calendario.dd;
	int mes = reloj_sistema.calendario.MM;
	int year = reloj_sistema.calendario.yyyy;
	int ampm;
	if(reloj_sistema.hora.formato == 12){
		ampm = hora_ampm[1][hora];
		hora = hora_ampm[0][hora];
	}
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	//impresion hora
	char str[12];
	//usamos sprintf para anadir a la variable str la hora min y seg
	if(hora<10){
		sprintf(str, "0%d:", hora);
	}else{
		sprintf(str, "%d:", hora);
	}
	if(min<10){
		sprintf(str, "%s0%d:", str, min);
	}else{
		sprintf(str, "%s%d:", str, min);
	}
	if(seg<10){
		sprintf(str, "%s0%d", str, seg);
	}else{
		sprintf(str, "%s%d", str, seg);
	}
	if(reloj_sistema.hora.formato == 24){
		lcdPrintf(p_sistema->lcdId, "%s", str);
	}else{
		if(ampm){
			lcdPrintf(p_sistema->lcdId, "%s pm", str);
		}else{
			lcdPrintf(p_sistema->lcdId, "%s am", str);
		}
	}
	char date[12];
	sprintf(date,"%c ", diasemana[p_sistema->reloj.calendario.weekDay]);
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "%s%d/%d/%d", date, dia, mes, year);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
}

//FUNCION RESET

void Reset(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	ResetReloj(&(p_sistema->reloj));
	piLock(SYSTEM_KEY);
			//ponemos a 0 flag_reset
			g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_RESET);
	piUnlock(SYSTEM_KEY);
	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\r[RESET] Fecha y hora reiniciadas\n");
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION==4
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "  [RESET]");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

//FUNCIONES TIME

void PrepareSetNewTime(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_SET_CANCEL_NEW_TIME);
	piUnlock(SYSTEM_KEY);
	#if VERSION==4
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "[SET_TIME]");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "__:__");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void CancelSetNewTime(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_SET_CANCEL_NEW_TIME);
	piUnlock(SYSTEM_KEY);
	#if VERSION==4
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "[SET_TIME]");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "CANCELLED");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void ProcesaDigitoTime(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	TipoReloj r = p_sistema->reloj;
	int formato = r.hora.formato;
	int tempTime = p_sistema->tempTime;
	int digitosGuardados = p_sistema->digitosGuardados;
	#if VERSION >= 2
	int ultimoDigito = g_coreWatch.digitoPulsado;
	#endif

	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);
	if(formato == 12){
		if(digitosGuardados == 0){
			ultimoDigito = min(1, ultimoDigito);
			tempTime = tempTime*10 + ultimoDigito;
		}else if(digitosGuardados == 1){
			if(tempTime == 0){
				ultimoDigito = max(1, ultimoDigito);
			}else{
				ultimoDigito = min(2, ultimoDigito);
			}
			tempTime = tempTime * 10 + ultimoDigito;
		}else if(digitosGuardados == 2){
			tempTime = tempTime * 10 + min(5, ultimoDigito);
		}else if (digitosGuardados == 3){
			tempTime = tempTime*10+ultimoDigito;
		}else{
			ultimoDigito = min(ultimoDigito, 1);
			tempTime = tempTime*10+ultimoDigito;
			piLock(SYSTEM_KEY);
				g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
			piUnlock(SYSTEM_KEY);
			piLock(SYSTEM_KEY);
				g_flagsCoreWatch = g_flagsCoreWatch | FLAG_NEW_TIME_IS_READY;
			piUnlock(SYSTEM_KEY);
		}
	}else{
		if(digitosGuardados == 0){
			ultimoDigito = min(2, ultimoDigito);
			tempTime = tempTime*10 + ultimoDigito;
		}else if(digitosGuardados == 1){
			if(tempTime  == 2){
				ultimoDigito = min(3, ultimoDigito);
			}
			tempTime = tempTime * 10 + ultimoDigito;
		}else if(digitosGuardados == 2){
			tempTime = tempTime * 10 + min(5, ultimoDigito);
		}else{
			tempTime = tempTime*10+ultimoDigito;
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
			piUnlock(SYSTEM_KEY);
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch | FLAG_NEW_TIME_IS_READY;
			piUnlock(SYSTEM_KEY);
		}
	}
	digitosGuardados++;
	#if VERSION == 4
	piLock(STD_IO_LCD_BUFFER_KEY);
	if(digitosGuardados<3){
		lcdPosition(p_sistema->lcdId, digitosGuardados-1, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempTime%10 + 48));
	}else{
		lcdPosition(p_sistema->lcdId, digitosGuardados, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempTime%10 + 48));
	}
	if(formato == 12 && digitosGuardados == 4){
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "SELECCIONE");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "AM:0 / PM:1");
	}
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	p_sistema->tempTime = tempTime;
	p_sistema->digitosGuardados = digitosGuardados;

}

void SetNewTime(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_NEW_TIME_IS_READY);
	piUnlock(SYSTEM_KEY);
	if (p_sistema->reloj.hora.formato == 12){
		int ampm = p_sistema->tempTime % 10;
		int min = (p_sistema->tempTime / 10) % 100;
		int hora = p_sistema->tempTime / 1000;
		if(ampm==1 && hora!=12){
			hora+=12;
		}
		if(ampm==0 && hora == 12){
			hora = 0;
		}
		hora = hora*100 + min;
		SetHora(hora, &p_sistema->reloj.hora);
	}else{
		SetHora(p_sistema->tempTime, &p_sistema->reloj.hora);
	}
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
}

//FUNCIONES DATE

void PrepareSetNewDate(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_SET_CANCEL_NEW_DATE);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_NEW_DATE_IS_READY);
	piUnlock(SYSTEM_KEY);
	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
			printf("\r[SET_DATE] Introduzca la nueva fecha en formato dd/mm/yyyy\n");
			fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION==4
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "[SET_DATE]");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "__/__/____");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void CancelSetNewDate(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->tempDate = 0;
	p_sistema->digitosGuardadosDate = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_SET_CANCEL_NEW_DATE);
	piUnlock(SYSTEM_KEY);
	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
			printf("\r[SET_DATE] Operacion cancelada\n");
			fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION==4
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "[SET_DATE]");
	lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "CANCELLED");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

void ProcesaDigitoDate(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	int tempDate = p_sistema->tempDate;
	int digitosGuardadosDate = p_sistema->digitosGuardadosDate;
	#if VERSION >= 2
	int ultimoDigito = g_coreWatch.digitoPulsado;
	#endif
	int aux = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);
	//procesamos el dia
	if(digitosGuardadosDate == 0){
		ultimoDigito = min(ultimoDigito, MAX_DAY/10);
		tempDate = tempDate*10 + ultimoDigito;
	}else if(digitosGuardadosDate == 1) {
		tempDate = min(tempDate*10+ultimoDigito, MAX_DAY);
	//procesamos el mes
	}else if(digitosGuardadosDate == 2) {
		ultimoDigito = min(ultimoDigito, 1);
		tempDate = tempDate*10 + ultimoDigito;
	}else if(digitosGuardadosDate == 3){
		aux = tempDate / 10;
		aux = (aux * 100) + MAX_MONTH; // aux = dd/12
		tempDate = min(tempDate*10+ultimoDigito, aux);
	//procesamos el year
	}else if(digitosGuardadosDate == 4){
		ultimoDigito = max(ultimoDigito, 1);
		tempDate = tempDate*10 + ultimoDigito;
	}else if(digitosGuardadosDate == 5){
		aux = tempDate / 10;
		aux = aux * 100 + MIN_YEAR/100; //aux = dd/mm/19xx
		tempDate = max(tempDate*10+ultimoDigito, aux);
	}else if(digitosGuardadosDate == 6){
		aux = tempDate / 100;
		aux = aux * 1000 + MIN_YEAR/10; //aux = dd/mm/197x
		tempDate = max(tempDate*10+ultimoDigito, aux);
	}else{
		aux = tempDate / 1000;
		aux = aux * 10000 + MIN_YEAR; //aux = dd/mm/1970
		tempDate = max(tempDate*10+ultimoDigito, aux);
		piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
		piUnlock(SYSTEM_KEY);
		piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch | FLAG_NEW_DATE_IS_READY;
		piUnlock(SYSTEM_KEY);
	}
	digitosGuardadosDate++;
	#if VERSION<4
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("\rNueva fecha temporal: %d\n", tempDate);
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION >= 4
	piLock(STD_IO_LCD_BUFFER_KEY);
	if(digitosGuardadosDate<3){
		lcdPosition(p_sistema->lcdId, digitosGuardadosDate-1, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempDate%10 + 48));
	}else if(digitosGuardadosDate >= 3 && digitosGuardadosDate < 5){
		lcdPosition(p_sistema->lcdId, digitosGuardadosDate, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempDate%10 + 48));
	}else{
		lcdPosition(p_sistema->lcdId, digitosGuardadosDate+1, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempDate%10 + 48));
	}
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	p_sistema->tempDate = tempDate;
	p_sistema->digitosGuardadosDate = digitosGuardadosDate;
}

void SetNewDate(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	int fecha = p_sistema->tempDate;
	int dia = fecha / 1000000; //2 digitos superiores
	int mes = (fecha / 10000) % 100; //2 digitos del medio
	int year = fecha%10000; //4 ultimos digitos
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_NEW_DATE_IS_READY);
	piUnlock(SYSTEM_KEY);
	//piLock(RELOJ_KEY);
	SetYear(year, &p_sistema->reloj.calendario);
	SetMes(mes, &p_sistema->reloj.calendario);
	SetDia(dia, &p_sistema->reloj.calendario);
	//piUnlock(RELOJ_KEY);
	dia = p_sistema->reloj.calendario.dd;
	mes = p_sistema->reloj.calendario.MM;
	year = p_sistema->reloj.calendario.yyyy;

	int weekDay = CalculaDiaSemana(dia, mes, year);
	p_sistema->reloj.calendario.weekDay = weekDay;
	p_sistema->tempDate = 0;
	p_sistema->digitosGuardadosDate = 0;
}

//FUNCIONES FORMATO

void SwitchFormat(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	int formato = 12;
	if(p_sistema->reloj.hora.formato == 12) formato = 24;
	SetFormat(formato, &p_sistema->reloj.hora);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_SWITCH_FORMAT);
	piUnlock(SYSTEM_KEY);
}

//FUNCIONES ALARMA

void SuenaAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	tmr_t* tempalarma = tmr_new(tmr_timeout_alarma);
	p_sistema->alarma.timer = tempalarma;
	tmr_startms(tempalarma, TIMEOUT_ALARMA);
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "Bip! Bip!");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "Bip! Bip!");
	piUnlock(STD_IO_LCD_BUFFER_KEY);
}

void ExitAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_TIMEOUT_ALARMA);
	piUnlock(SYSTEM_KEY);
}

void tmr_timeout_alarma(union sigval value) {
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch | FLAG_TIMEOUT_ALARMA;
	piUnlock(SYSTEM_KEY);
}

void PrepareSetAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->alarma.digitosGuardadosAlarma = 0;
	p_sistema->alarma.tempAlarma = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_SET_ALARMA);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_DIGITO_PULSADO);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_NEW_ALARMA_IS_READY);
	piUnlock(SYSTEM_KEY);
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "[SET_ALARMA]");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "__:__");
	delay(1000);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
}

void CancelSetNewAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	p_sistema->alarma.digitosGuardadosAlarma = 0;
	p_sistema->alarma.tempAlarma = 0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_SET_ALARMA);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_DIGITO_PULSADO);
		g_flagsCoreWatch = g_flagsCoreWatch & (~FLAG_NEW_ALARMA_IS_READY);
	piUnlock(SYSTEM_KEY);
	piLock(STD_IO_LCD_BUFFER_KEY);
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId, 0, 0);
	lcdPrintf(p_sistema->lcdId, "[SET_ALARMA]");
	lcdPosition(p_sistema->lcdId, 0, 1);
	lcdPrintf(p_sistema->lcdId, "CANCELLED");
	delay(1000);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
}

void ProcesaDigitoAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	TipoReloj r = p_sistema->reloj;
	int formato = r.hora.formato;
	int tempTime = p_sistema->alarma.tempAlarma;
	int digitosGuardados = p_sistema->alarma.digitosGuardadosAlarma;
	int ultimoDigito = g_coreWatch.digitoPulsado;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
	piUnlock(SYSTEM_KEY);
	if(formato == 12){
		if(digitosGuardados == 0){
			ultimoDigito = min(1, ultimoDigito);
			tempTime = tempTime*10 + ultimoDigito;
		}else if(digitosGuardados == 1){
			if(tempTime == 0){
				ultimoDigito = max(1, ultimoDigito);
			}else{
				ultimoDigito = min(2, ultimoDigito);
			}
			tempTime = tempTime * 10 + ultimoDigito;
		}else if(digitosGuardados == 2){
			tempTime = tempTime * 10 + min(5, ultimoDigito);
		}else if (digitosGuardados == 3){
			tempTime = tempTime*10+ultimoDigito;
		}else if (digitosGuardados == 4){
			ultimoDigito = min(ultimoDigito, 1);
			tempTime = tempTime*10+ultimoDigito;
		}else {
			tempTime = tempTime*10+ultimoDigito;
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
			piUnlock(SYSTEM_KEY);
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch | FLAG_NEW_ALARMA_IS_READY;
			piUnlock(SYSTEM_KEY);
		}
	}else{
		if(digitosGuardados == 0){
			ultimoDigito = min(2, ultimoDigito);
			tempTime = tempTime*10 + ultimoDigito;
		}else if(digitosGuardados == 1){
			if(tempTime  == 2){
				ultimoDigito = min(3, ultimoDigito);
			}
			tempTime = tempTime * 10 + ultimoDigito;
		}else if(digitosGuardados == 2){
			tempTime = tempTime * 10 + min(5, ultimoDigito);
		}else if(digitosGuardados == 3){
			tempTime = tempTime * 10 + ultimoDigito;
		}else{
			tempTime = tempTime*10+ultimoDigito;
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_DIGITO_PULSADO);
			piUnlock(SYSTEM_KEY);
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch = g_flagsCoreWatch | FLAG_NEW_ALARMA_IS_READY;
			piUnlock(SYSTEM_KEY);
		}
	}
	digitosGuardados++;
	#if VERSION == 4
	piLock(STD_IO_LCD_BUFFER_KEY);
	if(digitosGuardados<3){
		lcdPosition(p_sistema->lcdId, digitosGuardados-1, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempTime%10 + 48));
	}else{
		lcdPosition(p_sistema->lcdId, digitosGuardados, 1);
		lcdPutchar(p_sistema->lcdId, (char)(tempTime%10 + 48));
	}
	if(formato == 12 && digitosGuardados == 4){
		delay(500);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "SELECCIONE");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "AM:0 / PM:1");
	}
	if((formato == 24 && digitosGuardados == 4) || (formato == 12 && digitosGuardados == 5)){
		delay(500);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdPrintf(p_sistema->lcdId, "[SET_ALARMA]");
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPrintf(p_sistema->lcdId, "OFF:0 / ON:1");
	}
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	p_sistema->alarma.tempAlarma = tempTime;
	p_sistema->alarma.digitosGuardadosAlarma = digitosGuardados;
}

void SetNewAlarma(fsm_t* p_this){
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = g_flagsCoreWatch & (~ FLAG_NEW_ALARMA_IS_READY);
	piUnlock(SYSTEM_KEY);
	if (p_sistema->reloj.hora.formato == 12){
		int activada = p_sistema->alarma.tempAlarma % 10;
		int ampm = (p_sistema->alarma.tempAlarma / 10) % 10;
		int min = (p_sistema->alarma.tempAlarma / 100) % 100;
		int hora = p_sistema->alarma.tempAlarma / 10000;
		if(ampm==1 && hora!=12){
			hora+=12;
		}
		if(ampm==0 && hora == 12){
			hora = 0;
		}
		hora = hora*100 + min;
		SetHora(hora, &p_sistema->alarma.hora);
		p_sistema->alarma.alarmaActivada = activada;
	}else{
		int hora = p_sistema->alarma.tempAlarma/10;
		int activada = p_sistema->alarma.tempAlarma%10;
		SetHora(hora, &p_sistema->alarma.hora);
		p_sistema->alarma.alarmaActivada = activada;
	}
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
}

//------------------------------------------------------
// MAIN
//------------------------------------------------------
int main() {
	unsigned int next;

#if VERSION == 4
	TipoCoreWatch corePrueba;
	int sol = ConfiguraInicializaSistema(&corePrueba);
	TipoReloj r = corePrueba.reloj;
	if(sol != 0){printf("error");exit(0);}
	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(corePrueba.reloj));
	fsm_t* fsmCoreWatch = fsm_new(START, g_fsmTransCoreWatch, &(corePrueba));
	fsm_t* fsmDeteccionComandos = fsm_new(WAIT_COMMAND, g_fsmTransDeteccionComandos, &(corePrueba));
	fsm_t* fsmTeclado = fsm_new(TECLADO_ESPERA_COLUMNA, g_fsmTransExcitacionColumnas, &(corePrueba.teclado));
	next = millis();
	while(1){
		fsm_fire(fsmReloj);
		fsm_fire(fsmDeteccionComandos);
		fsm_fire(fsmTeclado);
		fsm_fire(fsmCoreWatch);
		next+=CLK_MS;
		DelayUntil(next);
	}
	//destruir
	tmr_destroy(r.tmrTic);
	fsm_destroy(fsmReloj);
	fsm_destroy(fsmCoreWatch);

#endif

}
