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
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MMA851_I2C_DEVICE_ADDRESS	0x1D	//Dirección del acelerómetro MMA8451
const uint16_t k_SENSITIVITY_2G = 4096;		//Sensibilidad del acelerómetro para el rango de +- 2g.

/*******************************************************************************
 * Private Prototypes
 ******************************************************************************/
void calibrarAcelerometro();	//Prototipo de la función de calibración del acelerómetro.

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
	uint8_t	nuevo_dato_i2c, nuevo_dato, dato_x_msb, dato_x_lsb, dato_y_msb, dato_y_lsb, dato_z_msb, dato_z_lsb;
	uint16_t dato_x_14_bits, dato_y_14_bits, dato_z_14_bits, dato_x_grados, dato_y_grados, dato_z_grados;


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

    //Inicializando el acelerómetro en el rango de +-2g donde 1g = 4096 cuentas
    (void)i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1, 0x00);
    (void)i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, XYZ_DATA_CFG, 0x00);
    (void)i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1, 0x01);

    //Valores a digitar para obtener información o realizar una acción
    printf("Usar teclado para controlar LEDs y mostrar valores del acelerometro MMA8451\r\n");
    printf("R: Encender. r: Apagar. Led Rojo\r\n");
    printf("V: Encender. v: Apagar. Led Verde\r\n");
    printf("A: Encender. a: Apagar. Led Azul\r\n");
    printf("M: Encender. m: Apagar. Color Magenta\r\n");
    printf("C: Encender. c: Apagar. Color Cyan\r\n");
    printf("L: Encender. l: Apagar. Color Amarillo\r\n");
    printf("b: buscar acelerometro\r\n");
    printf("n: Calibrar acelerometro\r\n");
    printf("o: Cambiar a modo Activo\r\n");
    printf("f: Cambiar a modo StandBy\r\n");
    printf("e: Verificar en que modo se encuentra el acelerometro\r\n");
    printf("x o X: Mostrar valores del eje x\r\n");
    printf("y o Y: Mostrar valores del eje y\r\n");
    printf("z o Z: Mostrar valores del eje z\r\n");


    while(1) {

    	//Verificar si el Buffer contiene datos para mostrar
    	if(uart0CuantosDatosHayEnBuffer()>0){

    		//Leer los datos del Buffer
    		status=uart0LeerByteDesdeBuffer(&nuevo_byte_uart);

    		if(status==kStatus_Success){

    			printf("dato:%c\r\n",nuevo_byte_uart);

    			/*Selector para determinar que acción realizar.*/
    			switch (nuevo_byte_uart) {

    			//Encender LED Azul
    			case 'A':
					gpioPutLow(KPTB10);
					break;

				//Apagar LED Azul
    			case 'a':
					gpioPutHigh(KPTB10);
					break;

				//Encender LED Verde
    			case 'V':
					gpioPutLow(KPTB7);
					break;
				//Apagar LED Verde
				case 'v':
					gpioPutHigh(KPTB7);
					break;

				//Encender LED Rojo
				case 'R':
					gpioPutValue(KPTB6,0);
					break;
				//Apagar LED Rojo
				case 'r':
					gpioPutValue(KPTB6,1);
					break;

				//Encender color Magenta
				case 'M':
					gpioPutValue(KPTB6,0);
					gpioPutValue(KPTB7,1);
					gpioPutValue(KPTB10,0);
					break;
				//Apagar color Magenta
				case 'm':
					gpioPutValue(KPTB6,1);
					gpioPutValue(KPTB7,1);
					gpioPutValue(KPTB10,1);
					break;

				//Encender color Amarillo
				case 'L':
					gpioPutValue(KPTB6,0);
					gpioPutValue(KPTB7,0);
					gpioPutValue(KPTB10,1);
					break;
				//Apagar color Amarillo
				case 'l':
					gpioPutValue(KPTB6,1);
					gpioPutValue(KPTB7,1);
					gpioPutValue(KPTB10,1);
					break;

				//Encender color Cyan
				case 'C':
					gpioPutValue(KPTB6,1);
					gpioPutValue(KPTB7,0);
					gpioPutValue(KPTB10,0);
					break;
				//Apagar color Cyan
				case 'c':
					gpioPutValue(KPTB6,1);
					gpioPutValue(KPTB7,1);
					gpioPutValue(KPTB10,1);
					break;

				//Comprobar si hay conexión con el acelerómetro
				case 'b':
					i2c0MasterReadByte(&nuevo_dato_i2c, MMA851_I2C_DEVICE_ADDRESS, WHO_AM_I);
					if(nuevo_dato_i2c==0x1A)
						printf("MMA8451 encontrado!!\r\n");
					else
						printf("MMA8451 error\r\n");

					break;

				//Comprobar estado del acelerómetro (StandBy o Active)
				case 'e':
					i2c0MasterReadByte(&nuevo_dato, MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1);
					if(nuevo_dato==0x01){
						printf("MMA8451 Active\r\n");
					}
					else if (nuevo_dato == 0x00){
						printf("MMA8451 Standby\r\n");
					}
					break;

				//Colocar el acelerómetro en Modo Activo
				case 'o':
					i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1, 0x01);
					break;
				//Colocar el acelerómetro en Modo Standby
				case 'f':
					i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1, 0x00);
					break;

				//Extraer los valores del eje x del acelerómetro
				case 'x':
				case 'X':
					i2c0MasterReadByte(&dato_x_msb, MMA851_I2C_DEVICE_ADDRESS, OUT_X_MSB);
					i2c0MasterReadByte(&dato_x_lsb, MMA851_I2C_DEVICE_ADDRESS, OUT_X_LSB);
					dato_x_14_bits  = (uint16_t)(dato_x_msb << 6 | dato_x_lsb >> 2);
					dato_x_grados = (dato_x_14_bits*90)/k_SENSITIVITY_2G;
					printf("%i Cuentas",dato_x_14_bits);
					printf("\t%i Grados\r\n",dato_x_grados);
					break;

				//Extraer los valores del eje y del acelerómetro
				case 'y':
				case 'Y':
					i2c0MasterReadByte(&dato_y_msb, MMA851_I2C_DEVICE_ADDRESS, OUT_Y_MSB);
					i2c0MasterReadByte(&dato_y_lsb, MMA851_I2C_DEVICE_ADDRESS, OUT_Y_LSB);
					dato_y_14_bits  = (uint16_t)(dato_y_msb << 6 | dato_y_lsb >> 2);
					dato_y_grados = (dato_y_14_bits*90)/k_SENSITIVITY_2G;
					printf("%i Cuentas",dato_y_14_bits);
					printf("\t%i Grados\r\n",dato_y_grados);
					break;

				//Extraer los valores del eje z del acelerómetro
				case 'z':
				case 'Z':
					i2c0MasterReadByte(&dato_z_msb, MMA851_I2C_DEVICE_ADDRESS, OUT_Z_MSB);
					i2c0MasterReadByte(&dato_z_lsb, MMA851_I2C_DEVICE_ADDRESS, OUT_Z_LSB);
					dato_z_14_bits  = (uint16_t)(dato_z_msb << 6 | dato_z_lsb >> 2);
					dato_z_grados = (dato_z_14_bits*90)/k_SENSITIVITY_2G;
					printf("%i Cuentas",dato_z_14_bits);
					printf("\t%i Grados\r\n",dato_z_grados);
					break;

				//Calibrar el acelerómetro (reescribir el offset)
				case 'n':
					calibrarAcelerometro();
					break;
				}
    		}else{
    			printf("error\r\n");
    		}
    	}
    }

    return 0 ;
}

/*Función para calibrar el acelerómetro en el rango de +-2g*/
void calibrarAcelerometro(){

	/*Variables de la función*/
	uint8_t dato_x_msb, dato_x_lsb, dato_y_msb, dato_y_lsb, dato_z_msb, dato_z_lsb;
	uint16_t dato_x_14_bits, dato_y_14_bits, dato_z_14_bits, dato_x_offset, dato_y_offset, dato_z_offset;

	/*Extraer los valores de los ejes x, y, z del acelerómetro*/
	i2c0MasterReadByte(&dato_x_msb, MMA851_I2C_DEVICE_ADDRESS, OUT_X_MSB);
	i2c0MasterReadByte(&dato_x_lsb, MMA851_I2C_DEVICE_ADDRESS, OUT_X_LSB);
	i2c0MasterReadByte(&dato_y_msb, MMA851_I2C_DEVICE_ADDRESS, OUT_Y_MSB);
	i2c0MasterReadByte(&dato_y_lsb, MMA851_I2C_DEVICE_ADDRESS, OUT_Y_LSB);
	i2c0MasterReadByte(&dato_z_msb, MMA851_I2C_DEVICE_ADDRESS, OUT_Z_MSB);
	i2c0MasterReadByte(&dato_z_lsb, MMA851_I2C_DEVICE_ADDRESS, OUT_Z_LSB);

	dato_x_14_bits = (uint16_t)(dato_x_msb << 6 | dato_x_lsb >> 2);           // Calcula el valor del eje x en una variable de 16 bits
	dato_y_14_bits = (uint16_t)(dato_y_msb << 6 | dato_y_lsb >> 2);           // Calcula el valor del eje y en una variable de 16 bits
	dato_z_14_bits = (uint16_t)(dato_z_msb << 6 | dato_z_lsb >> 2);           // Calcula el valor del eje z en una variable de 16 bits

	dato_x_offset = (dato_x_14_bits/8)*(-1);        // Calcula el valor del offset del eje x
	dato_y_offset = (dato_y_14_bits/8)*(-1);        // Calcula el valor del offset del eje x
	dato_z_offset = (dato_z_14_bits-k_SENSITIVITY_2G)/8*(-1);          // Calcula el valor del offset del eje x

	// Colocar el acelerómetro en Standby mode para que permita registrar los nuevos valores del offset
	i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1, 0x00);

	//Escribir valores de offset de cada eje
	i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, OFF_X, dato_x_offset);
	i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, OFF_Y, dato_y_offset);
	i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, OFF_Z, dato_z_offset);

	//Volver al Active mode
	i2c0MasterWriteByte(MMA851_I2C_DEVICE_ADDRESS, CTRL_REG1, 0x01);
}
