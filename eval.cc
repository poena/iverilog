/*
 * Copyright (c) 1998-1999 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT) && !defined(macintosh)
#ident "$Id: eval.cc,v 1.23 2001/11/07 04:01:59 steve Exp $"
#endif

# include "config.h"

# include  <iostream>

# include  "PExpr.h"
# include  "netlist.h"
# include  "compiler.h"

verinum* PExpr::eval_const(const Design*, const NetScope*) const
{
      return 0;
}

verinum* PEBinary::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*l = left_->eval_const(des, scope);
      if (l == 0) return 0;
      verinum*r = right_->eval_const(des, scope);
      if (r == 0) {
	    delete l;
	    return 0;
      }
      verinum*res;

      switch (op_) {
	  case '+': {
		assert(l->is_defined());
		assert(r->is_defined());
		long lv = l->as_long();
		long rv = r->as_long();
		res = new verinum(lv+rv, l->len());
		break;
	  }
	  case '-': {
		assert(l->is_defined());
		assert(r->is_defined());
		long lv = l->as_long();
		long rv = r->as_long();
		res = new verinum(lv-rv, l->len());
		break;
	  }
	  case '*': {
		assert(l->is_defined());
		assert(r->is_defined());
		long lv = l->as_long();
		long rv = r->as_long();
		res = new verinum(lv * rv, l->len());
		break;
	  }
	  case '/': {
		assert(l->is_defined());
		assert(r->is_defined());
		long lv = l->as_long();
		long rv = r->as_long();
		res = new verinum(lv / rv, l->len());
		break;
	  }
	  case '%': {
		assert(l->is_defined());
		assert(r->is_defined());
		long lv = l->as_long();
		long rv = r->as_long();
		res = new verinum(lv % rv, l->len());
		break;
	  }
	  case 'l': {
		assert(r->is_defined());
		unsigned long rv = r->as_ulong();
		res = new verinum(verinum::V0, l->len());
		if (rv < res->len()) {
		      unsigned cnt = res->len() - rv;
		      for (unsigned idx = 0 ;  idx < cnt ;  idx += 1)
			    res->set(idx+rv, l->get(idx));
		}
		break;
	  }

	  default:
	    delete l;
	    delete r;
	    return 0;
      }

      delete l;
      delete r;
      return res;
}


/*
 * Evaluate an identifier as a constant expression. This is only
 * possible if the identifier is that of a parameter.
 */
verinum* PEIdent::eval_const(const Design*des, const NetScope*scope) const
{
      assert(scope);
      const NetExpr*expr = des->find_parameter(scope, text_);

      if (expr == 0)
	    return 0;

      assert(msb_ == 0);

      if (dynamic_cast<const NetEParam*>(expr)) {
	    cerr << get_line() << ": sorry: I cannot evaluate ``" <<
		  text_ << "'' in this scope: " << scope->name() << endl;
	    return 0;
      }

      const NetEConst*eval = dynamic_cast<const NetEConst*>(expr);
      assert(eval);
      return new verinum(eval->value());
}

verinum* PEFNumber::eval_const(const Design*, const NetScope*) const
{
      long val = value_->as_long();
      return new verinum(val);
}

verinum* PENumber::eval_const(const Design*, const NetScope*) const
{
      return new verinum(value());
}

verinum* PETernary::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*test = expr_->eval_const(des, scope);
      if (test == 0)
	    return 0;

      verinum::V bit = test->get(0);
      delete test;
      switch (bit) {
	  case verinum::V0:
	    return fal_->eval_const(des, scope);
	  case verinum::V1:
	    return tru_->eval_const(des, scope);
	  default:
	    return 0;
	      // XXXX It is possible to handle this case if both fal_
	      // and tru_ are constant. Someday...
      }
}

verinum* PEUnary::eval_const(const Design*des, const NetScope*scope) const
{
      verinum*val = expr_->eval_const(des, scope);
      if (val == 0)
	    return 0;

      switch (op_) {
	  case '+':
	    return val;

	  case '-': {
		  /* We need to expand the value a bit if we are
		     taking the 2's complement so that we are
		     guaranteed to not overflow. */
		verinum tmp (0UL, val->len()+1);
		for (unsigned idx = 0 ;  idx < val->len() ;  idx += 1)
		      tmp.set(idx, val->get(idx));

		*val = v_not(tmp) + verinum(verinum::V1, 1);
		val->has_sign(true);
		return val;
	  }

      }
	    delete val;
      return 0;
}


/*
 * $Log: eval.cc,v $
 * Revision 1.23  2001/11/07 04:01:59  steve
 *  eval_const uses scope instead of a string path.
 *
 * Revision 1.22  2001/11/06 06:11:55  steve
 *  Support more real arithmetic in delay constants.
 *
 * Revision 1.21  2001/07/25 03:10:49  steve
 *  Create a config.h.in file to hold all the config
 *  junk, and support gcc 3.0. (Stephan Boettcher)
 *
 * Revision 1.20  2001/02/09 02:49:59  steve
 *  Be more clear about scope of failure.
 *
 * Revision 1.19  2001/01/27 05:41:48  steve
 *  Fix sign extension of evaluated constants. (PR#91)
 *
 * Revision 1.18  2001/01/14 23:04:56  steve
 *  Generalize the evaluation of floating point delays, and
 *  get it working with delay assignment statements.
 *
 *  Allow parameters to be referenced by hierarchical name.
 *
 * Revision 1.17  2001/01/04 04:47:51  steve
 *  Add support for << is signal indices.
 *
 * Revision 1.16  2000/12/10 22:01:36  steve
 *  Support decimal constants in behavioral delays.
 *
 * Revision 1.15  2000/09/07 22:38:13  steve
 *  Support unary + and - in constants.
 */

