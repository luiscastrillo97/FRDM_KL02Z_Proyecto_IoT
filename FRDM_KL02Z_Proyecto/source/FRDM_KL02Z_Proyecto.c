/*! @file : main.c
 * @author  Luis José Castrillo Fernández
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


#include "sdk_hal_gpio.h"
#include "sdk_hal_uart0.h"
#include "sdk_hal_i2c0.h"

#include "sdk_mdlw_leds.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/


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


/*******************************************************************************
 * Public Source Code
 ******************************************************************************/
int main(void) {

	/*Variables del proyecto*/
	status_t status;
	uint8_t nuevo_byte_uart;


  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    /*Inicializar el puerto UART0 y el bus I2C0*/
    (void)uart0Inicializar(115200);	//115200bps
    (void)i2c0MasterInit(100000);	//100kbps


    //Valores a digitar para obtener información o realizar una acción
    printf("Usar teclado para controlar LEDs y mostrar valores del acelerometro MMA8451\r\n");
    printf("R: Encender. r: Apagar. Led Rojo\r\n");
    printf("V: Encender. v: Apagar. Led Verde\r\n");
    printf("A o a: Encender o Apagar. Led Azul\r\n");


    while(1) {

    	//Verificar si el Buffer contiene datos para mostrar
    	if(uart0CuantosDatosHayEnBuffer()>0){

    		//Leer los datos del Buffer
    		status=uart0LeerByteDesdeBuffer(&nuevo_byte_uart);

    		if(status==kStatus_Success){

    			printf("dato:%c\r\n",nuevo_byte_uart);

    			/*Selector para determinar que acción realizar.*/
    			switch (nuevo_byte_uart) {

    			//Encender o Apagar Led Azul
				case 'a':
				case 'A':
					toggleLedAzul();
					break;

				//Encender Led Verder
				case 'V':
					encenderLedVerde();
					break;
				//Apagar Led Verde
				case 'v':
					apagarLedVerde();
					break;

				//Encender Led Rojo
				case 'R':
					encenderLedRojo();
					break;
				//Apagar Led Rojo
				case 'r':
					apagarLedRojo();
					break;


				}
    		}else{
    			printf("error\r\n");
    		}
    	}
    }

    return 0 ;
}

