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

	/*Variables del programa*/
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

    /*Inicializando puerto UART*/
    (void)uart0Inicializar(115200);	//115200bps

    /*Menú de opciones*/
    PRINTF("Usar teclado para controlar LEDs\r\n");
    PRINTF("R: Encender. r: Apagar led ROJO\r\n");
    PRINTF("V: Encender. v: Apagar led VERDE\r\n");
    PRINTF("A: Encender. a: Apagar led AZUL\r\n");

    while(1) {
    	/*Verificar si hay datos en en buffer circular*/
    	if(uart0CuantosDatosHayEnBuffer()>0){

    		/*Leer los datos del buffer circular*/
    		status=uart0LeerByteDesdeBuffer(&nuevo_byte_uart);

    		/*Condicional que se ejecuta si se leen correctamente los datos*/
    		if(status==kStatus_Success){

    			/*Imprimir en pantalla la tecla oprimida*/
    			printf("dato:%c\r\n",nuevo_byte_uart);

    			/*Switch que ejecuta la acción según la opción seleccionada*/
    			switch (nuevo_byte_uart) {

    			//Encender Led Azul
				case 'A':
					gpioPutLow(KPTB10);
					break;
				//Apagar Led Azul
				case 'a':
					gpioPutHigh(KPTB10);
					break;

				//Encender Led Verde
				case 'V':
					gpioPutLow(KPTB7);
					break;
				//Apagar Led Verde
				case 'v':
					gpioPutHigh(KPTB7);
					break;

				//Encender Led Rojo
				case 'R':
					gpioPutValue(KPTB6,0);
					break;
				//Apagar Led Rojo
				case 'r':
					gpioPutValue(KPTB6,1);
					break;


				}
    		}else{
    			printf("error\r\n");
    		}
    	}
    }

    return 0 ;
}
