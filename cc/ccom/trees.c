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

/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Some of the changes from 32V include:
 * - Understand "void" as type.
 * - Handle enums as ints everywhere.
 * - Convert some C-specific ops into branches.
 */

# include "pass1.h"
# include "pass2.h"

# include <stdarg.h>

static void chkpun(NODE *p);
static int opact(NODE *p);
static int moditype(TWORD);
static NODE *strargs(NODE *);
static void rmcops(NODE *p);

int lastloc = -1;

/*	some special actions, used in finding the type of nodes */
# define NCVT 01
# define PUN 02
# define TYPL 04
# define TYPR 010
# define TYMATCH 040
# define LVAL 0100
# define CVTO 0200
# define CVTL 0400
# define CVTR 01000
# define PTMATCH 02000
# define OTHER 04000
# define NCVTR 010000

/* node conventions:

	NAME:	rval>0 is stab index for external
		rval<0 is -inlabel number
		lval is offset in bits
	ICON:	lval has the value
		rval has the STAB index, or - label number,
			if a name whose address is in the constant
		rval = NONAME means no name
	REG:	rval is reg. identification cookie

	*/

int bdebug = 0;

NODE *
buildtree(int o, NODE *l, NODE *r)
{
	NODE *p, *q;
	int actions;
	int opty;
	struct symtab *sp;
	NODE *lr, *ll;
	char *name;
	struct symtab **elem;

#ifdef PCC_DEBUG
	if (bdebug)
		printf("buildtree(%s, %p, %p)\n", copst(o), l, r);
#endif
	opty = coptype(o);

	/* check for constants */

	if( opty == UTYPE && l->n_op == ICON ){

		switch( o ){

		case NOT:
		case UNARY MINUS:
		case COMPL:
			if( conval( l, o, l ) ) return(l);
			break;
		}
	} else if (o == NOT && l->n_op == FCON) {
		l = clocal(block(SCONV, l, NIL, INT, 0, MKSUE(INT)));
	} else if( o == UNARY MINUS && l->n_op == FCON ){
			l->n_dcon = -l->n_dcon;
			return(l);

	} else if( o==QUEST && l->n_op==ICON ) {
		CONSZ c = l->n_lval;
		nfree(l);
		if (c) {
			tfree(r->n_right);
			l = r->n_left;
			nfree(r);
			return(l);
		} else {
			tfree(r->n_left);
			l = r->n_right;
			nfree(r);
			return(l);
		}
	} else if( (o==ANDAND || o==OROR) && (l->n_op==ICON||r->n_op==ICON) )
		goto ccwarn;

	else if( opty == BITYPE && l->n_op == ICON && r->n_op == ICON ){

		switch( o ){

		case ULT:
		case UGT:
		case ULE:
		case UGE:
		case LT:
		case GT:
		case LE:
		case GE:
		case EQ:
		case NE:
		case ANDAND:
		case OROR:

		ccwarn:

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			if( conval( l, o, r ) ) {
				nfree(r);
				return(l);
			}
			break;
		}
	} else if (opty == BITYPE && (l->n_op == FCON || l->n_op == ICON) &&
	    (r->n_op == FCON || r->n_op == ICON) && (o == PLUS || o == MINUS ||
	    o == MUL || o == DIV)) {
		switch(o){
		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
			if (l->n_op == ICON)
				l->n_dcon = l->n_lval;
			if (r->n_op == ICON)
				r->n_dcon = r->n_lval;
			switch (o) {
			case PLUS:
				l->n_dcon += r->n_dcon; break;
			case MINUS:
				l->n_dcon -= r->n_dcon; break;
			case MUL:
				l->n_dcon *= r->n_dcon; break;
			case DIV:
				if (r->n_dcon == 0)
					uerror("division by 0.");
				else
					l->n_dcon /= r->n_dcon;
			}
			l->n_op = FCON;
			l->n_type = DOUBLE;
			l->n_sue = MKSUE(DOUBLE);
			nfree(r);
			return(l);
		}
	}

	/* its real; we must make a new node */

	p = block(o, l, r, INT, 0, MKSUE(INT));

	actions = opact(p);

	if (actions & LVAL) { /* check left descendent */
		if (notlval(p->n_left)) {
			uerror("lvalue required");
#ifdef notyet
		} else {
			if ((l->n_type > BTMASK && ISCON(l->n_qual)) ||
			    (l->n_type <= BTMASK && ISCON(l->n_qual << TSHIFT)))
				if (blevel > 0)
					uerror("lvalue is declared const");
#endif
		}
	}

	if( actions & NCVTR ){
		p->n_left = pconvert( p->n_left );
		}
	else if( !(actions & NCVT ) ){
		switch( opty ){

		case BITYPE:
			p->n_right = pconvert( p->n_right );
		case UTYPE:
			p->n_left = pconvert( p->n_left );

			}
		}

	if ((actions&PUN) && (o!=CAST))
		chkpun(p);

	if( actions & (TYPL|TYPR) ){

		q = (actions&TYPL) ? p->n_left : p->n_right;

		p->n_type = q->n_type;
		p->n_qual = q->n_qual;
		p->n_df = q->n_df;
		p->n_sue = q->n_sue;
		}

	if( actions & CVTL ) p = convert( p, CVTL );
	if( actions & CVTR ) p = convert( p, CVTR );
	if( actions & TYMATCH ) p = tymatch(p);
	if( actions & PTMATCH ) p = ptmatch(p);

	if( actions & OTHER ){
		l = p->n_left;
		r = p->n_right;

		switch(o){

		case NAME:
			sp = spname;
			if (sp->stype == UNDEF) {
				uerror("%s undefined", sp->sname);
				/* make p look reasonable */
				p->n_type = INT;
				p->n_sue = MKSUE(INT);
				p->n_df = NULL;
				p->n_sp = sp;
				p->n_lval = 0;
				defid(p, SNULL);
				break;
			}
			p->n_type = sp->stype;
			p->n_qual = sp->squal;
			p->n_df = sp->sdf;
			p->n_sue = sp->ssue;
			p->n_lval = 0;
			p->n_sp = sp;
			/* special case: MOETY is really an ICON... */
			if (p->n_type == MOETY) {
				p->n_sp = NULL;
				p->n_lval = sp->soffset;
				p->n_df = NULL;
				p->n_type = ENUMTY;
				p->n_op = ICON;
			}
			if (sp->sflags & STNODE) {
				p->n_lval = sp->soffset;
				p->n_op = TEMP;
			}
			break;

		case STRING:
			p->n_op = NAME;
#ifdef CHAR_UNSIGNED
			p->n_type = UCHAR+ARY;
			p->n_sue = MKSUE(UCHAR);
#else
			p->n_type = CHAR+ARY;
			p->n_sue = MKSUE(CHAR);
#endif
			p->n_lval = 0;
			p->n_sp = NULL;
			break;

		case STREF:
			/* p->x turned into *(p+offset) */
			/* rhs must be a name; check correctness */

			/* Find member symbol struct */
			if (l->n_type != PTR+STRTY && l->n_type != PTR+UNIONTY){
				uerror("struct or union required");
				break;
			}

			if ((elem = l->n_sue->suelem) == NULL)
				uerror("undefined struct or union");

			name = r->n_name;
			for (; *elem != NULL; elem++) {
				sp = *elem;
				if (sp->sname == name)
					break;
			}
			if (*elem == NULL)
				uerror("member '%s' not declared", name);

			r->n_sp = sp;
			p = stref(p);
			break;

		case UNARY MUL:
			if (l->n_op == UNARY AND) {
				nfree(p);
				p = l->n_left;
				nfree(l);
			}
			if( !ISPTR(l->n_type))uerror("illegal indirection");
			p->n_type = DECREF(l->n_type);
			p->n_qual = DECREF(l->n_qual);
			p->n_df = l->n_df;
			p->n_sue = l->n_sue;
			break;

		case UNARY AND:
			switch( l->n_op ){

			case UNARY MUL:
				nfree(p);
				p = l->n_left;
				nfree(l);
			case NAME:
				p->n_type = INCREF(l->n_type);
				p->n_qual = INCQAL(l->n_qual);
				p->n_df = l->n_df;
				p->n_sue = l->n_sue;
				break;

			case COMOP:
				nfree(p);
				lr = buildtree(UNARY AND, l->n_right, NIL);
				p = buildtree( COMOP, l->n_left, lr );
				nfree(l);
				break;

			case QUEST:
				lr = buildtree( UNARY AND, l->n_right->n_right, NIL );
				ll = buildtree( UNARY AND, l->n_right->n_left, NIL );
				nfree(p); nfree(l->n_right);
				p = buildtree( QUEST, l->n_left, buildtree( COLON, ll, lr ) );
				nfree(l);
				break;

			default:
				uerror("unacceptable operand of &: %d", l->n_op );
				break;
				}
			break;

		case LS:
		case RS:
		case ASG LS:
		case ASG RS:
			if(tsize(p->n_right->n_type, p->n_right->n_df, p->n_right->n_sue) > SZINT)
				p->n_right = r = makety(r, INT, 0, 0, MKSUE(INT));
			break;

		case RETURN:
		case ASSIGN:
		case CAST:
			/* structure assignment */
			/* take the addresses of the two sides; then make an
			 * operator using STASG and
			 * the addresses of left and right */

			{
				struct suedef *sue;
				TWORD t;
				union dimfun *d;

				if (l->n_sue != r->n_sue)
					uerror("assignment of different structures");

				r = buildtree(UNARY AND, r, NIL);
				t = r->n_type;
				d = r->n_df;
				sue = r->n_sue;

				l = block(STASG, l, r, t, d, sue);

				if( o == RETURN ){
					nfree(p);
					p = l;
					break;
				}

				p->n_op = UNARY MUL;
				p->n_left = l;
				p->n_right = NIL;
				break;
				}
		case COLON:
			/* structure colon */

			if (l->n_sue != r->n_sue)
				uerror( "type clash in conditional" );
			break;

		case CALL:
			p->n_right = r = strargs(p->n_right);
		case UNARY CALL:
			if (!ISPTR(l->n_type))
				uerror("illegal function");
			p->n_type = DECREF(l->n_type);
			if (!ISFTN(p->n_type))
				uerror("illegal function");
			p->n_type = DECREF(p->n_type);
			p->n_df = l->n_df;
			p->n_sue = l->n_sue;
			if (l->n_op == UNARY AND && l->n_left->n_op == NAME &&
			    l->n_left->n_sp != NULL && l->n_left->n_sp != NULL &&
			    (l->n_left->n_sp->sclass == FORTRAN ||
			    l->n_left->n_sp->sclass == UFORTRAN)) {
				p->n_op += (FORTCALL-CALL);
			}
			if (p->n_type == STRTY || p->n_type == UNIONTY) {
				/* function returning structure */
				/*  make function really return ptr to str., with * */

				p->n_op += STCALL-CALL;
				p->n_type = INCREF(p->n_type);
				p = buildtree(UNARY MUL, p, NIL);

				}
			break;

		default:
			cerror( "other code %d", o );
			}

		}

	/*
	 * Allow (void)0 casts.
	 * XXX - anything on the right side must be possible to cast.
	 * XXX - remove void types further on.
	 */
	if (p->n_op == CAST && p->n_type == VOID &&
	    p->n_right->n_op == ICON)
		p->n_right->n_type = VOID;

	if (actions & CVTO)
		p = oconvert(p);
	p = clocal(p);

#ifdef PCC_DEBUG
	if (bdebug) {
		printf("End of buildtree:\n");
		fwalk(p, eprint, 0);
	}
#endif

	return(p);

	}

NODE *
strargs( p ) register NODE *p;  { /* rewrite structure flavored arguments */

	if( p->n_op == CM ){
		p->n_left = strargs( p->n_left );
		p->n_right = strargs( p->n_right );
		return( p );
		}

	if( p->n_type == STRTY || p->n_type == UNIONTY ){
		p = block(STARG, p, NIL, p->n_type, p->n_df, p->n_sue);
		p->n_left = buildtree( UNARY AND, p->n_left, NIL );
		p = clocal(p);
		}
	return( p );
}

/*
 * apply the op o to the lval part of p; if binary, rhs is val
 */
int
conval(NODE *p, int o, NODE *q)
{
	int i, u;
	CONSZ val;

	val = q->n_lval;
	u = ISUNSIGNED(p->n_type) || ISUNSIGNED(q->n_type);
	if( u && (o==LE||o==LT||o==GE||o==GT)) o += (UGE-GE);

	if (p->n_sp != NULL && q->n_sp != NULL)
		return(0);
	if (q->n_sp != NULL && o != PLUS)
		return(0);
	if (p->n_sp != NULL && o != PLUS && o != MINUS)
		return(0);

	switch( o ){

	case PLUS:
		p->n_lval += val;
		if (p->n_sp == NULL) {
			p->n_rval = q->n_rval;
			p->n_type = q->n_type;
		}
		break;
	case MINUS:
		p->n_lval -= val;
		break;
	case MUL:
		p->n_lval *= val;
		break;
	case DIV:
		if (val == 0)
			uerror("division by 0");
		else 
			p->n_lval /= val;
		break;
	case MOD:
		if (val == 0)
			uerror("division by 0");
		else 
			p->n_lval %= val;
		break;
	case AND:
		p->n_lval &= val;
		break;
	case OR:
		p->n_lval |= val;
		break;
	case ER:
		p->n_lval ^= val;
		break;
	case LS:
		i = val;
		p->n_lval = p->n_lval << i;
		break;
	case RS:
		i = val;
		p->n_lval = p->n_lval >> i;
		break;

	case UNARY MINUS:
		p->n_lval = - p->n_lval;
		break;
	case COMPL:
		p->n_lval = ~p->n_lval;
		break;
	case NOT:
		p->n_lval = !p->n_lval;
		break;
	case LT:
		p->n_lval = p->n_lval < val;
		break;
	case LE:
		p->n_lval = p->n_lval <= val;
		break;
	case GT:
		p->n_lval = p->n_lval > val;
		break;
	case GE:
		p->n_lval = p->n_lval >= val;
		break;
	case ULT:
		p->n_lval = (p->n_lval-val)<0;
		break;
	case ULE:
		p->n_lval = (p->n_lval-val)<=0;
		break;
	case UGT:
		p->n_lval = (p->n_lval-val)>=0;
		break;
	case UGE:
		p->n_lval = (p->n_lval-val)>0;
		break;
	case EQ:
		p->n_lval = p->n_lval == val;
		break;
	case NE:
		p->n_lval = p->n_lval != val;
		break;
	default:
		return(0);
		}
	return(1);
	}

/*
 * Checks p for the existance of a pun.  This is called when the op of p
 * is ASSIGN, RETURN, CAST, COLON, or relational.
 * One case is when enumerations are used: this applies only to lint.
 * In the other case, one operand is a pointer, the other integer type
 * we check that this integer is in fact a constant zero...
 * in the case of ASSIGN, any assignment of pointer to integer is illegal
 * this falls out, because the LHS is never 0.
 */
void
chkpun(NODE *p)
{
	union dimfun *d1, *d2;
	NODE *q;
	int t1, t2;

	t1 = p->n_left->n_type;
	t2 = p->n_right->n_type;

	switch (p->n_op) {
	case RETURN:
		/* return of void allowed but nothing else */
		if (t1 == VOID && t2 == VOID)
			return;
		if (t1 == VOID)
			return uerror("returning value from void function");
		if (t2 == VOID)
			return uerror("using void value");
	case COLON:
		if (t1 == VOID && t2 == VOID)
			return;
		break;
	default:
		if ((t1 == VOID && t2 != VOID) || (t1 != VOID && t2 == VOID))
			return uerror("value of void expression used");
		break;
	}

	/* allow void pointer assignments in any direction */
	if (BTYPE(t1) == VOID && (t2 & TMASK))
		return;
	if (BTYPE(t2) == VOID && (t1 & TMASK))
		return;

	/* check for enumerations */
	if (t1==ENUMTY || t2==ENUMTY) {
		if( clogop( p->n_op ) && p->n_op != EQ && p->n_op != NE ) {
			werror( "comparison of enums" );
			return;
			}
		if (t1==ENUMTY && t2==ENUMTY) {
			if (p->n_left->n_sue!=p->n_right->n_sue)
				werror("enumeration type clash, "
				    "operator %s", copst(p->n_op));
			return;
		}
		if ((t1 == ENUMTY && t2 <= BTMASK) ||
		    (t2 == ENUMTY && t1 <= BTMASK))
			return;
	}

	if (ISPTR(t1) || ISARY(t1))
		q = p->n_right;
	else
		q = p->n_left;

	if (!ISPTR(q->n_type) && !ISARY(q->n_type)) {
		if (q->n_op != ICON || q->n_lval != 0)
			werror("illegal combination of pointer and integer");
	} else {
		d1 = p->n_left->n_df;
		d2 = p->n_right->n_df;
		if (t1 == t2) {
			if (p->n_left->n_sue != p->n_right->n_sue)
				werror("illegal structure pointer combination");
			return;
		}
		for (;;) {
			if (ISARY(t1) || ISPTR(t1)) {
				if (!ISARY(t2) && !ISPTR(t2))
					break;
				if (ISARY(t1) && ISARY(t2) && d1->ddim != d2->ddim) {
					werror("illegal array size combination");
					return;
				}
				if (ISARY(t1))
					++d1;
				if (ISARY(t2))
					++d2;
			} else if (ISFTN(t1)) {
				if (chkftn(d1->dfun, d2->dfun)) {
					werror("illegal function "
					    "pointer combination");
					return;
				}
				++d1;
				++d2;
			} else
				break;
			t1 = DECREF(t1);
			t2 = DECREF(t2);
		}
		werror("illegal pointer combination");
	}
}

NODE *
stref(NODE *p)
{
	NODE *r;
	struct suedef *sue;
	union dimfun *d;
	TWORD t, q;
	int dsc;
	OFFSZ off;
	struct symtab *s;

	/* make p->x */
	/* this is also used to reference automatic variables */

	s = p->n_right->n_sp;
	nfree(p->n_right);
	r = p->n_left;
	nfree(p);
	p = pconvert(r);

	/* make p look like ptr to x */

	if (!ISPTR(p->n_type))
		p->n_type = PTR+UNIONTY;

	t = INCREF(s->stype);
	q = INCQAL(s->squal);
	d = s->sdf;
	sue = s->ssue;

	p = makety(p, t, q, d, sue);

	/* compute the offset to be added */

	off = s->soffset;
	dsc = s->sclass;

	if (dsc & FIELD) {  /* make fields look like ints */
		off = (off/ALINT)*ALINT;
		sue = MKSUE(INT);
	}
	if (off != 0) {
		p = block(PLUS, p, offcon(off, t, d, sue), t, d, sue);
		p->n_qual = q;
		p = optim(p);
	}

	p = buildtree(UNARY MUL, p, NIL);

	/* if field, build field info */

	if (dsc & FIELD) {
		p = block(FLD, p, NIL, s->stype, 0, s->ssue);
		p->n_qual = q;
		p->n_rval = PKFIELD(dsc&FLDSIZ, s->soffset%ALINT);
	}

	p = clocal(p);
	return p;
}

int
notlval(p) register NODE *p; {

	/* return 0 if p an lvalue, 1 otherwise */

	again:

	switch( p->n_op ){

	case FLD:
		p = p->n_left;
		goto again;

	case NAME:
	case OREG:
	case UNARY MUL:
		if( ISARY(p->n_type) || ISFTN(p->n_type) ) return(1);
	case TEMP:
	case REG:
		return(0);

	default:
		return(1);

		}

	}
/* make a constant node with value i */
NODE *
bcon(int i)
{
	register NODE *p;

	p = block(ICON, NIL, NIL, INT, 0, MKSUE(INT));
	p->n_lval = i;
	p->n_sp = NULL;
	return(clocal(p));
}

NODE *
bpsize(NODE *p)
{
	return(offcon(psize(p), p->n_type, p->n_df, p->n_sue));
}

/*
 * p is a node of type pointer; psize returns the
 * size of the thing pointed to
 */
OFFSZ
psize(NODE *p)
{

	if (!ISPTR(p->n_type)) {
		uerror("pointer required");
		return(SZINT);
	}
	/* note: no pointers to fields */
	return(tsize(DECREF(p->n_type), p->n_df, p->n_sue));
}

/*
 * convert an operand of p
 * f is either CVTL or CVTR
 * operand has type int, and is converted by the size of the other side
 * convert is called when an integer is to be added to a pointer, for
 * example in arrays or structures.
 */
NODE *
convert(NODE *p, int f)
{
	union dimfun *df;
	TWORD ty, ty2;
	NODE *q, *r, *s, *rv;

	if (f == CVTL) {
		q = p->n_left;
		s = p->n_right;
	} else {
		q = p->n_right;
		s = p->n_left;
	}
	ty2 = ty = DECREF(s->n_type);
	while (ISARY(ty))
		ty = DECREF(ty);

	r = offcon(tsize(ty, s->n_df, s->n_sue), s->n_type, s->n_df, s->n_sue);
	ty = ty2;
	rv = bcon(1);
	df = s->n_df;
	while (ISARY(ty)) {
		rv = buildtree(MUL, rv, df->ddim >= 0 ? bcon(df->ddim) :
		    tempnode(-df->ddim, INT, 0, MKSUE(INT)));
		df++;
		ty = DECREF(ty);
	}
	rv = clocal(block(PMCONV, rv, r, INT, 0, MKSUE(INT)));
	rv = optim(rv);

	r = block(PMCONV, q, rv, INT, 0, MKSUE(INT));
	r = clocal(r);
	/*
	 * Indexing is only allowed with integer arguments, so insert
	 * SCONV here if arg is not an integer.
	 * XXX - complain?
	 */
	if (r->n_type != INT)
		r = clocal(block(SCONV, r, NIL, INT, 0, MKSUE(INT)));
	if (f == CVTL)
		p->n_left = r;
	else
		p->n_right = r;
	return(p);
}

/*
 * change enums to ints, or appropriate types
 */
void
econvert( p ) register NODE *p; {


	register TWORD ty;

	if( (ty=BTYPE(p->n_type)) == ENUMTY || ty == MOETY ) {
		if (p->n_sue->suesize == SZCHAR)
			ty = CHAR;
		else if (p->n_sue->suesize == SZINT)
			ty = INT;
		else if (p->n_sue->suesize == SZSHORT)
			ty = SHORT;
		else if (p->n_sue->suesize == SZLONGLONG)
			ty = LONGLONG;
		else
			ty = LONG;
		ty = ctype(ty);
		p->n_sue = MKSUE(ty);
		MODTYPE(p->n_type,ty);
		if (p->n_op == ICON && ty != LONG && ty != LONGLONG)
			p->n_type = INT, p->n_sue = MKSUE(INT);
	}
}

NODE *
pconvert( p ) register NODE *p; {

	/* if p should be changed into a pointer, do so */

	if( ISARY( p->n_type) ){
		p->n_type = DECREF( p->n_type );
		++p->n_df;
		return( buildtree( UNARY AND, p, NIL ) );
	}
	if( ISFTN( p->n_type) )
		return( buildtree( UNARY AND, p, NIL ) );

	return( p );
	}

NODE *
oconvert(p) register NODE *p; {
	/* convert the result itself: used for pointer and unsigned */

	switch(p->n_op) {

	case LE:
	case LT:
	case GE:
	case GT:
		if( ISUNSIGNED(p->n_left->n_type) || ISUNSIGNED(p->n_right->n_type) )  p->n_op += (ULE-LE);
	case EQ:
	case NE:
		return( p );

	case MINUS:
		return(  clocal( block( PVCONV,
			p, bpsize(p->n_left), INT, 0, MKSUE(INT))));
		}

	cerror( "illegal oconvert: %d", p->n_op );

	return(p);
	}

/*
 * makes the operands of p agree; they are
 * either pointers or integers, by this time
 * with MINUS, the sizes must be the same
 * with COLON, the types must be the same
 */
NODE *
ptmatch(NODE *p)
{
	struct suedef *sue, *sue2;
	union dimfun *d, *d2;
	TWORD t1, t2, t, q1, q2, q;
	int o;

	o = p->n_op;
	t = t1 = p->n_left->n_type;
	q = q1 = p->n_left->n_qual;
	t2 = p->n_right->n_type;
	q2 = p->n_right->n_qual;
	d = p->n_left->n_df;
	d2 = p->n_right->n_df;
	sue = p->n_left->n_sue;
	sue2 = p->n_right->n_sue;

	switch( o ){

	case ASSIGN:
	case RETURN:
	case CAST:
		{  break; }

	case MINUS:
		{  if( psize(p->n_left) != psize(p->n_right) ){
			uerror( "illegal pointer subtraction");
			}
		   break;
		   }
	case COLON:
		if (t1 != t2) {
			/*
			 * Check for void pointer types. They are allowed
			 * to cast to/from any pointers.
			 */
			if (ISPTR(t1) && ISPTR(t2) &&
			    (BTYPE(t1) == VOID || BTYPE(t2) == VOID))
				break;
			uerror("illegal types in :");
		}
		break;

	default:  /* must work harder: relationals or comparisons */

		if( !ISPTR(t1) ){
			t = t2;
			q = q2;
			d = d2;
			sue = sue2;
			break;
			}
		if( !ISPTR(t2) ){
			break;
			}

		/* both are pointers */
		if( talign(t2,sue2) < talign(t,sue) ){
			t = t2;
			q = q2;
			sue = sue2;
			}
		break;
		}

	p->n_left = makety( p->n_left, t, q, d, sue );
	p->n_right = makety( p->n_right, t, q, d, sue );
	if( o!=MINUS && !clogop(o) ){

		p->n_type = t;
		p->n_qual = q;
		p->n_df = d;
		p->n_sue = sue;
		}

	return(clocal(p));
	}

int tdebug = 0;

NODE *
tymatch(p)  register NODE *p; {

	/* satisfy the types of various arithmetic binary ops */

	/* rules are:
		if assignment, type of LHS
		if any float or doubles, make double
		if any longs, make long
		otherwise, make int
		if either operand is unsigned, the result is...
	*/

	TWORD t1, t2, t, tu;
	int o, u;

	o = p->n_op;

	t1 = p->n_left->n_type;
	t2 = p->n_right->n_type;

	u = 0;
	if( ISUNSIGNED(t1) ){
		u = 1;
		t1 = DEUNSIGN(t1);
		}
	if( ISUNSIGNED(t2) ){
		u = 1;
		t2 = DEUNSIGN(t2);
		}

	if( ( t1 == CHAR || t1 == SHORT ) && o!= RETURN ) t1 = INT;
	if( t2 == CHAR || t2 == SHORT ) t2 = INT;

	if (t1 == FLOAT && t2 == FLOAT)
		t = FLOAT;
	else if (t1 == DOUBLE || t2 == DOUBLE)
		t = DOUBLE;
	else if (t1==LONG || t2==LONG)
		t = LONG;
	else if (t1==LONGLONG || t2 == LONGLONG)
		t = LONGLONG;
	else
		t = INT;

	if( casgop(o) ){
		tu = p->n_left->n_type;
		t = t1;
	} else {
		tu = (u && UNSIGNABLE(t))?ENUNSIGN(t):t;
	}

	/* because expressions have values that are at least as wide
	   as INT or UNSIGNED, the only conversions needed
	   are those involving FLOAT/DOUBLE, and those
	   from LONG to INT and ULONG to UNSIGNED */


	if( t != t1 ) p->n_left = makety( p->n_left, tu, 0, 0, MKSUE(tu));

	if( t != t2 || o==CAST) p->n_right = makety( p->n_right, tu, 0, 0, MKSUE(tu));

	if( casgop(o) ){
		p->n_type = p->n_left->n_type;
		p->n_df = p->n_left->n_df;
		p->n_sue = p->n_left->n_sue;
		}
	else if( !clogop(o) ){
		p->n_type = tu;
		p->n_df = NULL;
		p->n_sue = MKSUE(t);
		}

#ifdef PCC_DEBUG
	if (tdebug)
		printf("tymatch(%p): %o %s %o => %o\n",p,t1,copst(o),t2,tu );
#endif

	return(p);
	}

/*
 * make p into type t by inserting a conversion
 */
NODE *
makety(NODE *p, TWORD t, TWORD q, union dimfun *d, struct suedef *sue)
{

	if (p->n_type == ENUMTY && p->n_op == ICON)
		econvert(p);
	if (t == p->n_type) {
		p->n_df = d;
		p->n_sue = sue;
		p->n_qual = q;
		return(p);
	}

	if ((p->n_type == FLOAT || p->n_type == DOUBLE || p->n_type == LDOUBLE)
	    && (t == FLOAT || t == DOUBLE || t == LDOUBLE) && p->n_op == FCON) {
		p->n_type = t;
		p->n_qual = q;
		p->n_df = d;
		p->n_sue = sue;
		return(p);
	}

	if (t & TMASK) {
		/* non-simple type */
		p = block(PCONV, p, NIL, t, d, sue);
		p->n_qual = q;
		return clocal(p);
	}

	if (p->n_op == ICON) {
		if (t == DOUBLE || t == FLOAT) {
			p->n_op = FCON;
			if (ISUNSIGNED(p->n_type))
				p->n_dcon = (U_CONSZ) p->n_lval;
			else
				p->n_dcon = p->n_lval;
			p->n_type = t;
			p->n_qual = q;
			p->n_sue = MKSUE(t);
			return (clocal(p));
		}
	}
	p = block(SCONV, p, NIL, t, d, sue);
	p->n_qual = q;
	return clocal(p);

}

NODE *
block(int o, NODE *l, NODE *r, TWORD t, union dimfun *d, struct suedef *sue)
{
	register NODE *p;

	p = talloc();
	p->n_rval = 0;
	p->n_op = o;
	p->n_lval = 0; /* Protect against large lval */
	p->n_left = l;
	p->n_right = r;
	p->n_type = t;
	p->n_qual = 0;
	p->n_df = d;
	p->n_sue = sue;
	return(p);
	}

int
icons(p) register NODE *p; {
	/* if p is an integer constant, return its value */
	int val;

	if( p->n_op != ICON ){
		uerror( "constant expected");
		val = 1;
		}
	else {
		val = p->n_lval;
		if( val != p->n_lval ) uerror( "constant too big for cross-compiler" );
		}
	tfree( p );
	return(val);
}

/* 
 * the intent of this table is to examine the
 * operators, and to check them for
 * correctness.
 * 
 * The table is searched for the op and the
 * modified type (where this is one of the
 * types INT (includes char and short), LONG,
 * DOUBLE (includes FLOAT), and POINTER
 * 
 * The default action is to make the node type integer
 * 
 * The actions taken include:
 * 	PUN	  check for puns
 * 	CVTL	  convert the left operand
 * 	CVTR	  convert the right operand
 * 	TYPL	  the type is determined by the left operand
 * 	TYPR	  the type is determined by the right operand
 * 	TYMATCH	  force type of left and right to match,by inserting conversions
 * 	PTMATCH	  like TYMATCH, but for pointers
 * 	LVAL	  left operand must be lval
 * 	CVTO	  convert the op
 * 	NCVT	  do not convert the operands
 * 	OTHER	  handled by code
 * 	NCVTR	  convert the left operand, not the right...
 * 
 */

# define MINT 01	/* integer */
# define MDBI 02	/* integer or double */
# define MSTR 04	/* structure */
# define MPTR 010	/* pointer */
# define MPTI 020	/* pointer or integer */
# define MENU 040	/* enumeration variable or member */

int
opact(NODE *p)
{
	int mt12, mt1, mt2, o;

	mt12 = 0;

	switch (coptype(o = p->n_op)) {
	case BITYPE:
		mt12=mt2 = moditype(p->n_right->n_type);
	case UTYPE:
		mt12 &= (mt1 = moditype(p->n_left->n_type));
		break;
	}

	switch( o ){

	case NAME :
	case STRING :
	case ICON :
	case FCON :
	case CALL :
	case UNARY CALL:
	case UNARY MUL:
		{  return( OTHER ); }
	case UNARY MINUS:
		if( mt1 & MDBI ) return( TYPL );
		break;

	case COMPL:
		if( mt1 & MINT ) return( TYPL );
		break;

	case UNARY AND:
		return( NCVT+OTHER );
	case NOT:
	case INIT:
	case CM:
	case CBRANCH:
	case ANDAND:
	case OROR:
		return( 0 );

	case MUL:
	case DIV:
		if ((mt1&MDBI) && (mt2&MENU)) return( TYMATCH );
		if ((mt2&MDBI) && (mt1&MENU)) return( TYMATCH );
		if( mt12 & MDBI ) return( TYMATCH );
		break;

	case MOD:
	case AND:
	case OR:
	case ER:
		if( mt12 & MINT ) return( TYMATCH );
		break;

	case LS:
	case RS:
		if( mt12 & MINT ) return( TYPL+OTHER );
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		if( (mt1&MENU)||(mt2&MENU) ) return( PTMATCH+PUN+NCVT );
		if( mt12 & MDBI ) return( TYMATCH+CVTO );
		else if( mt12 & MPTR ) return( PTMATCH+PUN );
		else if( mt12 & MPTI ) return( PTMATCH+PUN );
		else break;

	case QUEST:
	case COMOP:
		if( mt2&MENU ) return( TYPR+NCVTR );
		return( TYPR );

	case STREF:
		return( NCVTR+OTHER );

	case FORCE:
		return( TYPL );

	case COLON:
		if( mt12 & MENU ) return( NCVT+PUN+PTMATCH );
		else if( mt12 & MDBI ) return( TYMATCH );
		else if( mt12 & MPTR ) return( TYPL+PTMATCH+PUN );
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+PUN );
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+PUN );
		else if( mt12 & MSTR ) return( NCVT+TYPL+OTHER );
		break;

	case ASSIGN:
	case RETURN:
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+OTHER );
	case CAST:
		if( mt12 & MDBI ) return( TYPL+LVAL+TYMATCH );
		else if(mt1&MENU && mt2&MDBI) return( TYPL+LVAL+TYMATCH );
		else if(mt2&MENU && mt1&MDBI) return( TYPL+LVAL+TYMATCH );
		else if( (mt1&MENU)||(mt2&MENU) )
			return( LVAL+NCVT+TYPL+PTMATCH+PUN );
		else if( mt1 & MPTR) return( LVAL+PTMATCH+PUN );
		else if( mt12 & MPTI ) return( TYPL+LVAL+TYMATCH+PUN );
		break;

	case ASG LS:
	case ASG RS:
		if( mt12 & MINT ) return( TYPL+LVAL+OTHER );
		break;

	case ASG MUL:
	case ASG DIV:
		if( mt12 & MDBI ) return( LVAL+TYMATCH );
		break;

	case ASG MOD:
	case ASG AND:
	case ASG OR:
	case ASG ER:
		if (mt12 & MINT)
			return(LVAL+TYMATCH);
		break;

	case ASG PLUS:
	case ASG MINUS:
	case INCR:
	case DECR:
		if (mt12 & MDBI)
			return(TYMATCH+LVAL);
		else if ((mt1&MPTR) && (mt2&MINT))
			return(TYPL+LVAL+CVTR);
		break;

	case MINUS:
		if (mt12 & MPTR)
			return(CVTO+PTMATCH+PUN);
		if (mt2 & MPTR)
			break;
		/* FALLTHROUGH */
	case PLUS:
		if (mt12 & MDBI)
			return(TYMATCH);
		else if ((mt1&MPTR) && (mt2&MINT || mt2&MENU))
			return(TYPL+CVTR);
		else if ((mt1&MINT || mt1&MENU) && (mt2&MPTR))
			return(TYPR+CVTL);

	}
	uerror("operands of %s have incompatible types", copst(o));
	return(NCVT);
}

int
moditype(TWORD ty)
{
	switch (ty) {

	case ENUMTY:
	case MOETY:
		return( MENU );

	case STRTY:
	case UNIONTY:
		return( MSTR );

	case CHAR:
	case SHORT:
	case UCHAR:
	case USHORT:
	case UNSIGNED:
	case ULONG:
	case ULONGLONG:
	case INT:
	case LONG:
	case LONGLONG:
		return( MINT|MDBI|MPTI );
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
		return( MDBI );
	default:
		return( MPTR|MPTI );

	}
}

/*
 * Returns a TEMP node with temp number nr.
 * If nr == 0, return a node with a new number.
 */
NODE *
tempnode(int nr, TWORD type, union dimfun *df, struct suedef *sue)
{
	NODE *r;
	static int tvaloff;

	r = block(TEMP, NIL, NIL, type, df, sue);
	r->n_lval = nr == 0 ? ++tvaloff : nr;
	return r;
}

/*
 * Do sizeof on p.
 * XXX - add runtime evaluation sizeof.
 */
NODE *
doszof(NODE *p)
{
	int i;

	i = tsize( p->n_type, p->n_df, p->n_sue )/SZCHAR;

	tfree(p);
	if (i <= 0)
		werror( "sizeof returns 0" );
	return (bcon(i));
}

#ifdef PCC_DEBUG
int
eprint(NODE *p, int down, int *a, int *b)
{
	int ty;

	*a = *b = down+1;
	while( down > 1 ){
		printf( "\t" );
		down -= 2;
		}
	if( down ) printf( "    " );

	ty = coptype( p->n_op );

	printf("%p) %s, ", p, copst(p->n_op));
	if (ty == LTYPE) {
		printf(CONFMT, p->n_lval);
		printf(", %d, ", p->n_rval);
	}
	tprint( p->n_type, p->n_qual);
	printf( ", %p, %p\n", p->n_df, p->n_sue );
	return 0;
}
# endif

void
prtdcon(NODE *p)
{
	int o = p->n_op, i;

	if( o == FCON ){
		send_passt(IP_LOCCTR, DATA);
		defalign( p->n_type == DOUBLE ? ALDOUBLE : ALFLOAT );

		send_passt(IP_DEFLAB, i = getlab());
		fincode(p, p->n_type == DOUBLE ? SZDOUBLE : SZFLOAT);
		p->n_op = NAME;
		p->n_lval = 0;
		p->n_sp = tmpalloc(sizeof(struct symtab_hdr));
		p->n_sp->sclass = ILABEL;
		p->n_sp->soffset = i;
	}
}

extern int negrel[];

/*
 * Walk up through the tree from the leaves,
 * removing constant operators.
 */
static void
logwalk(NODE *p)
{
	int o = coptype(p->n_op);
	NODE *l, *r;

	l = p->n_left;
	r = p->n_right;
	switch (o) {
	case LTYPE:
		return;
	case BITYPE:
		logwalk(r);
	case UTYPE:
		logwalk(l);
	}
	if (!clogop(p->n_op))
		return;
	if (p->n_op == NOT && l->n_op == ICON) {
		p->n_lval = l->n_lval == 0;
		nfree(l);
		p->n_op = ICON;
	}
	if (l->n_op == ICON && r->n_op == ICON) {
		if (conval(l, p->n_op, r) == 0)
			cerror("logwalk");
		p->n_lval = l->n_lval;
		p->n_op = ICON;
		nfree(l);
		nfree(r);
	}
}

/*
 * Removes redundant logical operators for branch conditions.
 */
static void
fixbranch(NODE *p, int label)
{

	logwalk(p);

	if (p->n_op == ICON) {
		if (p->n_lval != 0)
			branch(label);
		nfree(p);
	} else {
		if (!clogop(p->n_op)) /* Always conditional */
			p = buildtree(NE, p, bcon(0));
		ecode(buildtree(CBRANCH, p, bcon(label)));
	}
}

/*
 * Write out logical expressions as branches.
 */
static void
andorbr(NODE *p, int true, int false)
{
	NODE *q;
	int o, lab;

	lab = -1;
	switch (o = p->n_op) { 
	case EQ:
	case NE:
		/*
		 * Remove redundant EQ/NE nodes.
		 */
		while (((o = p->n_left->n_op) == EQ || o == NE) && 
		    p->n_right->n_op == ICON) {
			o = p->n_op;
			q = p->n_left;
			if (p->n_right->n_lval == 0) {
				nfree(p->n_right);
				*p = *q;
				nfree(q);
				if (o == EQ)
					p->n_op = NE;
			} else if (p->n_right->n_lval == 1) {
				nfree(p->n_right);
				*p = *q;
				nfree(q);
				if (o == NE)
					p->n_op = EQ;
			} else
				break;
			
		}
		/* FALLTHROUGH */
	case LE:
	case LT:
	case GE:
	case GT:
calc:		if (true < 0) {
			p->n_op = negrel[p->n_op - EQ];
			true = false;
			false = -1;
		}

		rmcops(p->n_left);
		rmcops(p->n_right);
		fixbranch(p, true);
		if (false >= 0)
			branch(false);
		break;

	case ULE:
	case UGT:
		/* Convert to friendlier ops */
		if (p->n_right->n_op == ICON && p->n_right->n_lval == 0)
			p->n_op = o == ULE ? EQ : NE;
		goto calc;

	case UGE:
	case ULT:
		/* Already true/false by definition */
		if (p->n_right->n_op == ICON && p->n_right->n_lval == 0) {
			if (true < 0) {
				o = o == ULT ? UGE : ULT;
				true = false;
			}
			rmcops(p->n_left);
			ecode(p->n_left);
			rmcops(p->n_right);
			ecode(p->n_right);
			nfree(p);
			if (o == UGE) /* true */
				branch(true);
			break;
		}
		goto calc;

	case ANDAND:
		lab = false<0 ? getlab() : false ;
		andorbr(p->n_left, -1, lab);
		andorbr(p->n_right, true, false);
		if (false < 0)
			send_passt(IP_DEFLAB, lab);
		nfree(p);
		break;

	case OROR:
		lab = true<0 ? getlab() : true;
		andorbr(p->n_left, lab, -1);
		andorbr(p->n_right, true, false);
		if (true < 0)
			send_passt(IP_DEFLAB, lab);
		nfree(p);
		break;

	case NOT:
		andorbr(p->n_left, false, true);
		nfree(p);
		break;

	default:
		rmcops(p);
		if (true >= 0)
			fixbranch(p, true);
		if (false >= 0) {
			if (true >= 0)
				branch(false);
			else
				fixbranch(buildtree(EQ, p, bcon(0)), false);
		}
	}
}

/*
 * Massage the output trees to remove C-specific nodes:
 *	COMOPs are split into separate statements.
 *	QUEST/COLON are rewritten to branches.
 *	ANDAND/OROR/NOT are rewritten to branches for lazy-evaluation.
 *	CBRANCH conditions are rewritten for lazy-evaluation.
 */
static void
rmcops(NODE *p)
{
	TWORD type;
	NODE *q, *r;
	int o, ty, lbl, lbl2, tval = 0;

again:
	o = p->n_op;
	ty = coptype(o);
	switch (o) {
	case QUEST:

		/*
		 * Create a branch node from ?:
		 * || and && must be taken special care of.
		 */
		type = p->n_type;
		andorbr(p->n_left, -1, lbl = getlab());

		/* Make ASSIGN node */
		/* Only if type is not void */
		q = p->n_right->n_left;
		if (type != VOID) {
			r = tempnode(0, q->n_type, q->n_df, q->n_sue);
			tval = r->n_lval;
			q = buildtree(ASSIGN, r, q);
		}
		rmcops(q);
		ecode(q); /* Done with assign */
		branch(lbl2 = getlab());
		send_passt(IP_DEFLAB, lbl);

		q = p->n_right->n_right;
		if (type != VOID) {
			r = tempnode(tval, q->n_type, q->n_df, q->n_sue);
			q = buildtree(ASSIGN, r, q);
		}
		rmcops(q);
		ecode(q); /* Done with assign */

		send_passt(IP_DEFLAB, lbl2);

		nfree(p->n_right);
		p->n_op = p->n_type == VOID ? ICON : TEMP;
		p->n_lval = tval;
		break;

	case ULE:
	case ULT:
	case UGE:
	case UGT:
	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
	case ANDAND:
	case OROR:
	case NOT:
#ifdef SPECIAL_CCODES
#error fix for private CCODES handling
#else
		r = talloc();
		*r = *p;
		andorbr(r, -1, lbl = getlab());
		q = tempnode(0, p->n_type, p->n_df, p->n_sue);
		tval = q->n_lval;
		r = tempnode(tval, p->n_type, p->n_df, p->n_sue);
		ecode(buildtree(ASSIGN, q, bcon(1)));
		branch(lbl2 = getlab());
		send_passt(IP_DEFLAB, lbl);
		ecode(buildtree(ASSIGN, r, bcon(0)));
		send_passt(IP_DEFLAB, lbl2);
		p->n_op = TEMP;
		p->n_lval = tval;
#endif
		break;
	case CBRANCH:
		andorbr(p->n_left, p->n_right->n_lval, -1);
		nfree(p->n_right);
		p->n_op = ICON; p->n_type = VOID;
		break;
	case COMOP:
		rmcops(p->n_left);
		ecode(p->n_left);
		/* Now when left tree is dealt with, rm COMOP */
		q = p->n_right;
		*p = *p->n_right;
		nfree(q);
		goto again;

	default:
		if (ty == LTYPE)
			return;
		rmcops(p->n_left);
		if (ty == BITYPE)
			rmcops(p->n_right);
       }
}

/*
 * Return 1 if an assignment is found.
 */
static int
has_se(NODE *p)
{
	if (cdope(p->n_op) & ASGFLG)
		return 1;
	if (coptype(p->n_op) == LTYPE)
		return 0;
	if (has_se(p->n_left))
		return 1;
	if (coptype(p->n_op) == BITYPE)
		return has_se(p->n_right);
	return 0;
}

/*
 * Find and convert asgop's to separate statements.
 * Be careful about side effects.
 * assign tells whether ASSIGN should be considered giving
 * side effects or not.
 */
static void
delasgop(NODE *p)
{
	NODE *q, *r;

	if ((cdope(p->n_op)&ASGOPFLG) && p->n_op != RETURN && p->n_op != CAST) {
		NODE *l = p->n_left;
		NODE *ll = l->n_left;

		if (has_se(l)) {
			q = tempnode(0, ll->n_type, ll->n_df, ll->n_sue);
			r = tempnode(q->n_lval, ll->n_type, ll->n_df,ll->n_sue);
			l->n_left = q;
			/* Now the left side of node p has no side effects. */
			/* side effects on the right side must be obeyed */
			delasgop(p);
			
			r = buildtree(ASSIGN, r, ll);
			delasgop(r);
			ecode(r);
		} else {
			p->n_right = buildtree(NOASG p->n_op, ccopy(l),
			    p->n_right);
			p->n_op = ASSIGN;
			delasgop(p->n_right);
		}
	} else {
		if (coptype(p->n_op) == LTYPE)
			return;
		delasgop(p->n_left);
		if (coptype(p->n_op) == BITYPE)
			delasgop(p->n_right);
	}
}

int edebug = 0;
void
ecomp(NODE *p)
{

#ifdef PCC_DEBUG
	if (edebug)
		fwalk(p, eprint, 0);
#endif
	if (!reached) {
		werror("statement not reached");
		reached = 1;
	}
	p = optim(p);
	rmcops(p);
	delasgop(p);
	send_passt(IP_LOCCTR, PROG);
	if (p->n_op == ICON && p->n_type == VOID)
		tfree(p);
	else
		ecode(p);
}

#if defined(MULTIPASS)
void	
p2tree(NODE *p)
{
	struct symtab *q;
	int ty;

# ifdef MYP2TREE
	MYP2TREE(p);  /* local action can be taken here; then return... */
# endif

	ty = coptype(p->n_op);

	printf("%d\t", p->n_op);

	if (ty == LTYPE) {
		printf(CONFMT, p->n_lval);
		printf("\t");
	}
	if (ty != BITYPE) {
		if (p->n_op == NAME || p->n_op == ICON)
			printf("0\t");
		else
			printf("%d\t", p->n_rval);
		}

	printf("%o\t", p->n_type);

	/* handle special cases */

	switch (p->n_op) {

	case NAME:
	case ICON:
		/* print external name */
		if (p->n_sp != NULL) {
			if (p->n_sp->sflags & SLABEL ||
			    p->n_sp->sclass == ILABEL) {
				printf(LABFMT, p->n_sp->soffset);
			} else
				printf("%s\n", exname(q->sname));
		} else
			printf("\n");
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* print out size */
		/* use lhs size, in order to avoid hassles
		 * with the structure `.' operator
		 */

		/* note: p->left not a field... */
		printf(CONFMT, (CONSZ)tsize(STRTY, p->n_left->n_df,
		    p->n_left->n_sue));
		printf("\t%d\t\n", talign(STRTY, p->n_left->n_sue));
		break;

	default:
		printf(	 "\n" );
	}

	if (ty != LTYPE)
		p2tree(p->n_left);
	if (ty == BITYPE)
		p2tree(p->n_right);
}
#else
void
p2tree(NODE *p)
{
	int ty;

# ifdef MYP2TREE
	MYP2TREE(p);  /* local action can be taken here; then return... */
# endif

	ty = coptype(p->n_op);

	switch( p->n_op ){

	case NAME:
	case ICON:
		if (p->n_sp != NULL) {
			if (p->n_sp->sflags & SLABEL ||
			    p->n_sp->sclass == ILABEL) {
				char *cp = (isinlining ?
				    permalloc(32) : tmpalloc(32));
				sprintf(cp, LABFMT, p->n_sp->soffset);
				p->n_name = cp;
			} else {
				p->n_name = exname(p->n_sp->sname);
			}
		} else
			p->n_name = "";
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* set up size parameters */
		p->n_stsize = (tsize(STRTY,p->n_left->n_df,p->n_left->n_sue)+SZCHAR-1)/SZCHAR;
		p->n_stalign = talign(STRTY,p->n_left->n_sue)/SZCHAR;
		break;

	case REG:
		rbusy( p->n_rval, p->n_type );
	default:
		p->n_name = "";
		}

	p->n_rall = NOPREF;

	if( ty != LTYPE ) p2tree( p->n_left );
	if( ty == BITYPE ) p2tree( p->n_right );
	}

#endif

void
ecode(NODE *p)	
{
	/* walk the tree and write out the nodes.. */

	if (nerrors)	
		return;

	p = optim(p);
	delasgop(p);
	walkf(p, prtdcon);
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("Fulltree:\n"); 
		fwalk(p, eprint, 0); 
	}
#endif
	p2tree(p);
#if !defined(MULTIPASS)
	send_passt(IP_NODE, p);
#endif
}

/*
 * Send something further on to the next pass.
 */
void
send_passt(int type, ...)
{
	struct interpass *ip;
	va_list ap;
	int nloc;

	va_start(ap, type);
	if (type == IP_LOCCTR) {
		nloc = va_arg(ap, int);
		if (nloc == lastloc)
			return;
	}
	ip = isinlining ? permalloc(sizeof(*ip)) : tmpalloc(sizeof(*ip));
	ip->type = type;
	switch (type) {
	case IP_NODE:
		ip->ip_node = va_arg(ap, NODE *);
		break;
	case IP_PROLOG:
	case IP_NEWBLK:
	case IP_EPILOG:
		ip->ip_regs = va_arg(ap, int);
		ip->ip_auto = va_arg(ap, int);
		ip->ip_retl = va_arg(ap, int);
		break;
	case IP_LOCCTR:
		ip->ip_locc = nloc;
		lastloc = ip->ip_locc;
		break;
	case IP_DEFLAB:
		ip->ip_lbl = va_arg(ap, int);
		break;
	case IP_DEFNAM:
		ip->ip_name = va_arg(ap, char *);
		ip->ip_vis = va_arg(ap, int);
		break;
	case IP_ASM:
		ip->ip_asm = va_arg(ap, char *);
		break;
	default:
		cerror("bad send_passt type %d", type);
	}
	va_end(ap);
	if (isinlining)
		inline_addarg(ip);
	else
		pass2_compile(ip);
}

char *
copst(int op)
{
	if (op <= MAXOP)
		return opst[op];
#define	SNAM(x,y) case x: return #y;
	switch (op) {
	SNAM(QUALIFIER,QUALIFIER)
	SNAM(CLASS,CLASS)
	SNAM(RB,])
	SNAM(DOT,.)
	SNAM(ELLIPSIS,...)
	SNAM(LB,[)
	SNAM(TYPE,TYPE)
	SNAM(COMOP,COMOP)
	SNAM(QUEST,?)
	SNAM(COLON,:)
	SNAM(ANDAND,&&)
	SNAM(OROR,||)
	SNAM(NOT,!)
	default:
		cerror("bad copst %d", op);
	}
	return 0; /* XXX gcc */
}

int
cdope(int op)
{
	if (op <= MAXOP)
		return dope[op];
	switch (op) {
	case QUALIFIER:
	case CLASS:
	case RB:
	case DOT:
	case ELLIPSIS:
	case TYPE:
		return LTYPE;
	case COMOP:
	case QUEST:
	case COLON:
	case LB:
		return BITYPE;
	case ANDAND:
	case OROR:
		return BITYPE|LOGFLG;
	case NOT:
		return UTYPE|LOGFLG;
	}
	return 0; /* XXX gcc */
}

/* 
 * make a fresh copy of p
 */
NODE *
ccopy(NODE *p) 
{  
	NODE *q;

	q = talloc();
	*q = *p;

	switch (coptype(q->n_op)) {
	case BITYPE:
		q->n_right = ccopy(p->n_right);
	case UTYPE: 
		q->n_left = ccopy(p->n_left);
	}

	return(q);
}

