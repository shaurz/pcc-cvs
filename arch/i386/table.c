/*	$Id$	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


# include "pass2.h"

# define TLL TLONGLONG|TULONGLONG
# define ANYSIGNED TINT|TLONG|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TULONG|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TUWORD TUNSIGNED|TULONG
# define TSWORD TINT|TLONG
# define TWORD TUWORD|TSWORD

struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/*
 * A bunch conversions of integral<->integral types
 */

/* convert pointers to int. */
{ SCONV,	INTAREG,
	STAREG,	TPOINT|TWORD,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* convert (u)longlong to (u)longlong. */
{ SCONV,	INTAREG,
	SAREG|STAREG,	TLL,
	SAREG|STAREG,	TLL,
		0,	RLEFT,
		"", },

/* convert (u)char to (u)char. */
{ SCONV,	INTAREG,
	SAREG|STAREG,	TCHAR|TUCHAR,
	SAREG|STAREG,	TCHAR|TUCHAR,
		0,	RLEFT,
		"", },


/* convert int to short/char. This is done when register is loaded */
{ SCONV,	INTAREG,
	STAREG,	TWORD,
	SANY,	TSHORT|TUSHORT|TCHAR|TUCHAR|TWORD,
		0,	RLEFT,
		"", },

/* convert short to char. This is done when register is loaded */
{ SCONV,	INTAREG,
	SAREG|STAREG,	TSHORT|TUSHORT,
	STAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"", },

/* convert signed char to int. */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TCHAR,
	SAREG|STAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movsbl ZL,A1\n", },

/* convert char to short. */
{ SCONV,	INTAREG,
	SAREG|STAREG,	TCHAR,
	SAREG|STAREG,	TSHORT,
		NASL|NAREG,	RESC1,
		"	movzbw ZL,Z1\n", },

/* convert char to unsigned short. */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TCHAR,
	SAREG|STAREG,	TUSHORT,
		NASL|NAREG,	RESC1,
		"	movsbw ZL,Z1\n", },

/* convert unsigned char to (u)int. */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TUCHAR,
	SAREG|STAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movzbl ZL,A1\n", },

/* convert short to (u)int. */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TSHORT,
	SAREG|STAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movswl ZL,A1\n", },

/* convert float/double to long long XXX - may this fail? */
{ SCONV,	INTAREG,
	SBREG|STBREG,	TDOUBLE|TFLOAT,
	SNAME|SOREG,	TLL,
		NAREG,	RESC1,
		"	subl $8,%esp\n	fistpq (%esp)\n"
		"	popl %eax\n	popl %edx\n", },

/* convert float/double to (u) int/short/char. XXX should use NTEMP here */
{ SCONV,	INTAREG,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
	SAREG|STAREG,	TWORD|TSHORT|TUSHORT|TCHAR|TUCHAR,
		NAREG,	RESC1,
		"	subl $4,%esp\n	fistpl (%esp)\n	popl A1\n", },

/* convert double <-> float. nothing to do here */
{ SCONV,	INTBREG,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"", },

/* convert unsigned short to (u)int. */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TUSHORT,
	SAREG|STAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movzwl ZL,A1\n", },

/* convert unsigned char to (u)short. */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TUCHAR,
	SAREG|STAREG,	TSHORT|TUSHORT,
		NASL|NAREG,	RESC1,
		"	movzbw ZL,Z1\n", },

/* convert unsigned short to (u)long long */
{ SCONV,	INTAREG,
	SAREG|STAREG,	TUSHORT,
	SAREG|STAREG,	TLL,
		NSPECIAL|NAREG|NASL,	RESC1,
		"	movzwl ZL,AL\n	cltd\n", },

/* convert unsigned char to (u)long long */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TUCHAR,
	SAREG|STAREG,			TLL,
		NAREG|NASL,	RESC1,
		"	movzbl ZL,A1\n	xorl U1,U1\n", },

/* convert char to (u)long long */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TCHAR,
	SAREG|STAREG,	TLL,
		NSPECIAL|NAREG|NASL,	RESC1,
		"	movsbl ZL,%eax\n	cltd\n", },

/* convert short to (u)long long */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TSHORT,
	SAREG|STAREG,	TLL,
		NSPECIAL|NAREG|NASL,	RESC1,
		"	movswl ZL,%eax\n	cltd\n", },

/* convert int to long long */
{ SCONV,	INTAREG,
	SAREG|STAREG,	TWORD|TPOINT,
	SAREG|STAREG,	TLONGLONG,
		NSPECIAL|NAREG|NASL,	RESC1,
		"	cltd\n", },

/* convert long long to int */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TLL,
	SAREG|STAREG,	TWORD|TPOINT,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

/* convert (u)long long to (u)short */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TLL,
	SAREG|STAREG,	TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	movw ZL,Z1\n", },

/* convert (u)long long to (u)char */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TLL,
	SAREG|STAREG,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"	movb ZL,Z1\n", },

/* convert int to unsigned long long */
{ SCONV,	INTAREG,
	SAREG|STAREG|SOREG|SNAME,	TWORD|TPOINT,
	SAREG|STAREG,	TULONGLONG,
		NASL|NAREG,	RESC1,
		"	movl AL,A1\n	xorl U1,U1\n", },

/* convert long long (in memory) to double */
{ SCONV,	INTBREG,
	SOREG|SNAME,	TLL,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NBREG,	RESC1,
		"	fildq AL\n", },

/* convert int (in memory) to double */
{ SCONV,	INTBREG,
	SOREG|SNAME,	TWORD,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NBREG,	RESC1,
		"	fildl AL\n", },

/* convert int (in register) to double */
{ SCONV,	INTBREG,
	SAREG|STAREG,	TWORD,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NBREG,	RESC1,
		"	pushl AL\n	fildl (%esp)\n	addl $4,%esp\n", },

/* convert char (in register) to double XXX - use NTEMP */
{ SCONV,	INTBREG,
	SAREG|STAREG|SOREG|SNAME,	TCHAR,
	SBREG|STBREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NBREG|NTEMP,	RESC2,
		"	movsbl ZL,A1\n	pushl A1\n"
		"	fildl (%esp)\n	addl $4,%esp\n", },

/* convert (u)char (in register) to double XXX - use NTEMP */
{ SCONV,	INTBREG,
	SAREG|STAREG|SOREG|SNAME,	TUCHAR,
	SBREG|STBREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NBREG|NTEMP,	RESC2,
		"	movzbl ZL,A1\n	pushl A1\n"
		"	fildl (%esp)\n	addl $4,%esp\n", },

/* convert short (in memory) to float/double */
{ SCONV,	INTBREG,
	SOREG|SNAME,	TSHORT,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NBREG,	RESC1,
		"	fild AL\n", },

/* convert short (in register) to float/double */
{ SCONV,	INTBREG,
	SAREG|STAREG,	TSHORT,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NBREG,	RESC1,
		"	pushw ZL\n	fild (%esp)\n	addl $2,%esp\n", },

/* convert unsigned short to double XXX - use NTEMP */
{ SCONV,	INTBREG,
	SAREG|STAREG|SOREG|SNAME,	TUSHORT,
	SBREG|STBREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NBREG|NTEMP|NASL,	RESC2,
		"	movzwl ZL,A1\n	pushl A1\n"
		"	fildl (%esp)\n	addl $4,%esp\n", },

/*
 * Store constant initializers.
 */
{ INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TWORD|TPOINT,
		0,	RNOP,
		"	.long CL\n", },

{ INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TLL,
		0,	RNOP,
		"	.long ZI\n	.long CL\n", },

/*
 * Subroutine calls.
 */

{ UCALL,	INTAREG,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INTAREG,
	SAREG|STAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

/* struct return */
{ USTCALL,	INTAREG,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ USTCALL,	INTAREG,
	SNAME,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

/*
 * The next rules handle all binop-style operators.
 */
/* Special treatment for long long */
{ PLUS,		INAREG|FOREFF,
	STAREG,		TLL,
	SAREG|SNAME|SOREG,	TLL,
		0,	RLEFT,
		"	addl AR,AL\n	adcl UR,UL\n", },

/* Special treatment for long long  XXX - fix commutative check */
{ PLUS,		INAREG|FOREFF,
	SAREG|STAREG|SNAME|SOREG,	TLL,
	SAREG|STAREG,			TLL,
		0,	RRIGHT,
		"	addl AL,AR\n	adcl UL,UR\n", },

{ PLUS,		INBREG,
	STBREG,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	faddl AR\n", },

{ PLUS,		INBREG,
	STBREG,		TLDOUBLE|TDOUBLE|TFLOAT,
	STBREG|SBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	faddp %st,%st(1)\n", },

/* address as register offset, positive */
{ PLUS,		INTAREG,
	SAREG|STAREG,	TWORD|TPOINT,
	SCON,	TANY,
		NAREG|NASL,	RESC1,
		"	leal CR(AL),A1\n", },

/* address as register offset, negative */
{ MINUS,	INTAREG,
	SAREG|STAREG,	TWORD|TPOINT,
	SCON,	TANY,
		NAREG|NASL,	RESC1,
		"	leal -CR(AL),A1\n", },

{ MINUS,		INAREG|FOREFF,
	STAREG,		TLL,
	SAREG|SNAME|SOREG,	TLL,
		0,	RLEFT,
		"	subl AR,AL\n	sbbl UR,UL\n", },

{ MINUS,		INBREG,
	STBREG,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	fsubl AR\n", },

{ MINUS,		INBREG,
	STBREG,		TLDOUBLE|TDOUBLE|TFLOAT,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fsubZHp %st,%st(1)\n", },

/* Simple r/m->reg ops */
{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG,			TWORD|TPOINT,
	SAREG|STAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RLEFT,
		"	Ol AR,AL\n", },

{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG,		TSHORT|TUSHORT,
	SAREG|STAREG|SNAME|SOREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	Ow ZR,ZL\n", },

{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG,		TCHAR|TUCHAR,
	SAREG|STAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	Ob ZR,ZL\n", },

{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG,		TWORD|TPOINT,
	SCON,	TWORD|TPOINT,
		0,	RLEFT,
		"	Ol AR,AL\n", },

{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,	TANY,
		0,	RLEFT,
		"	Ow ZR,ZL\n", },

{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,	TANY,
		0,	RLEFT,
		"	Ob ZR,ZL\n", },

{ OPSIMP,	INAREG|FOREFF,
	SAREG|STAREG,	TLL,
	SAREG|STAREG|SNAME|SOREG,	TLL,
		0,	RLEFT,
		"	orl AR,AL\n	orl UR,UL\n", },


/*
 * The next rules handle all shift operators.
 */
{ LS,	INTAREG|INAREG,
	STAREG|SAREG,	TWORD,
	STAREG,	TWORD|TCHAR|TUCHAR|TSHORT|TUSHORT,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	sall ZA,AL\n", },

{ LS,	FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TWORD,
	STAREG,	TWORD|TCHAR|TUCHAR|TSHORT|TUSHORT,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	sall ZA,AL\n", },

{ LS,	FOREFF,
	STAREG|SAREG,	TWORD,
	SCON,	TANY,
		0,	RLEFT,
		"	sall ZA,AL\n", },

{ LS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,	TANY,
		0,	RLEFT,
		"	shlw ZA,ZL\n", },

{ LS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	STAREG|SAREG,	TANY,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	shlw ZA,ZL\n", },

{ LS,	FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,	TANY,
		0,	RLEFT,
		"	salb ZA,ZL\n", },

{ LS,	FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	STAREG|SAREG,	TANY,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	salb ZA,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TSWORD,
	STAREG|SCON,	TWORD|TCHAR|TUCHAR|TSHORT|TUSHORT,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	sarl ZA,AL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TUWORD,
	STAREG|SCON,	TWORD|TCHAR|TUCHAR|TSHORT|TUSHORT,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	shrl ZA,AL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TUSHORT,
	SCON,	TANY,
		0,	RLEFT,
		"	shrw AR,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TUSHORT,
	STAREG|SAREG,	TANY,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	shrw ZA,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TSHORT,
	SCON,	TANY,
		0,	RLEFT,
		"	sarw AR,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TSHORT,
	STAREG|SAREG,	TANY,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	sarw ZA,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TUCHAR,
	SCON,	TANY,
		0,	RLEFT,
		"	shrb AR,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TUCHAR,
	STAREG|SAREG,	TANY,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	shrb ZA,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TCHAR,
	SCON,	TANY,
		0,	RLEFT,
		"	sarb AR,ZL\n", },

{ RS,	INTAREG|INAREG|FOREFF,
	STAREG|SAREG|SNAME|SOREG,	TCHAR,
	STAREG|SAREG,	TANY,
		3*NAREG|NASL|NSPECIAL,	RLEFT,
		"	sarb ZA,ZL\n", },

/*
 * The next rules takes care of assignments. "=".
 */
{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TLL,
	SCON,		TANY,
		0,	0,
		"	movl AR,AL\n	movl UR,UL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TWORD|TPOINT,
	SCON,		TANY,
		0,	0,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,		TANY,
		0,	0,
		"	movw ZR,ZL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,		TANY,
		0,	0,
		"	movb ZR,ZL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	STAREG|SAREG|SNAME|SOREG,	TLL,
	STAREG|SAREG,			TLL,
		0,	RRIGHT,
		"	movl AR,AL\n	movl UR,UL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TWORD|TPOINT,
	SAREG|STAREG,		TWORD|TPOINT,
		0,	RRIGHT,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG,			TWORD|TPOINT,
	SAREG|STAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RLEFT,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	STAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	STAREG,		TSHORT|TUSHORT,
		0,	RRIGHT,
		"	movw ZR,ZL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG|STAREG,			TCHAR|TUCHAR|TWORD,
		0,	RRIGHT,
		"	movb ZR,ZL\n", },

{ ASSIGN,	FOREFF|INTAREG,
	SFLD,		TANY,
	STAREG|SAREG,	TWORD,
		NAREG,	RRIGHT,
		"ZE", },

{ ASSIGN,	FOREFF|INTAREG,
	SFLD,		TANY,
	STAREG|SAREG|SNAME|SOREG|SCON,	TWORD,
		NAREG,	0,
		"ZE", },

/* order of table entries is very important here! */
{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLDOUBLE,
	STBREG|SBREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RRIGHT,
		"	fstt AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLDOUBLE,
	STBREG|SBREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	0,
		"	fstpt AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TDOUBLE,
	STBREG|SBREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RRIGHT,
		"	fstl AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TDOUBLE,
	STBREG|SBREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	0,
		"	fstpl AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TFLOAT,
	STBREG|SBREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RRIGHT,
		"	fsts AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TFLOAT,
	STBREG|SBREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	0,
		"	fstps AL\n", },
/* end very important order */

/* Not really an assign node */
{ MOVE,		FOREFF|INTAREG,
	SAREG|STAREG,	TWORD|TPOINT,
	SAREG|STAREG,	TWORD|TPOINT,
		NAREG,	RRIGHT,
		"	movl AL,AR\n" },

{ MOVE,		FOREFF|INTAREG,
	SAREG|STAREG,	TSHORT|TUSHORT,
	SAREG|STAREG,	TSHORT|TUSHORT,
		NAREG,	RRIGHT,
		"	movw ZL,ZR\n" },

{ MOVE,		FOREFF|INTAREG,
	SAREG|STAREG,	TCHAR|TUCHAR,
	SAREG|STAREG,	TCHAR|TUCHAR,
		NAREG,	RRIGHT,
		"	movb ZL,ZR\n" },
/*
 * DIV/MOD/MUL 
 */
{ DIV,	INTAREG,
	STAREG,				TSWORD,
	STAREG|SAREG|SNAME|SOREG,	TSWORD,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	cltd\n	idivl AR\n", },

{ DIV,	INTAREG,
	STAREG,				TUWORD|TPOINT,
	STAREG|SAREG|SNAME|SOREG,	TUWORD|TPOINT,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	xorl %edx,%edx\n	divl AR\n", },

{ DIV,	INTAREG,
	STAREG,				TUSHORT,
	STAREG|SAREG|SNAME|SOREG,	TUSHORT,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	xorl %edx,%edx\n	divw AR\n", },

{ DIV,	INTAREG,
	STAREG,				TUCHAR,
	STAREG|SAREG|SNAME|SOREG,	TUCHAR,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	xorl %ah,%ah\n	divb AR\n", },

{ DIV,	INTBREG,
	STBREG,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	fdivl AR\n", },

{ DIV,	INTBREG,
	STBREG,		TLDOUBLE|TDOUBLE|TFLOAT,
	STBREG,		TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fdivrp %st,%st(1)\n", },

{ MOD,	INTAREG,
	STAREG,				TSWORD,
	STAREG|SAREG|SNAME|SOREG,	TSWORD,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	cltd\n	idivl AR\n", },

{ MOD,	INTAREG,
	STAREG,				TUWORD|TPOINT,
	STAREG|SAREG|SNAME|SOREG,	TUWORD|TPOINT,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	xorl %edx,%edx\n	divl AR\n", },

{ MOD,	INTAREG,
	STAREG,				TUSHORT,
	STAREG|SAREG|SNAME|SOREG,	TUSHORT,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	xorl %edx,%edx\n	divw AR\n", },

{ MOD,	INTAREG,
	STAREG,				TUCHAR,
	STAREG|SAREG|SNAME|SOREG,	TUCHAR,
		3*NAREG|NASL|NSPECIAL,		RESC1,
		"	xorl %ah,%ah\n	divb AR\n", },

/* XXX - fix this */
{ MUL,	INTAREG,
	STAREG,		TCHAR|TUCHAR,
	STAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"	imulb ZR,ZL\n", },

{ MUL,	INTAREG,
	STAREG,			TSHORT|TUSHORT,
	STAREG|SNAME|SOREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	imulw ZR,ZL\n", },

{ MUL,	INTAREG,
	STAREG,				TWORD|TPOINT,
	STAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT,
		"	imull AR,AL\n", },

{ MUL,	INTBREG,
	STBREG,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	fmull AR\n", },

{ MUL,	INTBREG,
	STBREG,		TLDOUBLE|TDOUBLE|TFLOAT,
	STBREG,		TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fmulp %st,%st(1)\n", },

/*
 * Indirection operators.
 */
{ UMUL,	INTAREG,
	SAREG|STAREG,	TPTRTO|TLL,
	SANY,		TLL,
		NAREG|NASL,	RESC1,
		"	movl 4(AL),U1\n	movl (AL),A1\n", },

{ UMUL,	INTAREG,
	SAREG|STAREG,	TPOINT|TWORD,
	SANY,		TPOINT|TWORD,
		NAREG|NASL,	RESC1,
		"	movl (AL),A1\n", },

{ UMUL,	INTAREG,
	SAREG|STAREG,	TCHAR|TUCHAR|TPTRTO,
	SANY,		TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"	movb (AL),Z1\n", },

{ UMUL,	INTAREG,
	SAREG|STAREG,	TSHORT|TUSHORT|TPTRTO,
	SANY,		TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	movw (AL),Z1\n", },

{ UMUL,	INTBREG,
	SBREG|STBREG,	TLDOUBLE|TPTRTO,
	SANY,		TLDOUBLE,
		NBREG,	RESC1,
		"	fldq (AL)\n", },

{ UMUL,	INTBREG,
	SBREG|STBREG,	TDOUBLE|TPTRTO,
	SANY,		TDOUBLE,
		NBREG,	RESC1,
		"	fldl (AL)\n", },

{ UMUL,	INTBREG,
	SBREG|STBREG,	TFLOAT|TPTRTO,
	SANY,		TFLOAT,
		NBREG,	RESC1,
		"	flds (AL)\n", },

/*
 * INCR/DECR operators (post-increment)
 */
{ INCR,	INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TCHAR|TUCHAR|TPTRTO,
	SANY,	TANY,
		NAREG,	RESC1,
		"	movl ZL,Z1\n	incl ZL\n", },

{ INCR,	INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TWORD,
	SANY,	TANY,
		NAREG,	RESC1,
		"	movl ZL,Z1\n	incl ZL\n", },

{ INCR,	INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SANY,	TANY,
		NAREG,	RESC1,
		"	movw ZL,Z1\n	incw ZL\n", },

{ INCR,	INTAREG,
	SAREG|STAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SANY,	TANY,
		NAREG,	RESC1,
		"	movb ZL,Z1\n	incb ZL\n", },

/*
 * Logical/branching operators
 */

/* Comparisions, take care of everything */
{ OPLOG,	FORCC,
	SAREG|STAREG|SOREG|SNAME,	TLL,
	SAREG|STAREG,			TLL,
		0,	0,
		"ZD", },

{ OPLOG,	FORCC,
	SAREG|STAREG|SOREG|SNAME,	TWORD|TPOINT,
	SCON|SAREG|STAREG,	TWORD|TPOINT,
		0, 	RESCC,
		"	cmpl AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|STAREG|SOREG|SNAME,	TSHORT|TUSHORT,
	SCON|SAREG|STAREG,	TANY,
		0, 	RESCC,
		"	cmpw ZR,ZL\n", },

{ OPLOG,	FORCC,
	SAREG|STAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON|SAREG|STAREG,	TANY,
		0, 	RESCC,
		"	cmpb ZR,ZL\n", },

{ OPLOG,	FORCC,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		3*NAREG, 	0,
		"ZG", },

{ OPLOG,	FORCC,
	SOREG|SNAME,	TDOUBLE|TFLOAT,
	SBREG|STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		3*NAREG, 	0,
		"ZG", },

{ OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	0,
		"diediedie!", },

/* AND/OR/ER/NOT */
{ AND,	INTAREG|FOREFF,
	SAREG|STAREG|SOREG|SNAME,	TWORD,
	SCON|SAREG|STAREG,		TWORD,
		0,	RLEFT,
		"	andl AR,AL\n", },

{ AND,	INTAREG|FOREFF,
	SAREG|STAREG,			TLL,
	SAREG|STAREG|SOREG|SNAME,	TLL,
		0,	RLEFT,
		"	andl AR,AL\n	andl UR,UL\n", },

{ AND,	INTAREG|FOREFF,
	SAREG|STAREG,			TWORD,
	SAREG|STAREG|SOREG|SNAME,	TWORD,
		0,	RLEFT,
		"	andl AR,AL\n", },

{ AND,	INTAREG|FOREFF,  
	SAREG|STAREG|SOREG|SNAME,	TSHORT|TUSHORT,
	SCON|SAREG|STAREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"	andw ZR,ZL\n", },

{ AND,	INTAREG|FOREFF,  
	SAREG|STAREG,			TSHORT|TUSHORT,
	SAREG|STAREG|SOREG|SNAME,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	andw ZR,ZL\n", },

{ AND,	INTAREG|FOREFF,
	SAREG|STAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON|SAREG|STAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"	andb ZR,ZL\n", },

{ AND,	INTAREG|FOREFF,
	SAREG|STAREG,			TCHAR|TUCHAR,
	SAREG|STAREG|SOREG|SNAME,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	andb ZR,ZL\n", },






/* AND/OR/ER/NOT */

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp LL\n", },

/*
 * Convert LTYPE to reg.
 */
{ OPLTYPE,	INTAREG,
	SANY,	TANY,
	SCON|SOREG|SNAME,	TLL,
		NAREG,	RESC1,
		"	movl UL,U1\n	movl AL,A1\n", },

{ OPLTYPE,	INTAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TWORD|TPOINT,
		NAREG,	RESC1,
		"	movl AL,A1\n", },

{ OPLTYPE,	INTAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
		NAREG,	RESC1,
		"	movb ZL,Z1\n", },

{ OPLTYPE,	INTAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME|SCON,	TSHORT|TUSHORT,
		NAREG,	RESC1,
		"	movw ZL,Z1\n", },

{ OPLTYPE,	INTBREG,
	SANY,		TLDOUBLE,
	SOREG|SNAME,	TLDOUBLE,
		NBREG,	RESC1,
		"	fldt AL\n", },

{ OPLTYPE,	INTBREG,
	SANY,		TDOUBLE,
	SOREG|SNAME,	TDOUBLE,
		NBREG,	RESC1,
		"	fldl AL\n", },

{ OPLTYPE,	INTBREG,
	SANY,		TFLOAT,
	SOREG|SNAME,	TFLOAT,
		NBREG,	RESC1,
		"	flds AL\n", },

/*
 * Negate a word.
 */

{ UMINUS,	INAREG|INTAREG|FOREFF,
	STAREG,	TLL,
	STAREG,	TLL,
		0,	RLEFT,
		"	negl AL\n	adcl $0,UL\n	negl UL\n", },

{ UMINUS,	INAREG|INTAREG|FOREFF,
	STAREG,	TWORD|TPOINT,
	STAREG,	TWORD|TPOINT,
		0,	RLEFT,
		"	negl AL\n", },

{ UMINUS,	INAREG|INTAREG|FOREFF,
	STAREG,	TSHORT|TUSHORT,
	STAREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	negw ZL\n", },

{ UMINUS,	INAREG|INTAREG|FOREFF,
	STAREG,	TCHAR|TUCHAR,
	STAREG,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	negb ZL\n", },

{ UMINUS,	INBREG|INTBREG|FOREFF,
	STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
	STBREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fchs\n", },

#if 0
{ UMINUS,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG|SNAME|SOREG,	TLL,
	SANY,	TLL,
		NAREG|NASR,	RESC1,
		"	dmovn A1,AL\n", },
#endif

{ COMPL,	INTAREG,
	SAREG|STAREG,	TWORD,
	SANY,	TANY,
		NASL,	RLEFT,
		"	notl AL\n", },

{ COMPL,	INTAREG,
	SAREG|STAREG,	TSHORT|TUSHORT,
	SANY,	TANY,
		NASL,	RLEFT,
		"	notw ZL\n", },

{ COMPL,	INTAREG,
	SAREG|STAREG,	TCHAR|TUCHAR,
	SANY,	TANY,
		NASL,	RLEFT,
		"	notb ZL\n", },

/*
 * Arguments to functions.
 */
{ FUNARG,	FOREFF,
	SCON|SAREG|SNAME|SOREG,	TLL,
	SANY,	TLL,
		0,	RNULL,
		"	pushl UL\n	pushl AL\n", },

{ FUNARG,	FOREFF,
	SCON|SAREG|SNAME|SOREG,	TWORD|TPOINT|TFLOAT,
	SANY,	TWORD|TPOINT|TFLOAT,
		0,	RNULL,
		"	pushl AL\n", },

{ FUNARG,	FOREFF,
	SCON,	TWORD|TSHORT|TUSHORT|TCHAR|TUCHAR,
	SANY,	TWORD|TSHORT|TUSHORT|TCHAR|TUCHAR,
		0,	RNULL,
		"	pushl AL\n", },

{ FUNARG,	FOREFF,
	SAREG|SNAME|SOREG,	TSHORT,
	SANY,	TSHORT,
		NAREG,	0,
		"	movswl ZL,A1\n	pushl A1\n", },

{ FUNARG,	FOREFF,
	SAREG|SNAME|SOREG,	TUSHORT,
	SANY,	TUSHORT,
		NAREG,	0,
		"	movzwl ZL,A1\n	pushl A1\n", },

{ FUNARG,	FOREFF,
	SAREG|SNAME|SOREG,	TCHAR,
	SANY,	TCHAR,
		NAREG,	0,
		"	movsbl ZL,A1\n	pushl A1\n", },

{ FUNARG,	FOREFF,
	SAREG|SNAME|SOREG,	TUCHAR,
	SANY,	TUCHAR,
		NAREG,	0,
		"	movzbl ZL,A1\n	pushl A1\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TDOUBLE,
	SANY,	TDOUBLE,
		0,	0,
		"	pushl UL\n	pushl AL\n", },

{ FUNARG,	FOREFF,
	SBREG|STBREG,	TDOUBLE,
	SANY,		TDOUBLE,
		0,	0,
		"	subl $8,%esp\n	fstpl (%esp)\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TFLOAT,
	SANY,		TFLOAT,
		0,	0,
		"	pushl AL\n", },

{ FUNARG,	FOREFF,
	SBREG|STBREG,	TFLOAT,
	SANY,		TFLOAT,
		0,	0,
		"	subl $4,%esp\n	fstps (%esp)\n", },

{ FUNARG,	FOREFF,
	SBREG|STBREG,	TLDOUBLE,
	SANY,		TLDOUBLE,
		0,	0,
		"	subl $12,%esp\n	fstpt (%esp)\n", },

{ FUNARG,	FOREFF,
	STAREG|SAREG|SOREG|SNAME|SCON,	TANY,
	SANY,	TSTRUCT,
		3*NAREG|NASL,	0,
		"ZF", },

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ INCR, DF(INCR), },

{ DECR, DF(INCR), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ FLD, DF(FLD), },

{ OPLEAF, DF(NAME), },

{ INIT, DF(INIT), },

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" },
};

int tablesize = sizeof(table)/sizeof(table[0]);
