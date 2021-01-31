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
	kAT_CREG,
	kAT_CMGF_1,
	kAT_CMGS,
	kAT_TEXT_MSG_END,
};

enum _fsm_ec25_state{
	kFSM_INICIO=0,
	kFSM_ENVIANDO_AT,
	kFSM_ENVIANDO_ATI,
	kFSM_ENVIANDO_CPIN,
	kFSM_ENVIANDO_CREG,
	kFSM_ENVIANDO_CMGF,
	kFSM_ENVIANDO_CMGS,
	kFSM_ENVIANDO_MENSAJE_TXT,
	kFSM_ESPERANDO_RESPUESTA,
	kFSM_RESULTADO_ERROR,
	kFSM_RESULTADO_EXITOSO
};
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
status_t ec25EnviarMensajeDeTexto(uint8_t *mensaje, uint8_t size_mensaje );
uint8_t ec25Polling(void);
status_t detectarModemQuectel(void);

/** @} */ // end of X group
/** @} */ // end of X group

#endif /* SDK_PPH_EC25AU_H_ */
