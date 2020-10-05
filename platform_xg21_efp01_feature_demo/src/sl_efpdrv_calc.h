/**************************************************************************/
//** EFP01 Configuration Tool Generated Configuration Header **//

#include "sl_efp01.h"

//** Generated EFP01 General Configuration Settings **//
//		VOA_V=0x04: VOA_V=4
//		BB_IPK=0x48: BB_IPK_EM2=2, BB_IPK=8
//		BB_CTRL5=0x80: BB_DRVR_SPEED=2, BB_IPK_BOOST_ADJ=0
//		BB_CTRL6=0x00: BB_IPK_NOADJ=0, SW_FAST=0, BB_IRI_CON=0, BB_TOFF_MAX=0
//		LDOC_BB_CTRL=0x50: SEQ_BB_FIRST=0, BB_TON_MAX=5, VOC_IRI_CON=0
//		BB_CTRL3=0xB5: NTM_LDO_THRSH=5, NTM_DUR=2, BB_MODE=5
//		VOB_EM0_V=0x8D: OOR_DIS=1, VOB_EM0_V=13
//		VOB_EM2_V=0x0D: VOB_EM2_V=13
//		BK_IPK=0x48: BK_IPK_EM2=2, BK_IPK=8
//		BK_CTRL2=0x50: BK_RES_TON_ONLY=0, BK_LDO_THRESH=5, BK_IRI_CON=0
//		LDOB_CTRL=0x0C: LDO_NO_AUTO_BYP=0, LDOB_BYP=0, LDOB_VMIN=0, 
//						LDOB_IGAIN=12
//		VOC_V=0x05: VOC_V=5
//		LDOC_CTRL=0x01: LDOC_ENA_SA=0, LDOC_BYP=0, LDOC_VMIN=0, LDOC_IGAIN=1

//** Configuration Changes from Original (i.e., OTP Default) Configuration **//
//		BB_IPK.BB_IPK value changed from 18 to 8
//		BB_IPK.BB_IPK_EM2 value changed from 4 to 2
//		BB_CTRL6.BB_TOFF_MAX value changed from 3 to 0
//		VOB_EM0_V.VOB_EM0_V value changed from 0 to 13
//		VOB_EM2_V.VOB_EM2_V value changed from 0 to 13
//		BK_IPK.BK_IPK value changed from 0 to 8
//		BK_IPK.BK_IPK_EM2 value changed from 0 to 2
//		LDOB_CTRL.LDOB_IGAIN value changed from 0 to 12
//		VOC_V.VOC_V value changed from 0 to 5
//		LDOC_CTRL.LDOC_IGAIN value changed from 12 to 1

#define SL_EFP_GEN_SIZE 13

#define SL_EFP_GEN {	\
	{EFP01_VOA_V, 0x04},	\
	{EFP01_BB_IPK, 0x48},	\
	{EFP01_BB_CTRL5, 0x80},	\
	{EFP01_BB_CTRL6, 0x00},	\
	{EFP01_LDOC_BB_CTRL, 0x50},	\
	{EFP01_BB_CTRL3, 0xB5},	\
	{EFP01_VOB_EM0_V, 0x8D},	\
	{EFP01_VOB_EM2_V, 0x0D},	\
	{EFP01_BK_IPK, 0x48},	\
	{EFP01_BK_CTRL2, 0x50},	\
	{EFP01_LDOB_CTRL, 0x0C},	\
	{EFP01_VOC_V, 0x05},	\
	{EFP01_LDOC_CTRL, 0x01},	\
}

// ** EFR32 DECOUPLE Handoff Sequence Configuration Settings ** //

// For applications where EFP01's DCDC B output (VOB) is powering the EFR32's 
// DECOUPLE supply, EFR32 must manage the handoff from EFR32's internal LDO to 
//  the EFP01 DCDC output.
//
// The EFP01 configuration settings below are passed to a EFP01 driver function 
// (sl_efp_decouple_handoff()) to manage a seamless transition of the DECOUPLE 
// power supply from the internal EFR32 LDO to the EFP01 DCDC B output. During 
// this transition, certain EFP01 settings (inrush current, on-time maximum, 
// peak current)  are momentarily set to conservative values, and then restored 
// to the user-desired configuration values.
//
// If this managed-handoff sequence is not used, there is the possibility of 
// creating a momentary voltage overshoot on the DECOUPLE that exceeds the 
// DECOUPLE pin maximum voltage. Therefore, the sl_efp_decouple_handoff() 
// function should be used whenever powering DECOUPLE from EFP01.
//
// If SL_EFP_DECOUPLE_HANDOFF_ARGS_SIZE > 0, the EFP01 init() function will 
// attempt to call the sl_efp_decouple_handoff() function; otherwise, 
// sl_efp_decouple_handoff() will not be called. 

#define SL_EFP_DECOUPLE_HANDOFF_ARGS_SIZE 3

// BK_IRI_CON=0, BK_TON_MAX=7, BK_IPK=8
#define SL_DECOUPLE_HANDOFF_ARGS {0, 7, 8}
