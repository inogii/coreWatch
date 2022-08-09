#ifndef COREWATCH_H_
#define COREWATCH_H_

// INCLUDES
// Propios:
#include "systemConfig.h"     // Sistema: includes, entrenadora (GPIOs, MUTEXes y entorno), setup de perifericos y otros otros.
#include "reloj.h"
#include "teclado_TL04.h"
// DEFINES Y ENUMS
enum FSM_ESTADOS_SISTEMA{
	START, STAND_BY, SET_TIME, SET_DATE, SET_ALARMA, ALARMA, SET_PASSWORD, PASSWORD
};
enum FSM_DETECCION_COMANDOS{
	WAIT_COMMAND
};

// FLAGS FSM DEL SISTEMA CORE WATCH
#define FLAG_SETUP_DONE 0x01 //0000 0001
#define FLAG_TIME_ACTUALIZADO 0x02 //0000 0010
#define FLAG_RESET 0x04 //0000 0100
#define FLAG_SET_CANCEL_NEW_TIME 0x08 //0000 1000
#define FLAG_NEW_TIME_IS_READY 0x10 //0001 0000
#define FLAG_DIGITO_PULSADO 0x20 //0010 0000
#define FLAG_SET_CANCEL_NEW_DATE 0x40 //0100 0000
#define FLAG_NEW_DATE_IS_READY 0x80 //1000 0000
#define FLAG_SWITCH_FORMAT 0x100 //0001 0000 0000
#define FLAG_PASSWORD_TIME 0x200 //0010 0000 0000
#define FLAG_PASSWORD_DATE 0x400
#define FLAG_SET_ALARMA 0x800
#define FLAG_NEW_ALARMA_IS_READY 0x1000
#define FLAG_TIMEOUT_ALARMA 0x2000
#define FLAG_TIMEOUT_PASSWORD 0x4000
#define FLAG_PASSWORD_ALARMA 0x8000
#define FLAG_SET_PASSWORD 0x10000
#define FLAG_PASSWORD_PASSWORD 0x20000
#define FLAG_NEW_PASSWORD_IS_READY 0x40000

// DECLARACIÓN ESTRUCTURAS
typedef  struct {
	TipoHora hora;
	tmr_t* timer;
	int  alarmaActivada;
	int tempAlarma;
	int digitosGuardadosAlarma;
}TipoAlarma;

typedef struct{
	int  number;
	tmr_t* tmrPassword;
	int  tempPassword;
	int  digitosGuardadosPassword;
	int tempNewPassword;
	int tempNewPasswordRepeat;
}TipoPassword;

typedef  struct {
	TipoReloj  reloj;
	TipoTeclado  teclado;
	int  lcdId;
	int  tempTime;
	int  digitosGuardados;
	int  tempDate;
	int  digitosGuardadosDate;
	int  digitoPulsado;
	TipoPassword password;
	TipoAlarma alarma;
} TipoCoreWatch;



// DECLARACIÓN VARIABLES
#define TECLA_ALARMA 'A'
//#define TECLA_RESET 'F'
#define TECLA_PASSWORD 'F'
#define TECLA_SET_CANCEL_TIME 'E'
#define TECLA_SET_CANCEL_DATE 'D'
#define TECLA_EXIT 'B'
#define TECLA_SWITCH_FORMAT 'C'
#define TIMEOUT_ALARMA 5000 //5 segundos
#define TIMEOUT_PASSWORD 8000 //8 segundos

// DEFINICIÓN VARIABLES
extern fsm_trans_t g_fsmTransCoreWatch[];

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
void DelayUntil(unsigned int next);
int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema);
int EsNumero(char value);
int min(int a, int b);
int max(int a, int b);
//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaDigitoPulsado(fsm_t* p_this);
int CompruebaNewTimeIsReady(fsm_t* p_this);
int CompruebaNewDateIsReady(fsm_t* p_this);
int CompruebaReset(fsm_t* p_this);
int CompruebaSetCancelNewTime(fsm_t* p_this);
int CompruebaSetCancelNewDate(fsm_t* p_this);
int CompruebaSwitchFormat(fsm_t* p_this);
int CompruebaSetupDone(fsm_t* p_this);
int CompruebaTeclaPulsada(fsm_t* p_this);
int CompruebaTimeActualizado(fsm_t* p_this);
int CompruebaPasswordTime(fsm_t* p_this);
int CompruebaPasswordDate(fsm_t* p_this);
int CompruebaPasswordAlarma(fsm_t* p_this);
int CompruebaHoraAlarma(fsm_t* p_this);
int CompruebaTimeoutAlarma(fsm_t* p_this);
int CompruebaSetCancelNewAlarma(fsm_t* p_this);
int CompruebaNewAlarmaIsReady(fsm_t* p_this);
int CompruebaTimeoutPassword(fsm_t* p_this);
int CompruebaSetPassword(fsm_t* p_this);
int CompruebaPasswordPassword(fsm_t* p_this);
int CompruebaNewPasswordIsReady(fsm_t* p_this);
//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void CancelSetNewTime(fsm_t* p_this);
void PrepareSetNewTime(fsm_t* p_this);
void CancelSetNewDate(fsm_t* p_this);
void PrepareSetNewDate(fsm_t* p_this);
void ProcesaDigitoTime(fsm_t* p_this);
void ProcesaDigitoDate(fsm_t* p_this);
void ProcesaTeclaPulsada(fsm_t* p_this);
void Reset(fsm_t* p_this);
void SetNewTime(fsm_t* p_this);
void SetNewDate(fsm_t* p_this);
void SwitchFormat(fsm_t* p_this);
void ShowTime(fsm_t* p_this);
void Start(fsm_t* p_this);
void EnterPassword(fsm_t* p_this);
void PrepareEnterPassword(fsm_t* p_this);
void CancelEnterPassword(fsm_t* p_this);
void PrepareSetAlarma(fsm_t* p_this);
void SuenaAlarma(fsm_t* p_this);
void ExitAlarma(fsm_t* p_this);
void CancelSetNewAlarma(fsm_t* p_this);
void SetNewAlarma(fsm_t* p_this);
void ProcesaDigitoAlarma(fsm_t* p_this);
void PrepareSetNewPassword(fsm_t* p_this);
void ProcesaDigitoNewPassword(fsm_t* p_this);
void EnterNewPassword(fsm_t* p_this);
void SetNewPassword(fsm_t* p_this);
void CancelSetNewPassword(fsm_t* p_this);
//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void tmr_timeout_alarma(union sigval value);
void tmr_timeout_password(union sigval value);
//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------
PI_THREAD(ThreadExploraTecladoPC);

#endif /* EAGENDA_H */
