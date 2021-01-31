/*! @file : sdk_pph_ec25au.c
 * @author  Luis José Castrillo Fernández
 * @version 1.0.0
 * @date    30 ene. 2021
 * @brief   Driver para EC25au
 * @details
 *
*/
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "stdio.h"
#include "sdk_pph_ec25au.h"
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
uint8_t buffer_rx_quectel[100];
uint8_t index_buffer_rx_quectel=0;

const uint8_t *comando_at_quectel[] = {
	"AT",
	"ATI",
	};

const uint8_t  *repuesta_at[]={
		"OK",
		"EC25",
};


/*******************************************************************************
 * Private Source Code
 ******************************************************************************/
void borrarBufferQuectel(void){
	uint8_t i;
	uint8_t bytes_en_buffer = sizeof(buffer_rx_quectel);

	for(i=0;i<bytes_en_buffer;i++){
		buffer_rx_quectel[i]=0;
	}
	index_buffer_rx_quectel=0;
}

void waytTimeModem(void) {
	uint32_t tiempo = 0xFFFF;
	do {
		tiempo--;
	} while (tiempo != 0x0000);
}

/*******************************************************************************
 * Public Source Code
 ******************************************************************************/
status_t detectarModemQuectel(void){
	uint8_t nuevo_byte_uart;
	status_t status;
	uint8_t *puntero_ok=0;
	uint32_t contador_tiempo_ms=0;

	borrarBufferQuectel();	//limpia buffer para recibir datos de quectel

	//envia comando AT a modem quectel
	printf("%s\r\n",comando_at_quectel[kATI]);

	do {
		waytTimeModem();
		contador_tiempo_ms++;

		if (uart0CuantosDatosHayEnBuffer() > 0) {
			status = uart0LeerByteDesdeBuffer(&nuevo_byte_uart);
			if (status == kStatus_Success) {
				//reinicia contador de tiempo
				contador_tiempo_ms=0;
				//almacena dato en buffer rx de quectel
				buffer_rx_quectel[index_buffer_rx_quectel] = nuevo_byte_uart;
				//incrementa apuntador de datos en buffer de quectel
				index_buffer_rx_quectel++;
			}
		}
	} while (contador_tiempo_ms < 5);

	//Busca palabra EC25 en buffer rx de quectel
	puntero_ok=(uint8_t *)(strstr((char*)(&buffer_rx_quectel[0]),(char *)(repuesta_at[kATI])));

	if(puntero_ok!=0){
		return(kStatus_Success);
	}else{
		return(kStatus_Fail);
	}
}
