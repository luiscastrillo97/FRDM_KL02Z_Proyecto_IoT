/*! @file : main.c
 * @author  Luis José Castrillo Fernández y Elías David Garcés Cantillo
 * @version 1.0.0
 * @date    16/01/2021
 * @brief   Archivo principal
 * @details
 *
*/
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL02Z4.h"
#include "fsl_debug_console.h"


#include "sdk_hal_uart0.h"
#include "sdk_hal_gpio.h"
#include "sdk_hal_i2c0.h"
#include "sdk_hal_i2c1.h"
#include "sdk_hal_lptmr0.h"

#include "sdk_mdlw_leds.h"
#include "sdk_pph_ec25au.h"
#include "sdk_pph_sht3x.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define HABILITAR_MODEM_EC25		1
#define HABILITAR_SENSOR_SHT3X		1
#define HABILITAR_TLPTMR0			1


/*******************************************************************************
 * Private Prototypes
 ******************************************************************************/


/*******************************************************************************
 * External vars
 ******************************************************************************/


/*******************************************************************************
 * Local vars
 ******************************************************************************/


/*******************************************************************************
 * Private Source Code
 ******************************************************************************/
void delay(void) {
	//espera a que hayan ocurrido por lo menos 100ms interrupciones
	while (lptmr0GetTimeValue() < 200){
	}
	lptmr0SetTimeValue(0);		//Reset de variable contador de interrupciones
}

/*
 * @brief   Application entry point.
 */
int main(void) {

	uint8_t ec25_estado_actual;
	uint16_t enteroT, decimalT, enteroH, decimalH;
	float temperatura, humedad;
	uint16_t obtener_temperatura, obtener_humedad;

	sht3x_data_t sht3x_datos;
	uint8_t sht3x_detectado=0;

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

	#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		BOARD_InitDebugConsole();
	#endif

		printf("Inicializa UART0:");
		//inicializa puerto UART0 y solo avanza si es exitoso el proceso
		if(uart0Inicializar(115200)!=kStatus_Success){	//115200bps
			printf("Error");
			return 0 ;
		};
		printf("OK\r\n");

		printf("Inicializa I2C0:");
		//inicializa puerto I2C0 y solo avanza si es exitoso el proceso
		if(i2c0MasterInit(100000)!=kStatus_Success){	//100kbps
			printf("Error");
			return 0 ;
		}
		printf("OK\r\n");

		printf("Inicializa I2C1:");
		//inicializa puerto I2C1 y solo avanza si es exitoso el proceso
		if(i2c1MasterInit(100000)!=kStatus_Success){	//100kbps
			printf("Error");
			return 0 ;
		}
		printf("OK\r\n");

	#if HABILITAR_SENSOR_SHT3X
		printf("Detectando SHT3X:");
		//LLamado a funcion que identifica sensor SHT3X
		if(sht3xInit()== kStatus_Success){
			sht3x_detectado=1;
			printf("OK\r\n");
		}
		else {
			printf("No detectado\r\n");
		}
	#endif


	#if HABILITAR_MODEM_EC25
		//Inicializa todas las funciones necesarias para trabajar con el modem EC25
		printf("Inicializa modem EC25\r\n");
		ec25Inicializacion();

	#endif

	#if HABILITAR_TLPTMR0
		//Inicializa todas las funciones necesarias para trabajar con el modem EC25
		printf("Inicializa low power timer 0\r\n");

		lptmr0Init();
	#endif

	//Ciclo infinito encendiendo y apagando led verde
	//inicia SUPERLOOP
    while(1) {
    	delay();		//base de tiempo fija aproximadamente 200ms

		#if HABILITAR_SENSOR_SHT3X
				if (sht3xReadData(&sht3x_datos) == kStatus_Success) {//toma lectura humedad, temperatura
					obtener_temperatura = sht3x_datos.temperatura;	//temperatura sin procesar
					obtener_humedad = sht3x_datos.humedad;	//humedad sin procesar
				}
				temperatura = -45 + (175*(float)(obtener_temperatura))/(65535);
				enteroT = (uint16_t)(temperatura);
				decimalT = (uint16_t)((temperatura-(float)(enteroT))*10000);
				enteroTemperatura(enteroT);
				decimalTemperatura(decimalT);

				humedad = (100*(float)(obtener_humedad))/(65535);
				enteroH = (uint16_t)(humedad);
				decimalH = (uint16_t)((humedad-(float)(enteroH))*10000);
				enteroHumedad(enteroH);
				decimalHumedad(decimalH);
		#endif

		#if HABILITAR_MODEM_EC25
				ec25_estado_actual = ec25Polling();	//actualiza maquina de estados encargada de avanzar en el proceso interno del MODEM
													//retorna el estado actual de la FSM

				switch(ec25_estado_actual){			//controla color de los LEDs dependiendo de estado modemEC25

				case kFSM_RESULTADO_ERROR:
					toggleLedRojo();
					apagarLedVerde();
					apagarLedAzul();
					break;

				case kFSM_RESULTADO_EXITOSO:
					apagarLedRojo();
					toggleLedVerde();
					apagarLedAzul();
					break;

				case kFSM_ESPERANDO_NUEVO_CICLO:
					printf("Esperando nuevo ciclo\r\n");
					apagarLedRojo();
					encenderLedVerde();
					apagarLedAzul();
					break;

				case kFSM_RESULTADO_ERROR_RSSI:
					toggleLedRojo();
					apagarLedVerde();
					toggleLedAzul();
					break;

				default:
					apagarLedRojo();
					apagarLedVerde();
					toggleLedAzul();
					break;
				}
		#endif
			}
    return 0 ;
}
