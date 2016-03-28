/*
 * Sony CXD8530AQ/CXD8530BQ/CXD8530CQ/CXD8661R
 *
 * PSX CPU emulator for the MAME project written by smf
 * Thanks to Farfetch'd for information on the delay slot bug
 *
 * The PSX CPU is a custom r3000a with a built in
 * geometry transform engine, no mmu & no data cache.
 *
 * There is a stall circuit for load delays, but
 * it doesn't work if the load occurs in a branch
 * delay slot.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"

#define GTE_OP(op) ( (op >> 20) & 31 )
#define GTE_SF(op) ( (op >> 19) & 1 )
#define GTE_MX(op) ( (op >> 17) & 3 )
#define GTE_V(op) ( (op >> 15) & 3 )
#define GTE_CV(op) ( (op >> 13) & 3 )
#define GTE_CD(op) ( (op >> 11) & 3 )
#define GTE_LM(op) ( (op >> 10) & 1 )
#define GTE_CT(op) ( (op >> 6) & 15 )
#define GTE_FUNCT(op) ( op & 63 )

#define VXY0 ( psxRegs.CP2D.r[ 0 ] )
#define VX0  ( psxRegs.CP2D.r[ 0 ].w.l )
#define VY0  ( psxRegs.CP2D.r[ 0 ].w.h )
#define VZ0  ( psxRegs.CP2D.r[ 1 ].w.l )
#define VXY1 ( psxRegs.CP2D.r[ 2 ] )
#define VX1  ( psxRegs.CP2D.r[ 2 ].w.l )
#define VY1  ( psxRegs.CP2D.r[ 2 ].w.h )
#define VZ1  ( psxRegs.CP2D.r[ 3 ].w.l )
#define VXY2 ( psxRegs.CP2D.r[ 4 ] )
#define VX2  ( psxRegs.CP2D.r[ 4 ].w.l )
#define VY2  ( psxRegs.CP2D.r[ 4 ].w.h )
#define VZ2  ( psxRegs.CP2D.r[ 5 ].w.l )
#define RGB  ( psxRegs.CP2D.r[ 6 ] )
#define R    ( psxRegs.CP2D.r[ 6 ].b.l )
#define G    ( psxRegs.CP2D.r[ 6 ].b.h )
#define B    ( psxRegs.CP2D.r[ 6 ].b.h2 )
#define CODE ( psxRegs.CP2D.r[ 6 ].b.h3 )
#define OTZ  ( psxRegs.CP2D.r[ 7 ].w.l )
#define IR0  ( psxRegs.CP2D.r[ 8 ] )
#define IR1  ( psxRegs.CP2D.r[ 9 ] )
#define IR2  ( psxRegs.CP2D.r[ 10 ] )
#define IR3  ( psxRegs.CP2D.r[ 11 ] )
#define SXY0 ( psxRegs.CP2D.r[ 12 ] )
#define SX0  ( psxRegs.CP2D.r[ 12 ].w.l )
#define SY0  ( psxRegs.CP2D.r[ 12 ].w.h )
#define SXY1 ( psxRegs.CP2D.r[ 13 ] )
#define SX1  ( psxRegs.CP2D.r[ 13 ].w.l )
#define SY1  ( psxRegs.CP2D.r[ 13 ].w.h )
#define SXY2 ( psxRegs.CP2D.r[ 14 ] )
#define SX2  ( psxRegs.CP2D.r[ 14 ].w.l )
#define SY2  ( psxRegs.CP2D.r[ 14 ].w.h )
#define SXYP ( psxRegs.CP2D.r[ 15 ] )
#define SXP  ( psxRegs.CP2D.r[ 15 ].w.l )
#define SYP  ( psxRegs.CP2D.r[ 15 ].w.h )
#define SZ0  ( psxRegs.CP2D.r[ 16 ].w.l )
#define SZ1  ( psxRegs.CP2D.r[ 17 ].w.l )
#define SZ2  ( psxRegs.CP2D.r[ 18 ].w.l )
#define SZ3  ( psxRegs.CP2D.r[ 19 ].w.l )
#define RGB0 ( psxRegs.CP2D.r[ 20 ] )
#define R0_GTE   ( psxRegs.CP2D.r[ 20 ].b.l )
#define G0   ( psxRegs.CP2D.r[ 20 ].b.h )
#define B0   ( psxRegs.CP2D.r[ 20 ].b.h2 )
#define CD0  ( psxRegs.CP2D.r[ 20 ].b.h3 )
#define RGB1 ( psxRegs.CP2D.r[ 21 ] )
#define R1_GTE   ( psxRegs.CP2D.r[ 21 ].b.l )
#define G1   ( psxRegs.CP2D.r[ 21 ].b.h )
#define B1   ( psxRegs.CP2D.r[ 21 ].b.h2 )
#define CD1  ( psxRegs.CP2D.r[ 21 ].b.h3 )
#define RGB2 ( psxRegs.CP2D.r[ 22 ] )
#define R2_GTE   ( psxRegs.CP2D.r[ 22 ].b.l )
#define G2   ( psxRegs.CP2D.r[ 22 ].b.h )
#define B2   ( psxRegs.CP2D.r[ 22 ].b.h2 )
#define CD2  ( psxRegs.CP2D.r[ 22 ].b.h3 )
#define RES1 ( psxRegs.CP2D.r[ 23 ] )
#define MAC0 ( psxRegs.CP2D.r[ 24 ] )
#define MAC1 ( psxRegs.CP2D.r[ 25 ] )
#define MAC2 ( psxRegs.CP2D.r[ 26 ] )
#define MAC3 ( psxRegs.CP2D.r[ 27 ] )
#define IRGB ( psxRegs.CP2D.r[ 28 ] )
#define ORGB ( psxRegs.CP2D.r[ 29 ] )
#define LZCS ( psxRegs.CP2D.r[ 30 ] )
#define LZCR ( psxRegs.CP2D.r[ 31 ] )

#define D1  ( psxRegs.CP2C.r[ 0 ] )
#define R11_GTE ( psxRegs.CP2C.r[ 0 ].w.l )
#define R12_GTE ( psxRegs.CP2C.r[ 0 ].w.h )
#define R13_GTE ( psxRegs.CP2C.r[ 1 ].w.l )
#define R21 ( psxRegs.CP2C.r[ 1 ].w.h )
#define D2  ( psxRegs.CP2C.r[ 2 ] )
#define R22 ( psxRegs.CP2C.r[ 2 ].w.l )
#define R23 ( psxRegs.CP2C.r[ 2 ].w.h )
#define R31 ( psxRegs.CP2C.r[ 3 ].w.l )
#define R32 ( psxRegs.CP2C.r[ 3 ].w.h )
#define D3  ( psxRegs.CP2C.r[ 4 ] )
#define R33 ( psxRegs.CP2C.r[ 4 ].w.l )
#define TRX ( psxRegs.CP2C.r[ 5 ] )
#define TRY ( psxRegs.CP2C.r[ 6 ] )
#define TRZ ( psxRegs.CP2C.r[ 7 ] )
#define L11 ( psxRegs.CP2C.r[ 8 ].w.l )
#define L12 ( psxRegs.CP2C.r[ 8 ].w.h )
#define L13 ( psxRegs.CP2C.r[ 9 ].w.l )
#define L21 ( psxRegs.CP2C.r[ 9 ].w.h )
#define L22 ( psxRegs.CP2C.r[ 10 ].w.l )
#define L23 ( psxRegs.CP2C.r[ 10 ].w.h )
#define L31 ( psxRegs.CP2C.r[ 11 ].w.l )
#define L32 ( psxRegs.CP2C.r[ 11 ].w.h )
#define L33 ( psxRegs.CP2C.r[ 12 ].w.l )
#define RBK ( psxRegs.CP2C.r[ 13 ] )
#define GBK ( psxRegs.CP2C.r[ 14 ] )
#define BBK ( psxRegs.CP2C.r[ 15 ] )
#define LR1 ( psxRegs.CP2C.r[ 16 ].w.l )
#define LR2 ( psxRegs.CP2C.r[ 16 ].w.h )
#define LR3 ( psxRegs.CP2C.r[ 17 ].w.l )
#define LG1 ( psxRegs.CP2C.r[ 17 ].w.h )
#define LG2 ( psxRegs.CP2C.r[ 18 ].w.l )
#define LG3 ( psxRegs.CP2C.r[ 18 ].w.h )
#define LB1 ( psxRegs.CP2C.r[ 19 ].w.l )
#define LB2 ( psxRegs.CP2C.r[ 19 ].w.h )
#define LB3 ( psxRegs.CP2C.r[ 20 ].w.l )
#define RFC ( psxRegs.CP2C.r[ 21 ] )
#define GFC ( psxRegs.CP2C.r[ 22 ] )
#define BFC ( psxRegs.CP2C.r[ 23 ] )
#define OFX ( psxRegs.CP2C.r[ 24 ] )
#define OFY ( psxRegs.CP2C.r[ 25 ] )
#define H   ( psxRegs.CP2C.r[ 26 ].w.l )
#define DQA ( psxRegs.CP2C.r[ 27 ].w.l )
#define DQB ( psxRegs.CP2C.r[ 28 ] )
#define ZSF3 ( psxRegs.CP2C.r[ 29 ].w.l )
#define ZSF4 ( psxRegs.CP2C.r[ 30 ].w.l )
#define FLAG ( psxRegs.CP2C.r[ 31 ] )

INLINE s32 LIM( s32 n_value, s32 n_max, s32 n_min, u32 n_flag )
{
	if( n_value > n_max )
	{
		FLAG |= n_flag;
		return n_max;
	}
	else if( n_value < n_min )
	{
		FLAG |= n_flag;
		return n_min;
	}
	return n_value;
}

INLINE s64 BOUNDS( s64 n_value, s64 n_max, int n_maxflag, s64 n_min, int n_minflag )
{
	if( n_value > n_max )
	{
		FLAG |= 1 << n_maxflag;
	}
	else if( n_value < n_min )
	{
		FLAG |= 1 << n_minflag;
	}
	return n_value;
}

#define A1( a ) BOUNDS( ( a ), 0x7fffffff, 30, -(s64)0x80000000, 27 )
#define A2( a ) BOUNDS( ( a ), 0x7fffffff, 29, -(s64)0x80000000, 26 )
#define A3( a ) BOUNDS( ( a ), 0x7fffffff, 28, -(s64)0x80000000, 25 )
#define Lm_B1( a, l ) LIM( ( a ), 0x7fff, -0x8000 * !l, ( 1 << 31 ) | ( 1 << 24 ) )
#define Lm_B2( a, l ) LIM( ( a ), 0x7fff, -0x8000 * !l, ( 1 << 31 ) | ( 1 << 23 ) )
#define Lm_B3( a, l ) LIM( ( a ), 0x7fff, -0x8000 * !l, ( 1 << 31 ) | ( 1 << 22 ) )
#define Lm_C1( a ) LIM( ( a ), 0x00ff, 0x0000, ( 1 << 21 ) )
#define Lm_C2( a ) LIM( ( a ), 0x00ff, 0x0000, ( 1 << 20 ) )
#define Lm_C3( a ) LIM( ( a ), 0x00ff, 0x0000, ( 1 << 19 ) )
#define Lm_D( a ) LIM( ( a ), 0xffff, 0x0000, ( 1 << 31 ) | ( 1 << 18 ) )

INLINE u32 Lm_E( u32 n_z )
{
	if( n_z <= H / 2 )
	{
		n_z = H / 2;
		FLAG |= ( 1 << 31 ) | ( 1 << 17 );
	}
	if( n_z == 0 )
	{
		n_z = 1;
	}
	return n_z;
}

#define F( a ) BOUNDS( ( a ), 0x7fffffff, ( 1 << 31 ) | ( 1 << 16 ), -(s64)0x80000000, ( 1 << 31 ) | ( 1 << 15 ) )
#define Lm_G1( a ) LIM( ( a ), 0x3ff, -0x400, ( 1 << 31 ) | ( 1 << 14 ) )
#define Lm_G2( a ) LIM( ( a ), 0x3ff, -0x400, ( 1 << 31 ) | ( 1 << 13 ) )
#define Lm_H( a ) LIM( ( a ), 0xfff, 0x000, ( 1 << 12 ) )

void gteDoCOP2( u32 gteop )
{
	int n_sf;
	int n_v;
	int n_lm;
	int n_pass;
	u16 n_v1;
	u16 n_v2;
	u16 n_v3;
	const u16 **p_n_mx;
	const u32 **p_n_cv;
	static const u16 n_zm = 0;
	static const u32 n_zc = 0;
	static const u16 *p_n_vx[] = { &VX0, &VX1, &VX2 };
	static const u16 *p_n_vy[] = { &VY0, &VY1, &VY2 };
	static const u16 *p_n_vz[] = { &VZ0, &VZ1, &VZ2 };
	static const u16 *p_n_rm[] = { &R11_GTE, &R12_GTE, &R13_GTE, &R21, &R22, &R23, &R31, &R32, &R33 };
	static const u16 *p_n_lm[] = { &L11, &L12, &L13, &L21, &L22, &L23, &L31, &L32, &L33 };
	static const u16 *p_n_cm[] = { &LR1, &LR2, &LR3, &LG1, &LG2, &LG3, &LB1, &LB2, &LB3 };
	static const u16 *p_n_zm[] = { &n_zm, &n_zm, &n_zm, &n_zm, &n_zm, &n_zm, &n_zm, &n_zm, &n_zm };
	static const u16 **p_p_n_mx[] = { p_n_rm, p_n_lm, p_n_cm, p_n_zm };
	static const u32 *p_n_tr[] = { &TRX, &TRY, &TRZ };
	static const u32 *p_n_bk[] = { &RBK, &GBK, &BBK };
	static const u32 *p_n_fc[] = { &RFC, &GFC, &BFC };
	static const u32 *p_n_zc[] = { &n_zc, &n_zc, &n_zc };
	static const u32 **p_p_n_cv[] = { p_n_tr, p_n_bk, p_n_fc, p_n_zc };

	switch( GTE_FUNCT( gteop ) )
	{
	case 0x01:
		//if( gteop == 0x0180001 )
		{
			//GTELOG( "RTPS" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s32)TRX << 12 ) + ( (s16)R11_GTE * (s16)VX0 ) + ( (s16)R12_GTE * (s16)VY0 ) + ( (s16)R13_GTE * (s16)VZ0 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)(s32)TRY << 12 ) + ( (s16)R21 * (s16)VX0 ) + ( (s16)R22 * (s16)VY0 ) + ( (s16)R23 * (s16)VZ0 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)(s32)TRZ << 12 ) + ( (s16)R31 * (s16)VX0 ) + ( (s16)R32 * (s16)VY0 ) + ( (s16)R33 * (s16)VZ0 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 0 );
			IR2 = Lm_B2( (s32)MAC2, 0 );
			IR3 = Lm_B3( (s32)MAC3, 0 );
			SZ0 = SZ1;
			SZ1 = SZ2;
			SZ2 = SZ3;
			SZ3 = Lm_D( (s32)MAC3 );
			SXY0 = SXY1;
			SXY1 = SXY2;
			SX2 = Lm_G1( F( (s64)(s32)OFX + ( (s64)(s16)IR1 * ( ( (u32)H << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 );
			SY2 = Lm_G2( F( (s64)(s32)OFY + ( (s64)(s16)IR2 * ( ( (u32)H << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 );
			MAC0 = F( (s64)(s32)DQB + ( (s64)(s16)DQA * ( ( (u32)H << 16 ) / Lm_E( SZ3 ) ) ) );
			IR0 = Lm_H( (s32)MAC0 >> 12 );
			return;
		}
		break;
	case 0x06:
		//if( gteop == 0x0400006 ||
		//	gteop == 0x1400006 ||
		//	gteop == 0x0155cc6 )
		{
			//GTELOG( "NCLIP" );
			FLAG = 0;

			MAC0 = F( ( (s64)(s16)SX0 * (s16)SY1 ) + ( (s16)SX1 * (s16)SY2 ) + ( (s16)SX2 * (s16)SY0 ) - ( (s16)SX0 * (s16)SY2 ) - ( (s16)SX1 * (s16)SY0 ) - ( (s16)SX2 * (s16)SY1 ) );
			return;
		}
		break;
	case 0x0c:
		//if( GTE_OP( gteop ) == 0x17 )
		{
			//GTELOG( "OP" );
			n_sf = 12 * GTE_SF( gteop );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s32)D2 * (s16)IR3 ) - ( (s64)(s32)D3 * (s16)IR2 ) ) >> n_sf );
			MAC2 = A2( ( ( (s64)(s32)D3 * (s16)IR1 ) - ( (s64)(s32)D1 * (s16)IR3 ) ) >> n_sf );
			MAC3 = A3( ( ( (s64)(s32)D1 * (s16)IR2 ) - ( (s64)(s32)D2 * (s16)IR1 ) ) >> n_sf );
			IR1 = Lm_B1( (s32)MAC1, 0 );
			IR2 = Lm_B2( (s32)MAC2, 0 );
			IR3 = Lm_B3( (s32)MAC3, 0 );
			return;
		}
		break;
	case 0x10:
		//if( gteop == 0x0780010 )
		{
			//GTELOG( "DPCS" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)R << 16 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)RFC - ( R << 4 ), 0 ) ) ) ) >> 12 );
			MAC2 = A2( ( ( (s64)G << 16 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)GFC - ( G << 4 ), 0 ) ) ) ) >> 12 );
			MAC3 = A3( ( ( (s64)B << 16 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)BFC - ( B << 4 ), 0 ) ) ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 0 );
			IR2 = Lm_B2( (s32)MAC2, 0 );
			IR3 = Lm_B3( (s32)MAC3, 0 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x11:
		//if( gteop == 0x0980011 )
		{
			//GTELOG( "INTPL" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s16)IR1 << 12 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)RFC - (s16)IR1, 0 ) ) ) ) >> 12 );
			MAC2 = A2( ( ( (s64)(s16)IR2 << 12 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)GFC - (s16)IR2, 0 ) ) ) ) >> 12 );
			MAC3 = A3( ( ( (s64)(s16)IR3 << 12 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)BFC - (s16)IR3, 0 ) ) ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 0 );
			IR2 = Lm_B2( (s32)MAC2, 0 );
			IR3 = Lm_B3( (s32)MAC3, 0 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 );
			return;
		}
		break;
	case 0x12:
		//if( GTE_OP( gteop ) == 0x04 )
		{
			//GTELOG( "MVMVA" );
			n_sf = 12 * GTE_SF( gteop );
			p_n_mx = p_p_n_mx[ GTE_MX( gteop ) ];
			n_v = GTE_V( gteop );
			if( n_v < 3 )
			{
				n_v1 = *p_n_vx[ n_v ];
				n_v2 = *p_n_vy[ n_v ];
				n_v3 = *p_n_vz[ n_v ];
			}
			else
			{
				n_v1 = IR1;
				n_v2 = IR2;
				n_v3 = IR3;
			}
			p_n_cv = p_p_n_cv[ GTE_CV( gteop ) ];
			n_lm = GTE_LM( gteop );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s32)*p_n_cv[ 0 ] << 12 ) + ( (s16)*p_n_mx[ 0 ] * (s16)n_v1 ) + ( (s16)*p_n_mx[ 1 ] * (s16)n_v2 ) + ( (s16)*p_n_mx[ 2 ] * (s16)n_v3 ) ) >> n_sf );
			MAC2 = A2( ( ( (s64)(s32)*p_n_cv[ 1 ] << 12 ) + ( (s16)*p_n_mx[ 3 ] * (s16)n_v1 ) + ( (s16)*p_n_mx[ 4 ] * (s16)n_v2 ) + ( (s16)*p_n_mx[ 5 ] * (s16)n_v3 ) ) >> n_sf );
			MAC3 = A3( ( ( (s64)(s32)*p_n_cv[ 2 ] << 12 ) + ( (s16)*p_n_mx[ 6 ] * (s16)n_v1 ) + ( (s16)*p_n_mx[ 7 ] * (s16)n_v2 ) + ( (s16)*p_n_mx[ 8 ] * (s16)n_v3 ) ) >> n_sf );

			IR1 = Lm_B1( (s32)MAC1, n_lm );
			IR2 = Lm_B2( (s32)MAC2, n_lm );
			IR3 = Lm_B3( (s32)MAC3, n_lm );
			return;
		}
		break;
	case 0x13:
		//if( gteop == 0x0e80413 )
		{
			//GTELOG( "NCDS" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s16)L11 * (s16)VX0 ) + ( (s16)L12 * (s16)VY0 ) + ( (s16)L13 * (s16)VZ0 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)(s16)L21 * (s16)VX0 ) + ( (s16)L22 * (s16)VY0 ) + ( (s16)L23 * (s16)VZ0 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)(s16)L31 * (s16)VX0 ) + ( (s16)L32 * (s16)VY0 ) + ( (s16)L33 * (s16)VZ0 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			MAC1 = A1( ( ( ( (s64)R << 4 ) * (s16)IR1 ) + ( (s16)IR0 * Lm_B1( (s32)RFC - ( ( R * (s16)IR1 ) >> 8 ), 0 ) ) ) >> 12 );
			MAC2 = A2( ( ( ( (s64)G << 4 ) * (s16)IR2 ) + ( (s16)IR0 * Lm_B2( (s32)GFC - ( ( G * (s16)IR2 ) >> 8 ), 0 ) ) ) >> 12 );
			MAC3 = A3( ( ( ( (s64)B << 4 ) * (s16)IR3 ) + ( (s16)IR0 * Lm_B3( (s32)BFC - ( ( B * (s16)IR3 ) >> 8 ), 0 ) ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x14:
		//if( gteop == 0x1280414 )
		{
			//GTELOG( "CDP" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
			IR1 = Lm_B1( MAC1, 1 );
			IR2 = Lm_B2( MAC2, 1 );
			IR3 = Lm_B3( MAC3, 1 );
			MAC1 = A1( ( ( ( (s64)R << 4 ) * (s16)IR1 ) + ( (s16)IR0 * Lm_B1( (s32)RFC - ( ( R * (s16)IR1 ) >> 8 ), 0 ) ) ) >> 12 );
			MAC2 = A2( ( ( ( (s64)G << 4 ) * (s16)IR2 ) + ( (s16)IR0 * Lm_B2( (s32)GFC - ( ( G * (s16)IR2 ) >> 8 ), 0 ) ) ) >> 12 );
			MAC3 = A3( ( ( ( (s64)B << 4 ) * (s16)IR3 ) + ( (s16)IR0 * Lm_B3( (s32)BFC - ( ( B * (s16)IR3 ) >> 8 ), 0 ) ) ) >> 12 );
			IR1 = Lm_B1( MAC1, 1 );
			IR2 = Lm_B2( MAC2, 1 );
			IR3 = Lm_B3( MAC3, 1 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x16:
		//if( gteop == 0x0f80416 )
		{
			//GTELOG( "NCDT" );
			FLAG = 0;

			for( n_v = 0; n_v < 3; n_v++ )
			{
				MAC1 = A1( ( ( (s64)(s16)L11 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L12 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L13 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC2 = A2( ( ( (s64)(s16)L21 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L22 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L23 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC3 = A3( ( ( (s64)(s16)L31 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L32 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L33 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
				MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
				MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				MAC1 = A1( ( ( ( (s64)R << 4 ) * (s16)IR1 ) + ( (s16)IR0 * Lm_B1( (s32)RFC - ( ( R * (s16)IR1 ) >> 8 ), 0 ) ) ) >> 12 );
				MAC2 = A2( ( ( ( (s64)G << 4 ) * (s16)IR2 ) + ( (s16)IR0 * Lm_B2( (s32)GFC - ( ( G * (s16)IR2 ) >> 8 ), 0 ) ) ) >> 12 );
				MAC3 = A3( ( ( ( (s64)B << 4 ) * (s16)IR3 ) + ( (s16)IR0 * Lm_B3( (s32)BFC - ( ( B * (s16)IR3 ) >> 8 ), 0 ) ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				CD0 = CD1;
				CD1 = CD2;
				CD2 = CODE;
				R0_GTE = R1_GTE;
				R1_GTE = R2_GTE;
				R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
				G0 = G1;
				G1 = G2;
				G2 = Lm_C2( (s32)MAC2 >> 4 );
				B0 = B1;
				B1 = B2;
				B2 = Lm_C3( (s32)MAC3 >> 4 );
			}
			return;
		}
		break;
	case 0x1b:
		//if( gteop == 0x108041b )
		{
			//GTELOG( "NCCS" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s16)L11 * (s16)VX0 ) + ( (s16)L12 * (s16)VY0 ) + ( (s16)L13 * (s16)VZ0 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)(s16)L21 * (s16)VX0 ) + ( (s16)L22 * (s16)VY0 ) + ( (s16)L23 * (s16)VZ0 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)(s16)L31 * (s16)VX0 ) + ( (s16)L32 * (s16)VY0 ) + ( (s16)L33 * (s16)VZ0 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			MAC1 = A1( ( (s64)R * (s16)IR1 ) >> 8 );
			MAC2 = A2( ( (s64)G * (s16)IR2 ) >> 8 );
			MAC3 = A3( ( (s64)B * (s16)IR3 ) >> 8 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x1c:
		//if( gteop == 0x138041c )
		{
			//GTELOG( "CC" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
			IR1 = Lm_B1( MAC1, 1 );
			IR2 = Lm_B2( MAC2, 1 );
			IR3 = Lm_B3( MAC3, 1 );
			MAC1 = A1( ( (s64)R * (s16)IR1 ) >> 8 );
			MAC2 = A2( ( (s64)G * (s16)IR2 ) >> 8 );
			MAC3 = A3( ( (s64)B * (s16)IR3 ) >> 8 );
			IR1 = Lm_B1( MAC1, 1 );
			IR2 = Lm_B2( MAC2, 1 );
			IR3 = Lm_B3( MAC3, 1 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x1e:
		//if( gteop == 0x0c8041e )
		{
			//GTELOG( "NCS" );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s16)L11 * (s16)VX0 ) + ( (s16)L12 * (s16)VY0 ) + ( (s16)L13 * (s16)VZ0 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)(s16)L21 * (s16)VX0 ) + ( (s16)L22 * (s16)VY0 ) + ( (s16)L23 * (s16)VZ0 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)(s16)L31 * (s16)VX0 ) + ( (s16)L32 * (s16)VY0 ) + ( (s16)L33 * (s16)VZ0 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
			MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
			MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
			IR1 = Lm_B1( (s32)MAC1, 1 );
			IR2 = Lm_B2( (s32)MAC2, 1 );
			IR3 = Lm_B3( (s32)MAC3, 1 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x20:
		//if( gteop == 0x0d80420 )
		{
			//GTELOG( "NCT" );
			FLAG = 0;

			for( n_v = 0; n_v < 3; n_v++ )
			{
				MAC1 = A1( ( ( (s64)(s16)L11 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L12 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L13 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC2 = A2( ( ( (s64)(s16)L21 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L22 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L23 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC3 = A3( ( ( (s64)(s16)L31 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L32 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L33 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
				MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
				MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				CD0 = CD1;
				CD1 = CD2;
				CD2 = CODE;
				R0_GTE = R1_GTE;
				R1_GTE = R2_GTE;
				R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
				G0 = G1;
				G1 = G2;
				G2 = Lm_C2( (s32)MAC2 >> 4 );
				B0 = B1;
				B1 = B2;
				B2 = Lm_C3( (s32)MAC3 >> 4 );
			}
			return;
		}
		break;
	case 0x28:
		//if( GTE_OP( gteop ) == 0x0a && GTE_LM( gteop ) == 1 )
		{
			//GTELOG( "SQR" );
			n_sf = 12 * GTE_SF( gteop );
			FLAG = 0;

			MAC1 = A1( ( (s64)(s16)IR1 * (s16)IR1 ) >> n_sf );
			MAC2 = A2( ( (s64)(s16)IR2 * (s16)IR2 ) >> n_sf );
			MAC3 = A3( ( (s64)(s16)IR3 * (s16)IR3 ) >> n_sf );
			IR1 = Lm_B1( MAC1, 1 );
			IR2 = Lm_B2( MAC2, 1 );
			IR3 = Lm_B3( MAC3, 1 );
			return;
		}
		break;
	/* DCPL 0x29 */
	case 0x2a:
		//if( gteop == 0x0f8002a )
		{
			//GTELOG( "DPCT" );
			FLAG = 0;

			for( n_pass = 0; n_pass < 3; n_pass++ )
			{
				MAC1 = A1( ( ( (s64)R0_GTE << 16 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)RFC - ( R0_GTE << 4 ), 0 ) ) ) ) >> 12 );
				MAC2 = A2( ( ( (s64)G0 << 16 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)GFC - ( G0 << 4 ), 0 ) ) ) ) >> 12 );
				MAC3 = A3( ( ( (s64)B0 << 16 ) + ( (s64)(s16)IR0 * ( Lm_B1( (s32)BFC - ( B0 << 4 ), 0 ) ) ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 0 );
				IR2 = Lm_B2( (s32)MAC2, 0 );
				IR3 = Lm_B3( (s32)MAC3, 0 );
				CD0 = CD1;
				CD1 = CD2;
				CD2 = CODE;
				R0_GTE = R1_GTE;
				R1_GTE = R2_GTE;
				R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
				G0 = G1;
				G1 = G2;
				G2 = Lm_C2( (s32)MAC2 >> 4 );
				B0 = B1;
				B1 = B2;
				B2 = Lm_C3( (s32)MAC3 >> 4 );
			}
			return;
		}
		break;
	case 0x2d:
		//if( gteop == 0x158002d )
		{
			//GTELOG( "AVSZ3" );
			FLAG = 0;

			MAC0 = F( ( (s64)(s16)ZSF3 * SZ1 ) + ( (s16)ZSF3 * SZ2 ) + ( (s16)ZSF3 * SZ3 ) );
			OTZ = Lm_D( (s32)MAC0 >> 12 );
			return;
		}
		break;
	case 0x2e:
		//if( gteop == 0x168002e )
		{
			//GTELOG( "AVSZ4" );
			FLAG = 0;

			MAC0 = F( ( (s64)(s16)ZSF4 * SZ0 ) + ( (s16)ZSF4 * SZ1 ) + ( (s16)ZSF4 * SZ2 ) + ( (s16)ZSF4 * SZ3 ) );
			OTZ = Lm_D( (s32)MAC0 >> 12 );
			return;
		}
		break;
	case 0x30:
		//if( gteop == 0x0280030 )
		{
			//GTELOG( "RTPT" );
			FLAG = 0;

			for( n_v = 0; n_v < 3; n_v++ )
			{
				MAC1 = A1( ( ( (s64)(s32)TRX << 12 ) + ( (s16)R11_GTE * (s16)*p_n_vx[ n_v ] ) + ( (s16)R12_GTE * (s16)*p_n_vy[ n_v ] ) + ( (s16)R13_GTE * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC2 = A2( ( ( (s64)(s32)TRY << 12 ) + ( (s16)R21 * (s16)*p_n_vx[ n_v ] ) + ( (s16)R22 * (s16)*p_n_vy[ n_v ] ) + ( (s16)R23 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC3 = A3( ( ( (s64)(s32)TRZ << 12 ) + ( (s16)R31 * (s16)*p_n_vx[ n_v ] ) + ( (s16)R32 * (s16)*p_n_vy[ n_v ] ) + ( (s16)R33 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 0 );
				IR2 = Lm_B2( (s32)MAC2, 0 );
				IR3 = Lm_B3( (s32)MAC3, 0 );
				SZ0 = SZ1;
				SZ1 = SZ2;
				SZ2 = SZ3;
				SZ3 = Lm_D( (s32)MAC3 );
				SXY0 = SXY1;
				SXY1 = SXY2;
				SX2 = Lm_G1( F( ( (s64)(s32)OFX + ( (s64)(s16)IR1 * ( ( (u32)H << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 ) );
				SY2 = Lm_G2( F( ( (s64)(s32)OFY + ( (s64)(s16)IR2 * ( ( (u32)H << 16 ) / Lm_E( SZ3 ) ) ) ) >> 16 ) );
				MAC0 = F( (s64)(s32)DQB + ( (s64)(s16)DQA * ( ( (u32)H << 16 ) / Lm_E( SZ3 ) ) ) );
				IR0 = Lm_H( (s32)MAC0 >> 12 );
			}
			return;
		}
		break;
	case 0x3d:
		//if( GTE_OP( gteop ) == 0x09 ||
		//	GTE_OP( gteop ) == 0x19 )
		{
			//GTELOG( "GPF" );
			n_sf = 12 * GTE_SF( gteop );
			FLAG = 0;

			MAC1 = A1( ( (s64)(s16)IR0 * (s16)IR1 ) >> n_sf );
			MAC2 = A2( ( (s64)(s16)IR0 * (s16)IR2 ) >> n_sf );
			MAC3 = A3( ( (s64)(s16)IR0 * (s16)IR3 ) >> n_sf );
			IR1 = Lm_B1( (s32)MAC1, 0 );
			IR2 = Lm_B2( (s32)MAC2, 0 );
			IR3 = Lm_B3( (s32)MAC3, 0 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x3e:
		//if( GTE_OP( gteop ) == 0x1a )
		{
			//GTELOG( "GPL" );
			n_sf = 12 * GTE_SF( gteop );
			FLAG = 0;

			MAC1 = A1( ( ( (s64)(s32)MAC1 << n_sf ) + ( (s16)IR0 * (s16)IR1 ) ) >> n_sf );
			MAC2 = A2( ( ( (s64)(s32)MAC2 << n_sf ) + ( (s16)IR0 * (s16)IR2 ) ) >> n_sf );
			MAC3 = A3( ( ( (s64)(s32)MAC3 << n_sf ) + ( (s16)IR0 * (s16)IR3 ) ) >> n_sf );
			IR1 = Lm_B1( (s32)MAC1, 0 );
			IR2 = Lm_B2( (s32)MAC2, 0 );
			IR3 = Lm_B3( (s32)MAC3, 0 );
			CD0 = CD1;
			CD1 = CD2;
			CD2 = CODE;
			R0_GTE = R1_GTE;
			R1_GTE = R2_GTE;
			R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
			G0 = G1;
			G1 = G2;
			G2 = Lm_C2( (s32)MAC2 >> 4 );
			B0 = B1;
			B1 = B2;
			B2 = Lm_C3( (s32)MAC3 >> 4 );
			return;
		}
		break;
	case 0x3f:
		//if( gteop == 0x108043f ||
		//	gteop == 0x118043f )
		{
			//GTELOG( "NCCT" );
			FLAG = 0;

			for( n_v = 0; n_v < 3; n_v++ )
			{
				MAC1 = A1( ( ( (s64)(s16)L11 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L12 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L13 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC2 = A2( ( ( (s64)(s16)L21 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L22 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L23 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				MAC3 = A3( ( ( (s64)(s16)L31 * (s16)*p_n_vx[ n_v ] ) + ( (s16)L32 * (s16)*p_n_vy[ n_v ] ) + ( (s16)L33 * (s16)*p_n_vz[ n_v ] ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				MAC1 = A1( ( ( (s64)RBK << 12 ) + ( (s16)LR1 * (s16)IR1 ) + ( (s16)LR2 * (s16)IR2 ) + ( (s16)LR3 * (s16)IR3 ) ) >> 12 );
				MAC2 = A2( ( ( (s64)GBK << 12 ) + ( (s16)LG1 * (s16)IR1 ) + ( (s16)LG2 * (s16)IR2 ) + ( (s16)LG3 * (s16)IR3 ) ) >> 12 );
				MAC3 = A3( ( ( (s64)BBK << 12 ) + ( (s16)LB1 * (s16)IR1 ) + ( (s16)LB2 * (s16)IR2 ) + ( (s16)LB3 * (s16)IR3 ) ) >> 12 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				MAC1 = A1( ( (s64)R * (s16)IR1 ) >> 8 );
				MAC2 = A2( ( (s64)G * (s16)IR2 ) >> 8 );
				MAC3 = A3( ( (s64)B * (s16)IR3 ) >> 8 );
				IR1 = Lm_B1( (s32)MAC1, 1 );
				IR2 = Lm_B2( (s32)MAC2, 1 );
				IR3 = Lm_B3( (s32)MAC3, 1 );
				CD0 = CD1;
				CD1 = CD2;
				CD2 = CODE;
				R0_GTE = R1_GTE;
				R1_GTE = R2_GTE;
				R2_GTE = Lm_C1( (s32)MAC1 >> 4 );
				G0 = G1;
				G1 = G2;
				G2 = Lm_C2( (s32)MAC2 >> 4 );
				B0 = B1;
				B1 = B2;
				B2 = Lm_C3( (s32)MAC3 >> 4 );
			}
			return;
		}
		break;
	}
}

