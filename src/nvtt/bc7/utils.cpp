/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// Utility and common routines

#include "utils.h"
#include "avpcl.h"
#include <math.h>
#include <assert.h>
#include "rgba.h"
#include "arvo/Vec3.h"
#include "arvo/Vec4.h"

static int denom7_weights[] = {0, 9, 18, 27, 37, 46, 55, 64};										// divided by 64
static int denom15_weights[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};		// divided by 64

int Utils::lerp(int a, int b, int i, int bias, int denom)
{
#ifdef	USE_ZOH_INTERP
	assert (denom == 3 || denom == 7 || denom == 15);
	assert (i >= 0 && i <= denom);
	assert (bias >= 0 && bias <= denom/2);
	assert (a >= 0 && b >= 0);

	int round = 0;
#ifdef	USE_ZOH_INTERP_ROUNDED
	round = 32;
#endif

	switch (denom)
	{
	case 3:	denom *= 5; i *= 5;	// fall through to case 15
	case 15:return (a*denom15_weights[denom-i] + b*denom15_weights[i] + round) >> 6;
	case 7:	return (a*denom7_weights[denom-i] + b*denom7_weights[i] + round) >> 6;
	default: assert(0); return 0;
	}
#else
	return (((a)*((denom)-i)+(b)*(i)+(bias))/(denom));		// simple exact interpolation
#endif
}

Vec4 Utils::lerp(const Vec4& a, const Vec4 &b, int i, int bias, int denom)
{
#ifdef	USE_ZOH_INTERP
	assert (denom == 3 || denom == 7 || denom == 15);
	assert (i >= 0 && i <= denom);
	assert (bias >= 0 && bias <= denom/2);
//	assert (a >= 0 && b >= 0);

	// no need to bias these as this is an exact division

	switch (denom)
	{
	case 3:	denom *= 5; i *= 5;	// fall through to case 15
	case 15:return (a*denom15_weights[denom-i] + b*denom15_weights[i]) / 64.0;
	case 7:	return (a*denom7_weights[denom-i] + b*denom7_weights[i]) / 64.0;
	default: assert(0); return 0;
	}
#else
	return (((a)*((denom)-i)+(b)*(i)+(bias))/(denom));		// simple exact interpolation
#endif
}


int Utils::unquantize(int q, int prec)
{
	int unq;

	assert (prec > 3);	// we only want to do one replicate
	assert (RGBA_MIN == 0);

#ifdef USE_ZOH_QUANT
	if (prec >= 8)
		unq = q;
	else if (q == 0) 
		unq = 0;
	else if (q == ((1<<prec)-1)) 
		unq = RGBA_MAX;
	else
		unq = (q * (RGBA_MAX+1) + (RGBA_MAX+1)/2) >> prec;
#else
	// avpcl unquantizer -- bit replicate
	unq = (q << (8-prec)) | (q >> (2*prec-8));
#endif

	return unq;
}

// quantize to the best value -- i.e., minimize unquantize error
int Utils::quantize(float value, int prec)
{
	int q, unq;

	assert (prec > 3);	// we only want to do one replicate
	assert (RGBA_MIN == 0);

	unq = (int)floor(value + 0.5);
	assert (unq >= RGBA_MIN && unq <= RGBA_MAX);

#ifdef USE_ZOH_QUANT
	q = (prec >= 8) ? unq : (unq << prec) / (RGBA_MAX+1);
#else
	// avpcl quantizer -- scale properly for best possible bit-replicated result
	q = (unq * ((1<<prec)-1) + RGBA_MAX/2)/RGBA_MAX;
#endif

	assert (q >= 0 && q < (1 << prec));

	return q;
}

double Utils::metric4(const Vec4& a, const Vec4& b)
{
	Vec4 err = a - b;

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// weigh the components
		err.X() *= rwt;
		err.Y() *= gwt;
		err.Z() *= bwt;
	}

	return err * err;
}

// WORK -- implement rotatemode for the below -- that changes where the rwt, gwt, and bwt's go.
double Utils::metric3(const Vec3& a, const Vec3& b, int rotatemode)
{
	Vec3 err = a - b;

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// adjust weights based on rotatemode
		switch(rotatemode)
		{
		case ROTATEMODE_RGBA_RGBA: break;
		case ROTATEMODE_RGBA_AGBR: rwt = 1.0; break;
		case ROTATEMODE_RGBA_RABG: gwt = 1.0; break;
		case ROTATEMODE_RGBA_RGAB: bwt = 1.0; break;
		default: assert(0);
		}

		// weigh the components
		err.X() *= rwt;
		err.Y() *= gwt;
		err.Z() *= bwt;
	}

	return err * err;
}

double Utils::metric1(const float a, const float b, int rotatemode)
{
	float err = a - b;

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt, awt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// adjust weights based on rotatemode
		switch(rotatemode)
		{
		case ROTATEMODE_RGBA_RGBA: awt = 1.0; break;
		case ROTATEMODE_RGBA_AGBR: awt = rwt; break;
		case ROTATEMODE_RGBA_RABG: awt = gwt; break;
		case ROTATEMODE_RGBA_RGAB: awt = bwt; break;
		default: assert(0);
		}

		// weigh the components
		err *= awt;
	}

	return err * err;
}

float Utils::premult(float r, float a)
{
	// note that the args are really integers stored in floats
	int R = r, A = a;

	assert ((R==r) && (A==a));

	return float((R*A + RGBA_MAX/2)/RGBA_MAX);
}

static void premult4(Vec4& rgba)
{
	rgba.X() = Utils::premult(rgba.X(), rgba.W());
	rgba.Y() = Utils::premult(rgba.Y(), rgba.W());
	rgba.Z() = Utils::premult(rgba.Z(), rgba.W());
}

static void premult3(Vec3& rgb, float a)
{
	rgb.X() = Utils::premult(rgb.X(), a);
	rgb.Y() = Utils::premult(rgb.Y(), a);
	rgb.Z() = Utils::premult(rgb.Z(), a);
}

double Utils::metric4premult(const Vec4& a, const Vec4& b)
{
	Vec4 pma = a, pmb = b;

	premult4(pma);
	premult4(pmb);

	Vec4 err = pma - pmb;

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// weigh the components
		err.X() *= rwt;
		err.Y() *= gwt;
		err.Z() *= bwt;
	}

	return err * err;
}

double Utils::metric3premult_alphaout(const Vec3& rgb0, float a0, const Vec3& rgb1, float a1)
{
	Vec3 pma = rgb0, pmb = rgb1;

	premult3(pma, a0);
	premult3(pmb, a1);

	Vec3 err = pma - pmb;

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// weigh the components
		err.X() *= rwt;
		err.Y() *= gwt;
		err.Z() *= bwt;
	}

	return err * err;
}

double Utils::metric3premult_alphain(const Vec3& rgb0, const Vec3& rgb1, int rotatemode)
{
	Vec3 pma = rgb0, pmb = rgb1;

	switch(rotatemode)
	{
	case ROTATEMODE_RGBA_RGBA:
		// this function isn't supposed to be called for this rotatemode
		assert(0);
		break;
	case ROTATEMODE_RGBA_AGBR:
		pma.Y() = premult(pma.Y(), pma.X());
		pma.Z() = premult(pma.Z(), pma.X());
		pmb.Y() = premult(pmb.Y(), pmb.X());
		pmb.Z() = premult(pmb.Z(), pmb.X());
		break;
	case ROTATEMODE_RGBA_RABG:
		pma.X() = premult(pma.X(), pma.Y());
		pma.Z() = premult(pma.Z(), pma.Y());
		pmb.X() = premult(pmb.X(), pmb.Y());
		pmb.Z() = premult(pmb.Z(), pmb.Y());
		break;
	case ROTATEMODE_RGBA_RGAB:
		pma.X() = premult(pma.X(), pma.Z());
		pma.Y() = premult(pma.Y(), pma.Z());
		pmb.X() = premult(pmb.X(), pmb.Z());
		pmb.Y() = premult(pmb.Y(), pmb.Z());
		break;
	default: assert(0);
	}

	Vec3 err = pma - pmb;

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// weigh the components
		err.X() *= rwt;
		err.Y() *= gwt;
		err.Z() *= bwt;
	}

	return err * err;
}

double Utils::metric1premult(float rgb0, float a0, float rgb1, float a1, int rotatemode)
{
	float err = premult(rgb0, a0) - premult(rgb1, a1);

	// if nonuniform, select weights and weigh away
	if (AVPCL::flag_nonuniform || AVPCL::flag_nonuniform_ati)
	{
		double rwt, gwt, bwt, awt;
		if (AVPCL::flag_nonuniform)
		{
			rwt = 0.299; gwt = 0.587; bwt = 0.114;
		}
		else if (AVPCL::flag_nonuniform_ati)
		{
			rwt = 0.3086; gwt = 0.6094; bwt = 0.0820;
		}

		// adjust weights based on rotatemode
		switch(rotatemode)
		{
		case ROTATEMODE_RGBA_RGBA: awt = 1.0; break;
		case ROTATEMODE_RGBA_AGBR: awt = rwt; break;
		case ROTATEMODE_RGBA_RABG: awt = gwt; break;
		case ROTATEMODE_RGBA_RGAB: awt = bwt; break;
		default: assert(0);
		}

		// weigh the components
		err *= awt;
	}

	return err * err;
}
