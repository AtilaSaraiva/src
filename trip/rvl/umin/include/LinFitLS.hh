#ifndef __RVLALG_LINFIT_L2_H
#define __RVLALG_LINFIT_L2_H

/** Given an Operator F and a Vector d in the range of op,
    implements the function
    \f$$
    f(x) = \inf_{dx} \|DF(x)dx - d\|^2
    \f$$
    as an RVL::Functional. The linear least squares solver is specified by 
    policy.
*/

#include "alg.hh"
#include "terminator.hh"
#include "linop.hh"
#include "table.hh"

using namespace RVLAlg;

namespace RVLUmin {

  using namespace RVL;
  using namespace RVLAlg;    

  template<typename Scalar, typename LSPolicy, typename LSPolicyData> 
  class LinFitLS: public Functional<Scalar>, public LSPolicy {
    
    typedef typename ScalarFieldTraits<Scalar>::AbsType atype;

  private:
    
    Operator<Scalar> const & op;        // operator  
    LinearOp<Scalar> const & preop;     // preconditioner
    Vector<Scalar> const & d;           // data 
    mutable  Vector<Scalar> dx;         // intermediate data
    mutable bool applied;
    ostream & str;
    
  protected:

    void apply(const Vector<Scalar> & x, 
	       Scalar & val) const {
      try {
 /*         if (applied) {
              RVLException e;
              e<<"Error: LinFitLS::apply(x,val)\n";
              e<<"already applied, may not alter\n";
              throw e;
          }
  */
	atype rnorm;
	atype nrnorm;
    // access Operator through OperatorEvaluation
	OperatorEvaluation<Scalar> opeval(op,x);
    // Get Derivative of Operator
	LinearOp<Scalar> const & lop = opeval.getDeriv();
    // Composition of lop and preop
	OpComp<Scalar> gop(preop,lop);
          
	Vector<Scalar> x0(gop.getDomain());
	x0.zero();
	dx.zero();
    // build least square solver , solve for dx
	OperatorEvaluation<Scalar> gopeval(gop,x0);
	Algorithm * solver = LSPolicy::build(dx,gopeval.getDeriv(),d,rnorm,nrnorm,str);
    solver->run();
    // get the value of objective function
    val = 0.5*rnorm*rnorm;

    applied = true;
	delete solver;
      }
      catch (RVLException & e) {
	e<<"\ncalled from LinFitLS::apply\n";
	throw e;
      }
    } 

    void applyGradient(const Vector<Scalar> & x, 
		       Vector<Scalar> & g) const {
        try{
        if(!applied){
            Scalar val;
            this->apply(x,val);
        }
        OperatorEvaluation<Scalar> opeval(op,x);
        LinearOp<Scalar> const & lop = opeval.getDeriv();
        SymmetricBilinearOp<Scalar> const & sblop = opeval.getDeriv2();
            
        Vector<Scalar> dltd(lop.getRange());
        Vector<Scalar> dltx(preop.getRange());
        // compute dltx and dltd = DF * dltx - d
        preop.applyOp(dx,dltx);
        lop.applyOp(dltx,dltd);
        dltd.linComb(-1.0,d);
        // naive computation of gradient
        sblop.applyAdjOp(dltx,dltd,g);
            
        // compute and add correction term to gradient
        atype rnorm;
        atype nrnorm;
        OpComp<Scalar> gop(preop,lop);
        Vector<Scalar> x0(gop.getDomain());
        x0.zero();
        dx.zero();
        OperatorEvaluation<Scalar> gopeval(gop,x0);
        // solve DF * dx = dltd in LS sense 
        Algorithm * solver = LSPolicy::build(dx,gopeval.getDeriv(),dltd,rnorm,nrnorm,str);
        solver->run();
            
        Vector<Scalar> tmp(g.getSpace());
        Vector<Scalar> dx2(preop.getRange());
        preop.applyOp(dx,dx2);
        // compute and add correction term tmp to gradient g
        sblop.applyAdjOp(dx2,d,tmp);
        g.linComb(1.0, tmp);
            
        delete solver;
        }
        catch (RVLException & e) {
            e<<"\ncalled from LinFitLS::applyGradient\n";
            throw e;
        }
        
    }

    void applyHessian(const Vector<Scalar> & x,
		      const Vector<Scalar> & dx, 
		      Vector<Scalar> & dy) const {}

    Functional<Scalar> * clone() const {
      return new LinFitLS<Scalar,LSPolicy,LSPolicyData>(*this);
    }

  public:

    /* typical policy data 
	       atype _rtol,
	       atype _nrtol,
	       int _maxcount,
    */
    LinFitLS(Operator<Scalar> const & _op,
	     LinearOp<Scalar> const & _preop,
	     Vector<Scalar> const & _d,
	     LSPolicyData const & s,
	     ostream & _str)
	: LSPolicy(), op(_op), preop(_preop), d(_d), dx(preop.getDomain()), applied(false), str(_str) {
          try{
      dx.zero();
      LSPolicy::assign(s);
      if (s.verbose) {
	str<<"\n";
	str<<"==============================================\n";
	str<<"LinFitLS constructor - ls policy data = \n";
	s.write(str);
      }}
          catch (RVLException & e) {
              e<<"\ncalled from LinFitLS::Constructor\n";
              throw e;
          }
    }

    LinFitLS(LinFitLS<Scalar,LSPolicy,LSPolicyData> const & f) 
	: LSPolicy(f), op(f.op), preop(f.preop), d(f.d), dx(f.dx), str(f.str) {}

    const Space<Scalar> & getDomain() const { return op.getDomain(); }

    Scalar getMaxStep(const Vector<Scalar> & x,
		      const Vector<Scalar> & dx) const {
      try {
	return op.getMaxStep(x,dx);
      }
      catch (RVLException & e) {
	e<<"\ncalled from LinFitLS::getMaxStep\n";
	throw e;
      }
    }

    ostream & write(ostream & str) const {
      str<<"LinFitLS: \n";
      str<<"*** operator:\n";
      op.write(str);
      str<<"*** data vector:\n";
      d.write(str);
      return str;
    }
  };
}
#endif
