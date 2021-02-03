/*! @file : sdk_pph_ec25au.h
 * @author  Luis José Castrillo Fernández
 * @version 1.0.0
 * @date    30 ene. 2021
 * @brief   Driver para EC25au
 * @details
 *
 */
#ifndef SDK_PPH_EC25AU_H_
#define SDK_PPH_EC25AU_H_
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "sdk_hal_uart0.h"

/*!
 * @addtogroup PPH
 * @{
 */
/*!
 * @addtogroup EC25AU
 * @{
 */
/*******************************************************************************
 * Public Definitions
 ******************************************************************************/
enum _ec25_lista_comandos_at {
	kAT = 0,
	kATI,
	kAT_CPIN,
	kAT_QCFG_NWSCANMODE,
	kAT_QCFG_BAND,
	kAT_CREG,
	kAT_CGREG,
	kAT_CSQ,
	kAT_QICSGP,
	kAT_CGDCONT,
	kAT_QIACT,
	kAT_QIACT_PREG,
	kAT_QMTCFG,
	kAT_QMTOPEN,
	kAT_QMTCONN,
	kAT_QMTPUB,
	kAT_MENSAJE_END,
};

enum _fsm_ec25_state{
	kFSM_INICIO=0,
	kFSM_ENVIANDO_AT,
	kFSM_ENVIANDO_ATI,
	kFSM_ENVIANDO_CPIN,
	kFSM_ENVIANDO_QCFG_NWSCANMODE,
	kFSM_ENVIANDO_QCFG_BAND,
	kFSM_ENVIANDO_CREG,
	kFSM_ENVIANDO_CGREG,
	kFSM_ENVIANDO_CSQ,
	kFSM_ENVIANDO_QICSGP,
	kFSM_ENVIANDO_CGDCONT,
	kFSM_ENVIANDO_QIACT,
	kFSM_ENVIANDO_QIACT_PREG,
	kFSM_ENVIANDO_QMTCFG,
	kFSM_ENVIANDO_QMTOPEN,
	kFSM_ENVIANDO_QMTCONN,
	kFSM_ENVIANDO_QMTPUB,
	kFSM_ENVIANDO_MENSAJE_MQTT,
	kFSM_ESPERANDO_RESPUESTA,
	kFSM_RESULTADO_ERROR,
	kFSM_RESULTADO_EXITOSO,
	kFSM_PROCESANDO_RESPUESTA,
	kFSM_RESULTADO_ERROR_RSSI,
};

#define EC25_TIEMPO_MAXIMO_ESPERA	3		//Tiempo maximo que espera modem por respuesta
#define EC25_RSSI_MINIMO_ACEPTADO	20		//RSSI minimo aceptado segun tabla de fabricante
/*******************************************************************************
 * External vars
 ******************************************************************************/

/*******************************************************************************
 * Public vars
 ******************************************************************************/

/*******************************************************************************
 * Public Prototypes
 ******************************************************************************/
status_t ec25Inicializacion(void);
status_t ec25EnviarMensajeMqtt(uint8_t *mensaje, uint8_t size_mensaje );
uint8_t ec25Polling(void);

/** @} */ // end of X group
/** @} */ // end of X group

#endif /* SDK_PPH_EC25AU_H_ */
