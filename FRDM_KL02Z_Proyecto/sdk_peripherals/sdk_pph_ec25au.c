/*! @file : sdk_pph_ec25au.c
 * @author  Luis José Castrillo Fernández
 * @version 1.0.0
 * @date    14 de marzo 2021
 * @brief   Driver para EC25au
 * @details
 *
*/
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "stdio.h"
#include <string.h>
#include "stdlib.h"
#include "sdk_pph_ec25au.h"
#include "sdk_mdlw_leds.h"
#include "sdk_hal_lptmr0.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct _estado_fsm{
	uint8_t anterior;
	uint8_t actual;
}estado_fsm_t;


#define EC25_BYTES_EN_BUFFER	120
/*******************************************************************************
 * Private Prototypes
 ******************************************************************************/
void ec25BorrarBufferRX(void);
void waitTime(void);
void delayOpen(void);
/*******************************************************************************
 * External vars
 ******************************************************************************/


/*******************************************************************************
 * Local vars
 ******************************************************************************/
uint8_t ec25_buffer_tx[EC25_BYTES_EN_BUFFER];		//almacena los datos a enviar al modem
uint8_t ec25_buffer_rx[EC25_BYTES_EN_BUFFER];		//almacena las datos que provienen del modem
uint8_t ec25_index_buffer_rx = 0;				//apuntador de buffer de datos
uint8_t intTemperatura, decTemperatura, intHumedad, decHumedad;
uint8_t location[21]; 				//Almacena los datos de la ubicación
uint8_t fecha[20]; 					//Almecena los datos de fecha y hora

estado_fsm_t ec25_fsm;	//almacena estado actual de la maquina de estados modem EC25

uint32_t ec25_timeout;	//almacena las veces que han llamado la fsm del EC25 y estimar tiempo

uint8_t ec25_comando_en_ejecucion;	//almacena ultimo comando enviado para ser ejecutado

//Listado de comando AT disponibles para ser enviados al modem Quectel
const char *ec25_comandos_at[] = {
	"AT",			//comprueba disponibilidad de dispositivo
	"ATI",			//consulta información del modem
	"AT+QGPS?",		//Preguntar si el sistema de localización GNSS está activo
	"AT+QGPS=1",	//Activar el sistema de localización GNSS si no está activo
	"AT+CPIN?",		//consulta estado de la simcard
	"AT+QCFG=\"nwscanmode\",0,1", //Modo de escanear
	"AT+QCFG=\"band\",0,800005A,0", //Configurar bandas
	"AT+CREG?",		//consulta estado de la red celular y tecnología usada en red celular
	"AT+CGREG?", 	//Consulta estado de la red celular para datos móviles
	"AT+CSQ",		//Consulta calidad de la señal RSSI
	"AT+QMTOPEN?", //Preguntar si hay conexión con el broker
	"AT+QMTCFG=\"recv/mode\",0,0,1",	//Activar modo recepción
	"AT+QMTOPEN=0,\"40.76.245.149\",1883", //Abrir conexión con el broker
	"AT+QMTCONN?", //Preguntar si hay conexión con el tópico seleccionado
	"AT+QMTCONN=0,\"1/datos\",\"guest\",\"guest\"", //Conectarse a un tópico seleccionado. Credenciales
	"AT+QGPSLOC=2",	//Datos de localización
	"AT+QMTPUB=0,0,0,0,\"1/datos\"",		//Publicar mensaje en el tópico seleccionado
	"MENSAJE",

	};

//Lista de respuestas a cada comando AT
const char  *ec25_repuestas_at[]={
		"OK",		//AT
		"EC25",		//ATI
		"+QGPS: 1",	//AT+QGPS?
		"OK",		//AT+QGPS=1
		"READY",	//AT+CPIN?
		"OK", 		//AT+QCFG="nwscanmode",0,1
		"OK", 		//AT+QCFG="band",0,800005A,0
		"0,1",		//AT+CREG?
		"0,1", 		//AT+CGREG?
		"+CSQ:",	//AT+CSQ
		"1883", 	//AT+AMTOPEN?
		"OK",		//AT+QMTCFG="recv/mode",0,0,1
		"OK", 		//AT+QMTOPEN=0,"Servidor",1883
		"+QMTCONN: 0,3", 	//AT+QMTCONN?
		"OK", 		//AT+QMTCONN=0,"topic","user","password"
		"OK",		//AT+QGPSLOC=2
		">",		//AT+QMTPUB=0,0,0,0,"topic"
		"OK", 		//mensaje + ctrl z
};


/*******************************************************************************
 * Private Source Code
 ******************************************************************************/
//------------------------------------
void ec25BorrarBufferRX(void){
	uint8_t i;

	//LLenar de ceros buffer que va a recibir datos provenientes del modem
	for(i=0;i<EC25_BYTES_EN_BUFFER;i++){
		ec25_buffer_rx[i]=0;
	}

	//borra apuntador de posicion donde se va a almacenar el proximo dato
	//Reset al apuntador
	ec25_index_buffer_rx=0;
}
//------------------------------------
void ec25BorrarBufferTX(void){
	uint8_t i;

	//LLenar de ceros buffer que va a recibir datos provenientes del modem
	for(i=0;i<EC25_BYTES_EN_BUFFER;i++){
		ec25_buffer_tx[i]=0;
	}
}
//------------------------------------
void waitTime(void){
	//espera a que hayan ocurrido por lo menos 100ms interrupciones
	while (lptmr0GetTimeValue() < 5777){
	}
	lptmr0SetTimeValue(0);		//Reset de variable contador de interrupciones
}

void delayOpen(void){
	//espera a que hayan ocurrido por lo menos 100ms interrupciones
	while (lptmr0GetTimeValue() < 1600){
	}
	lptmr0SetTimeValue(0);		//Reset de variable contador de interrupciones
}
//------------------------------------
void ec25EnviarComandoAT(uint8_t comando){
	char comando_at[EC25_BYTES_EN_BUFFER];
	sprintf(comando_at,"%s\r\n", ec25_comandos_at[comando]);	//Prepara en buffercomandoa enviar al modem
	uart0ImprimirMensaje((uint8_t *)(&comando_at[0]),strlen(comando_at));	//Envia comando AT indicado
}
//------------------------------------
status_t ec25ProcesarRespuestaAT(uint8_t comando){
	status_t resultado_procesamiento;	//variable que almacenará el resultado del procesamiento
	uint8_t *puntero_ok=0;	//variable temporal que será usada para buscar respuesta

	switch(ec25_fsm.anterior){

	case kFSM_ENVIANDO_AT:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),(char*) (ec25_repuestas_at[kAT])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}

		break;

	case kFSM_ENVIANDO_ATI:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kATI])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QGPS_PREG:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QGPS_PREG])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QGPS:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QGPS])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_CPIN:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_CPIN])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QCFG_NWSCANMODE:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QCFG_NWSCANMODE])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QCFG_BAND:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QCFG_BAND])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_CREG:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_CREG])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_CGREG:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_CGREG])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_CSQ:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_CSQ])));

		if(puntero_ok!=0x00){
			//la respuesta a AT+CSQ incluye dos parametros RSSI,BER
			//el valor de interes para la aplicación es RSSI
			char string_data[4];
			int8_t rssi;	// received signal strength
			uint8_t i;		//usada para borrar los daot s

			//borra buffer que va a almacenar string
			for(i=0;i<sizeof(string_data);i++){
				string_data[i]=0;
			}
			memcpy(&string_data[0],puntero_ok+5, 3);	//copia los bytes que corresponden al RSSI (3 digitos)
			rssi=(int8_t)atoi(&string_data[0]);//convierte string a entero

			if((rssi>EC25_RSSI_MINIMO_ACEPTADO)&&(rssi!=99)){
				resultado_procesamiento=kStatus_Success;
			}else{
				resultado_procesamiento=kStatus_OutOfRange;
			}
			}else{
				resultado_procesamiento=kStatus_Fail;
			}
		break;

	case kFSM_ENVIANDO_QGPSLOC:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QGPSLOC])));

		if(puntero_ok!=0x00){

			uint8_t *p_location = location, *p_ec25_buffer_rx = ec25_buffer_rx;
			uint8_t hora[9];
			int8_t i = 0, cont = 0;		//usada para borrar los datos

			//borra buffer que va a almacenar string
			for(i=0;i<sizeof(location);i++){
				location[i]=0;
			}

			for(i=0;i<sizeof(fecha);i++){
				fecha[i]=0;
			}

			for(i=0;i<sizeof(hora);i++){
				hora[i]=0;
			}

			for (i=0;i<sizeof(ec25_buffer_rx);i++){

				if(*p_ec25_buffer_rx==','){
					cont++;
					if (cont!=2){
						p_ec25_buffer_rx++;
					}
				}

				if ((cont>0)&&(cont<3)){
					*p_location = *p_ec25_buffer_rx;
					p_location++;
				}

				if(*p_ec25_buffer_rx == ':'){
					memcpy(&hora[0],p_ec25_buffer_rx+2,2);
					memcpy(&hora[2],":",1);
					memcpy(&hora[3],p_ec25_buffer_rx+4,2);
					memcpy(&hora[5],":",1);
					memcpy(&hora[6],p_ec25_buffer_rx+6,2);
				}

				if (cont==8){
					memcpy(&fecha[0],p_ec25_buffer_rx+2,2);
					memcpy(&fecha[2],".",1);
					memcpy(&fecha[3],p_ec25_buffer_rx+4,2);
					memcpy(&fecha[5],".20",3);
					memcpy(&fecha[8],p_ec25_buffer_rx+6,2);
				}
				p_ec25_buffer_rx++;
			}

			strcat(fecha," ");
			strcat(fecha,hora);

			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QMTCFG:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QMTCFG])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QMTOPEN_PREG:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QMTOPEN_PREG])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QMTOPEN:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QMTOPEN])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QMTCONN_PREG:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QMTCONN_PREG])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QMTCONN:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QMTCONN])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_QMTPUB:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_QMTPUB])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	case kFSM_ENVIANDO_MENSAJE_MQTT:
		//Busca palabra EC25 en buffer rx de quectel
		puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
				(char*) (ec25_repuestas_at[kAT_MENSAJE_END])));

		if(puntero_ok!=0x00){
			resultado_procesamiento=kStatus_Success;
		}else{
			resultado_procesamiento=kStatus_Fail;
		}
		break;

	default:
		//para evitar bloqueos, si se le especifica un comando incorrecto, genera error
		resultado_procesamiento=kStatus_Fail;
		break;
	}
	return(resultado_procesamiento);
}
/*******************************************************************************
 * Public Source Code
 ******************************************************************************/
status_t ec25Inicializacion(void){
	ec25_fsm.anterior=kFSM_INICIO;	//reinicia estado anterior
	ec25_fsm.actual=kFSM_ENVIANDO_AT;	//inicia la FSM a rtabajar desde el primer comando AT
	ec25_timeout=0;	//borra contador de tiempo
	ec25BorrarBufferTX();	//borrar buffer de transmisión
	ec25BorrarBufferRX();	//borra buffer de recepción
	return(kStatus_Success);
}
//------------------------------------
status_t copiarMensaje(uint8_t *mensaje, uint8_t size_mensaje ){
	memcpy(&ec25_buffer_tx[0],mensaje, size_mensaje);	//copia mensaje a enviar en buffer TX del EC25
	return(kStatus_Success);
}

status_t enteroTemperatura(uint16_t dato){
	intTemperatura = (uint8_t)(dato);
	return(kStatus_Success);
}

status_t decimalTemperatura(uint16_t dato){
	decTemperatura = (uint8_t)(dato);
	return(kStatus_Success);
}

status_t enteroHumedad(uint16_t dato){
	intHumedad = (uint8_t)(dato);
	return(kStatus_Success);
}

status_t decimalHumedad(uint16_t dato){
	decHumedad = (uint8_t)(dato);
	return(kStatus_Success);
}
//------------------------------------
uint8_t ec25Polling(void){
	status_t resultado;
	uint8_t nuevo_byte_uart;

	switch (ec25_fsm.actual){
	case kFSM_INICIO:
		//En este estado no hace nada y está a la espera de iniciar una nueva orden
		break;

	case kFSM_RESULTADO_ERROR:
		//Se queda en este estado y solo se sale cuando se utilice la función ec25Inicializacion();
		break;

	case kFSM_RESULTADO_EXITOSO:
		ec25_fsm.actual = kFSM_ESPERANDO_NUEVO_CICLO;
		//Se queda en este estado y solo se sale cuando se utilice la función ec25Inicializacion();
		break;

	case kFSM_ESPERANDO_NUEVO_CICLO:
		waitTime();
		ec25_fsm.actual = kFSM_ENVIANDO_QGPSLOC;
		break;

	case kFSM_RESULTADO_ERROR_RSSI:
		//Se queda en este estado y solo se sale cuando se utilice la función ec25Inicializacion();
		break;

	case kFSM_ENVIANDO_AT:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT);	//Envia comando AT
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_ATI:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kATI);	//Envia comando AT
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QGPS_PREG:
			ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
			ec25EnviarComandoAT(kAT_QGPS_PREG);	//Envia comando AT
			ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
			ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
			ec25_timeout = 0;	//reset a contador de tiempo
			break;

	case kFSM_ENVIANDO_QGPS:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QGPS);	//Envia comando AT
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CPIN:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_CPIN);	//Envia comando AT+CPIN?
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QCFG_NWSCANMODE:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QCFG_NWSCANMODE);	//Envia comando AT+CSQ
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QCFG_BAND:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QCFG_BAND);	//Envia comando AT+CSQ
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_CREG);	//Envia comando AT+CREG?
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CGREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_CGREG);	//Envia comando AT+CREG?
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CSQ:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_CSQ);	//Envia comando AT+CSQ
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QGPSLOC:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QGPSLOC);	//Envia comando AT
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTCFG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QMTCFG);	//Envia comando AT+CMGS="3003564960"
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTOPEN_PREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QMTOPEN_PREG);	//Envia comando AT+CMGS="3003564960"
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTOPEN:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QMTOPEN);	//Envia comando AT+CMGS="3003564960"
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTCONN_PREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QMTCONN_PREG);	//Envia comando AT+CMGS="3003564960"
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTCONN:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QMTCONN);	//Envia comando AT+CMGS="3003564960"
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTPUB:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		ec25EnviarComandoAT(kAT_QMTPUB);	//Envia comando AT+CMGS="3003564960"
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_MENSAJE_MQTT:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s,%s,%d.%d,%d.%d%c\r\n",fecha,location,intTemperatura,decTemperatura,intHumedad,decHumedad,0x1A);	//Envia mensaje de texto incluido  CTRL+Z (0x1A)
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ESPERANDO_RESPUESTA:
		ec25_timeout++;	//incrementa contador de tiempo

		//Busca si llegaron nuevos datos desde modem mientras esperaba
		while (uart0CuantosDatosHayEnBuffer() > 0) {
			resultado = uart0LeerByteDesdeBuffer(&nuevo_byte_uart);
			//si fueron encontrados nuevos bytes en UART entonces los mueve al buffer del EC25
			if (resultado == kStatus_Success) {
				//reinicia contador de tiempo
				ec25_timeout=0;
				//almacena dato en buffer rx de quectel
				ec25_buffer_rx[ec25_index_buffer_rx] = nuevo_byte_uart;
				//incrementa apuntador de datos en buffer de quectel
				ec25_index_buffer_rx++;
			}
		}

		//pregunta si el tiempo de espera supera el configurado
		if (ec25_timeout > EC25_TIEMPO_MAXIMO_ESPERA) {
			ec25_fsm.actual = kFSM_PROCESANDO_RESPUESTA;
		}
		break;

	case kFSM_PROCESANDO_RESPUESTA:
		//procesa respuesta dependiendo de cual comando AT se le había enviado al modem
		resultado = ec25ProcesarRespuestaAT(ec25_fsm.anterior);
		//Si la respuesta al comando AT es correcta (kStatus_Success), avanza al siguiente resultado
		switch (resultado) {
		case kStatus_Success:
			//el siguiente estado depende del estado anterior
			switch (ec25_fsm.anterior) {
			case kFSM_ENVIANDO_AT:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_ATI;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_ATI:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el resultado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QGPS_PREG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QGPS_PREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el resultado actual
				ec25_fsm.actual = kFSM_ENVIANDO_CPIN;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QGPS:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el resultado actual
				ec25_fsm.actual = kFSM_ENVIANDO_CPIN;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_CPIN:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QCFG_NWSCANMODE;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QCFG_NWSCANMODE:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QCFG_BAND;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QCFG_BAND:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_CREG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_CREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_CGREG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_CGREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_CSQ;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_CSQ:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el resultado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QMTOPEN_PREG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTOPEN_PREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QMTCONN_PREG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTCFG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QMTOPEN;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTOPEN:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				delayOpen();
				ec25_fsm.actual = kFSM_ENVIANDO_QMTCONN_PREG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTCONN_PREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QGPSLOC;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTCONN:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QGPSLOC;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QGPSLOC:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QMTPUB;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTPUB:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_MENSAJE_MQTT;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_MENSAJE_MQTT:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_RESULTADO_EXITOSO;//avanza a enviar nuevo comando al modem
				break;

			default:
				//para evitar bloqueos, marca error de proceso en caso de detectar un estado ilegal
				printf("Error \r\n");
				ec25_fsm.actual = kFSM_RESULTADO_ERROR;	//se queda en resultado de error
				break;
			}
			break;

		case kStatus_Fail:
			switch (ec25_fsm.anterior) {
			case kFSM_ENVIANDO_QGPS_PREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el resultado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QGPS;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTOPEN_PREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QMTCFG;//avanza a enviar nuevo comando al modem
				break;

			case kFSM_ENVIANDO_QMTCONN_PREG:
				ec25_fsm.anterior = ec25_fsm.actual;//almacena el estado actual
				ec25_fsm.actual = kFSM_ENVIANDO_QMTCONN;//avanza a enviar nuevo comando al modem
				break;

			default:
				//para evitar bloqueos, marca error de proceso en caso de detectar un estado ilegal
				printf("Error \r\n");
				ec25_fsm.actual = kFSM_RESULTADO_ERROR;	//se queda en resultado de error
				break;
			}


			break;

		case kStatus_OutOfRange:
			//si la respuesta del analisis es fuera de rango, indica que el RSSI no se cumple
			printf("Fuera de rango RSSI \r\n");
			ec25_fsm.actual = kFSM_RESULTADO_ERROR_RSSI;//se queda en resultado de error
			break;

		default:
			//Si la respuesta es incorrecta, se queda en resultado de error
			//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
			printf("Error \r\n");
			ec25_fsm.actual = kFSM_RESULTADO_ERROR;	//se queda en resultado de error
			break;
		}

	}

	return(ec25_fsm.actual);

}

