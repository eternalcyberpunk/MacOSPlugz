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

/*	PunkDither.cpp	

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to 
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017
	1.1			Added 'Support URL' to PiPL and entry point				cjr			3/31/2023

*/

#include "PunkDither.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "Param_Utils.h"
#include "AEFX_SuiteHelper.h"
#include "AEGP_SuiteHandler.h"
#include <thread>
#include <vector>
#include <algorithm>
#include <omp.h> // OpenMP for parallel processing
#include <random>

using namespace std;


static PF_Err
About(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
		"%s v%d.%d\r%s",
		STR(StrID_Name),
		MAJOR_VERSION,
		MINOR_VERSION,
		STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION,
		MINOR_VERSION,
		BUG_VERSION,
		STAGE_VERSION,
		BUILD_VERSION);

	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc

	return PF_Err_NONE;
}



static PF_Err
ParamsSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	PF_Err      err = PF_Err_NONE;
	PF_ParamDef def;

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(
		"Dither Strength",  // UI Name
		0.0, 1.0, 0.0, 1.0, 0.5,  // Min, Max, Default
		PF_Precision_THOUSANDTHS, 0, 0,  // Precision & UI Flags
		1  // Param ID (matches Render)
	);

	// 🎨 Dither Color A (Dark)
	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(
		"Dither Color A",
		0, 0, 0, // Default: Black
		2 // Param ID
	);

	// 🎨 Dither Color B (Bright)
	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(
		"Dither Color B",
		255, 255, 254, // Default: White
		3 // Param ID
	);

	// 🎛 Direction Dropdown (1 = Up, 2 = Down, 3 = Left, 4 = Right)
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(
		"Dither Direction",
		4, // Number of choices
		2, // Default (2 = Down)
		"Up|Down|Left|Right", // Labels
		4  // Param ID
	);

	// ⚠️ Non-Editable Warning Dropdown
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(
		"Avoid Pure Red & Pure White",
		1,  // Only one option
		1,  // Default selection
		"For Color B",  // Message shown in dropdown
		5  // Param ID
	);

	// Add a new dropdown for Dithering Algorithm
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(
		"Dithering Algorithm",
		3, // Number of choices
		1, // Default (1 = Error Diffusion)
		"Error Diffusion|Bayer Matrix|Blue Noise", // Labels
		6  // Param ID
	);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(
		"Downscale Factor",
		10,  // Number of choices
		1,   // Default (1 = No downscale)
		"1x|2x|3x|4x|5x|6x|7x|8x|16x|32x",  // Labels
		7  // Param ID
	);


	out_data->num_params = 8;  // Updated to include the new dropdown


	return err;
}

static PF_Err
MySimpleGainFunc16(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel16* inP,
	PF_Pixel16* outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo* giP = reinterpret_cast<GainInfo*>(refcon);
	PF_FpLong	tempF = 0;

	if (giP) {
		tempF = giP->gainF * PF_MAX_CHAN16 / 100.0;
		if (tempF > PF_MAX_CHAN16) {
			tempF = PF_MAX_CHAN16;
		};

		outP->alpha = inP->alpha;
		outP->red = MIN((inP->red + (A_u_char)tempF), PF_MAX_CHAN16);
		outP->green = MIN((inP->green + (A_u_char)tempF), PF_MAX_CHAN16);
		outP->blue = MIN((inP->blue + (A_u_char)tempF), PF_MAX_CHAN16);
	}

	return err;
}

static PF_Err
MySimpleGainFunc8(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel8* inP,
	PF_Pixel8* outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo* giP = reinterpret_cast<GainInfo*>(refcon);
	PF_FpLong	tempF = 0;

	if (giP) {
		tempF = giP->gainF * PF_MAX_CHAN8 / 100.0;
		if (tempF > PF_MAX_CHAN8) {
			tempF = PF_MAX_CHAN8;
		};

		outP->alpha = inP->alpha;
		outP->red = MIN((inP->red + (A_u_char)tempF), PF_MAX_CHAN8);
		outP->green = MIN((inP->green + (A_u_char)tempF), PF_MAX_CHAN8);
		outP->blue = MIN((inP->blue + (A_u_char)tempF), PF_MAX_CHAN8);
	}

	return err;
}


using namespace std;

// Improved 8x8 Bayer matrix for smoother dithering
const int bayerMatrix8x8[8][8] = {
	{  0, 32,  8, 40,  2, 34, 10, 42 },
	{ 48, 16, 56, 24, 50, 18, 58, 26 },
	{ 12, 44,  4, 36, 14, 46,  6, 38 },
	{ 60, 28, 52, 20, 62, 30, 54, 22 },
	{  3, 35, 11, 43,  1, 33,  9, 41 },
	{ 51, 19, 59, 27, 49, 17, 57, 25 },
	{ 15, 47,  7, 39, 13, 45,  5, 37 },
	{ 63, 31, 55, 23, 61, 29, 53, 21 }
};

void ApplyBayerDither(PF_LayerDef* output, PunkDitherParams* params) {
	int width = output->extent_hint.right;
	int height = output->extent_hint.bottom;
	int rowbytes = output->rowbytes;

	float strength = params->strength * 4.0f; // Adjust dither intensity based on slider

#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < height; y++) {
		PF_Pixel8* row = (PF_Pixel8*)((char*)output->data + y * rowbytes);
		for (int x = 0; x < width; x++) {
			PF_Pixel8* pixel = &row[x];
			int threshold = bayerMatrix8x8[y % 8][x % 8] * strength; // Adjust with slider strength
			int grayscale = (pixel->red + pixel->green + pixel->blue) / 3;
			bool ditherMask = grayscale > threshold;
			pixel->red = ditherMask ? params->colorB.red : params->colorA.red;
			pixel->green = ditherMask ? params->colorB.green : params->colorA.green;
			pixel->blue = ditherMask ? params->colorB.blue : params->colorA.blue;
		}
	}
}


void ApplyBlueNoiseDither(PF_LayerDef* output, PunkDitherParams* params) {
	int width = output->extent_hint.right - output->extent_hint.left;
	int height = output->extent_hint.bottom - output->extent_hint.top;
	int rowbytes = output->rowbytes;
	std::vector<int> noise(width * height);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, 255);

	for (int i = 0; i < width * height; i++) {
		noise[i] = dis(gen);
	}

	for (int y = 0; y < height; y++) {
		PF_Pixel8* row = (PF_Pixel8*)((char*)output->data + y * rowbytes);
		for (int x = 0; x < width; x++) {
			PF_Pixel8* pixel = &row[x];
			int grayscale = (pixel->red + pixel->green + pixel->blue) / 3;
			int threshold = noise[y * width + x] * params->strength;
			bool ditherMask = grayscale > threshold;
			pixel->red = ditherMask ? params->colorB.red : params->colorA.red;
			pixel->green = ditherMask ? params->colorB.green : params->colorA.green;
			pixel->blue = ditherMask ? params->colorB.blue : params->colorA.blue;
		}
	}
}





#include <cstdlib>  // For randomness

void ApplyPunkDither(PF_LayerDef* output, PunkDitherParams* params, int direction) {
	if (params->strength < 0.01) return;

	int width = output->extent_hint.right;
	int height = output->extent_hint.bottom;
	int rowbytes = output->rowbytes;
	PF_FpLong strength = MAX(0.05, params->strength);



	if (direction == 1) {  // 🔼 UP - Process bottom to top (Unmodified)
		for (int y = height - 1; y > 0; y--) {
			PF_Pixel8* row = (PF_Pixel8*)((char*)output->data + y * rowbytes);
			PF_Pixel8* rowAbove = (PF_Pixel8*)((char*)output->data + (y - 1) * rowbytes);

			for (int x = 1; x < width - 1; x++) {
				PF_Pixel8* pixel = &row[x];
				int grayscale = (pixel->red + pixel->green + pixel->blue) / 3;
				int threshold = 128 * (1.0 - strength);
				threshold = MAX(64, MIN(192, threshold));
				bool ditherMask = (grayscale > threshold);
				int ditherValue = ditherMask ? 255 : 0;
				int err = grayscale - ditherValue;
				int diffusionFactor = 8 + (8 * strength);

				if (y > 0) {
					rowAbove[x].red = MIN(255, MAX(0, rowAbove[x].red + err * 8 / diffusionFactor));
					rowAbove[x].green = MIN(255, MAX(0, rowAbove[x].green + err * 8 / diffusionFactor));
					rowAbove[x].blue = MIN(255, MAX(0, rowAbove[x].blue + err * 8 / diffusionFactor));
				}
				pixel->red = ditherMask ? params->colorB.red : params->colorA.red;
				pixel->green = ditherMask ? params->colorB.green : params->colorA.green;
				pixel->blue = ditherMask ? params->colorB.blue : params->colorA.blue;
			}
		}
	}
	else if (direction == 2) {  // 🔽 DOWN - Process top to bottom
		for (int y = 0; y < height - 1; y++) {
			PF_Pixel8* row = (PF_Pixel8*)((char*)output->data + y * rowbytes);
			PF_Pixel8* rowBelow = (PF_Pixel8*)((char*)output->data + (y + 1) * rowbytes);

			for (int x = 1; x < width - 1; x++) {
				PF_Pixel8* pixel = &row[x];
				int grayscale = (pixel->red + pixel->green + pixel->blue) / 3;
				int threshold = 128 * (1.0 - strength);
				threshold = MAX(64, MIN(192, threshold));
				bool ditherMask = (grayscale > threshold);
				int ditherValue = ditherMask ? 255 : 0;
				int err = grayscale - ditherValue;
				int diffusionFactor = 8 + (8 * strength);

				if (y < height - 1) {
					rowBelow[x].red = MIN(255, MAX(0, rowBelow[x].red + err * 8 / diffusionFactor));
					rowBelow[x].green = MIN(255, MAX(0, rowBelow[x].green + err * 8 / diffusionFactor));
					rowBelow[x].blue = MIN(255, MAX(0, rowBelow[x].blue + err * 8 / diffusionFactor));
				}
				pixel->red = ditherMask ? params->colorB.red : params->colorA.red;
				pixel->green = ditherMask ? params->colorB.green : params->colorA.green;
				pixel->blue = ditherMask ? params->colorB.blue : params->colorA.blue;
			}
		}
	}
	else if (direction == 3) {  // ◀ LEFT - FIXED (Prevents dithering issues with white)
		if (strength > 0.01) {
			for (int y = 0; y < height; y++) {
				PF_Pixel8* row = (PF_Pixel8*)((char*)output->data + y * rowbytes);

				for (int x = width - 1; x > 0; x--) {
					PF_Pixel8* pixel = &row[x];
					int grayscale = (pixel->red + pixel->green + pixel->blue) / 3;
					int threshold = 128 * (1.0 - strength);
					threshold = MAX(64, MIN(192, threshold));
					bool ditherMask = (grayscale > threshold);
					int ditherValue = ditherMask ? 255 : 0;
					int err = grayscale - ditherValue;
					int diffusionFactor = 8 + (8 * strength);

					if (x > 0) {
						row[x - 1].red = MIN(255, MAX(0, row[x - 1].red + err * 8 / diffusionFactor));
						row[x - 1].green = MIN(255, MAX(0, row[x - 1].green + err * 8 / diffusionFactor));
						row[x - 1].blue = MIN(255, MAX(0, row[x - 1].blue + err * 8 / diffusionFactor));
					}

					// 🛠 FIX: Properly handle bright pixels (Prevent unwanted dithering on white)
					if (pixel->red > 240 && pixel->green > 240 && pixel->blue > 240) {
						pixel->red = params->colorB.red;
						pixel->green = params->colorB.green;
						pixel->blue = params->colorB.blue;
					}
					else {
						pixel->red = ditherMask ? params->colorB.red : params->colorA.red;
						pixel->green = ditherMask ? params->colorB.green : params->colorA.green;
						pixel->blue = ditherMask ? params->colorB.blue : params->colorA.blue;
					}
				}
			}
		}
	}
	else if (direction == 4) {  // ▶ RIGHT - Unchanged
		for (int y = 0; y < height; y++) {
			PF_Pixel8* row = (PF_Pixel8*)((char*)output->data + y * rowbytes);

			for (int x = 0; x < width - 1; x++) {
				PF_Pixel8* pixel = &row[x];
				int grayscale = (pixel->red + pixel->green + pixel->blue) / 3;
				int threshold = 128 * (1.0 - strength);
				threshold = MAX(64, MIN(192, threshold));
				bool ditherMask = (grayscale > threshold);
				int ditherValue = ditherMask ? 255 : 0;
				int err = grayscale - ditherValue;
				int diffusionFactor = 8 + (8 * strength);

				if (x < width - 1) {
					row[x + 1].red = MIN(255, MAX(0, row[x + 1].red + err * 8 / diffusionFactor));
					row[x + 1].green = MIN(255, MAX(0, row[x + 1].green + err * 8 / diffusionFactor));
					row[x + 1].blue = MIN(255, MAX(0, row[x + 1].blue + err * 8 / diffusionFactor));
				}
				pixel->red = ditherMask ? params->colorB.red : params->colorA.red;
				pixel->green = ditherMask ? params->colorB.green : params->colorA.green;
				pixel->blue = ditherMask ? params->colorB.blue : params->colorA.blue;
			}
		}
	}


}

void RetroDitherDownscale(PF_LayerDef* input, PF_LayerDef* output, int downscaleFactor) {
	if (downscaleFactor <= 1) {
		return;  // No downscaling needed
	}

	int width = input->width;
	int height = input->height;
	int downWidth = width / downscaleFactor;
	int downHeight = height / downscaleFactor;

	std::vector<PF_Pixel8> downsampled(downWidth * downHeight);

	// Step 1: Downscale - Sample nearest pixel without averaging
	for (int y = 0; y < downHeight; y++) {
		for (int x = 0; x < downWidth; x++) {
			int srcX = x * downscaleFactor;
			int srcY = y * downscaleFactor;

			PF_Pixel8* srcPixel = (PF_Pixel8*)((char*)input->data + srcY * input->rowbytes) + srcX;
			downsampled[y * downWidth + x] = *srcPixel;
		}
	}

	// Step 2: Upscale - Restore to original size using nearest-neighbor
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int srcX = x / downscaleFactor;
			int srcY = y / downscaleFactor;

			PF_Pixel8* dstPixel = (PF_Pixel8*)((char*)output->data + y * output->rowbytes) + x;
			*dstPixel = downsampled[srcY * downWidth + srcX];
		}
	}
}



static PF_Err Render(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
    PF_Err err = PF_Err_NONE;

    PF_LayerDef* input = &params[0]->u.ld;

    // 🎯 Step 1: Copy Input Image to Output (Preserve Original Image)
    memcpy(output->data, input->data, output->height * output->rowbytes);

    // 🎯 Step 2: Downsample Details (While Keeping Original Dimensions)
    float scaleFactor = 0.5;  // Example: Reduce effective resolution by 50%
	RetroDitherDownscale(input, output, scaleFactor);

    // 🎯 Step 3: Get UI Parameters
    PunkDitherParams dither;
    dither.strength = params[1]->u.fs_d.value;
    dither.colorA = params[2]->u.cd.value;
    dither.colorB = params[3]->u.cd.value;
    dither.algorithm = params[6]->u.pd.value;

    int ditherDirection = params[4]->u.pd.value;

    // 🎛 Step 4: Apply Selected Dithering Algorithm
    switch (dither.algorithm) {
        case 1: ApplyPunkDither(output, &dither, ditherDirection); break;
        case 2: ApplyBayerDither(output, &dither); break;
        case 3: ApplyBlueNoiseDither(output, &dither); break;
    }

    return err;
}










extern "C" DllExport
PF_Err PluginDataEntryFunction2(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB2 inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT_EXT2(
		inPtr,
		inPluginDataCallBackPtr,
		"Punk Dither", // Name
		"ADBE Punk Dither", // Match Name
		"Eternal Cyberia", // Category
		AE_RESERVED_INFO, // Reserved Info
		"EffectMain",	// Entry point
		"https://www.adobe.com");	// support URL

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output,
	void* extra)
{
	PF_Err		err = PF_Err_NONE;

	try {
		switch (cmd) {
		case PF_Cmd_ABOUT:

			err = About(in_data,
				out_data,
				params,
				output);
			break;

		case PF_Cmd_GLOBAL_SETUP:

			err = GlobalSetup(in_data,
				out_data,
				params,
				output);
			break;

		case PF_Cmd_PARAMS_SETUP:

			err = ParamsSetup(in_data,
				out_data,
				params,
				output);
			break;

		case PF_Cmd_RENDER:

			err = Render(in_data,
				out_data,
				params,
				output);
			break;
		}
	}
	catch (PF_Err& thrown_err) {
		err = thrown_err;
	}
	return err;
}

