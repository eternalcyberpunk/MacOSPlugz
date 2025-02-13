/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007-2023 Adobe Inc.                                  */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Inc. and its suppliers, if                    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Inc. and its                    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Inc.            */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*
	PunkDither.h
*/

#pragma once

#ifndef PUNKDITHER_H
#define PUNKDITHER_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "PunkDither_Strings.h"

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	1
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameters */
enum {
	PUNKDITHER_INPUT = 0,
	PUNKDITHER_STRENGTH,  // Dither Strength Slider
	PUNKDITHER_COLOR_A,   // Dark Color
	PUNKDITHER_COLOR_B,   // Bright Color
	PUNKDITHER_DIRECTION, // Dither Direction (Up, Down, Left, Right)
	PUNKDITHER_ALGORITHM, // Dithering Algorithm (Error Diffusion, Bayer, Blue Noise)
	PUNKDITHER_DOWNSCALE, // Downscale Factor (1x to 32x)
	PUNKDITHER_NUM_PARAMS
};

enum {
	DITHER_ID = 1 // 🎯 Only keeping dither param
};

/* Dithering Parameters */
typedef struct PunkDitherParams {
	PF_FpLong strength; // Dither intensity
	PF_Pixel8 colorA;    // Dark Color
	PF_Pixel8 colorB;    // Bright Color
	int algorithm;       // Dithering Algorithm (1 = Error Diffusion, 2 = Bayer, 3 = Blue Noise)
	int downscaleFactor; // Downscale Factor (1x, 2x, 3x, ..., 32x)
} PunkDitherParams;

typedef struct GainInfo {
	PF_FpLong	gainF;
} GainInfo, * GainInfoP, ** GainInfoH;




extern "C" {

	DllExport
		PF_Err
		EffectMain(
			PF_Cmd			cmd,
			PF_InData* in_data,
			PF_OutData* out_data,
			PF_ParamDef* params[],
			PF_LayerDef* output,
			void* extra);

}

#endif // PUNKDITHER_H