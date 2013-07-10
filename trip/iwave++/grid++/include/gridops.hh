#ifndef __TSOPT_GRIDPP_OPS__
#define __TSOPT_GRIDPP_OPS__

#include "rn.hh"
#include "op.hh"
#include "productspace.hh"
#include "mpiserialfo.hh"

#ifdef IWAVE_USE_MPI
#include "mpigridpp.hh"
#else
#include "gridpp.hh"
#endif

using RVL::ScalarFieldTraits;
using RVL::SpaceTest;
using RVL::Operator;
using RVL::LinearOp;
using RVL::Space;
using RVL::ProductSpace;
using RVL::Vector;
using RVL::Components;
using RVL::ProtectedDivision;
using RVL::RnArray;
using RVL::RVLScale;

/** piecewise linear taper function. no sanity check - calling unit
    responsible for assuring that w > 0 */
template<typename Scalar>
inline Scalar window(Scalar a, Scalar b, Scalar w, Scalar x) {
  return min(min(ScalarFieldTraits<Scalar>::One(),max(ScalarFieldTraits<Scalar>::Zero(),(x-a)/w)),min(ScalarFieldTraits<Scalar>::One(),max(ScalarFieldTraits<Scalar>::Zero(),(b-x)/w)));
}

namespace TSOpt {

  using RVL::BinaryLocalFunctionObject;
  using RVL::RVLException;
  using RVL::ContentPackage;
  using RVL::LocalDataContainer;

  /** function object for tapering a 1D grid function, in the form of
      a CP with Grid metadata. Scalar arrays ffset between input and
      output grids computed by calling unit
  */

  class GridWindowFO: public BinaryLocalFunctionObject<ireal> {

  private:

    IPNT iw;     // window width in gridpoints
    bool bias;   // add windowed values if set, else overwrite
    
    GridWindowFO();
    
  public:
    
    GridWindowFO(IPNT const & _iw, bool _bias=false) 
      : bias(_bias) { 
      IASN(iw,_iw);
    }

    GridWindowFO(GridWindowFO const & f) 
      : bias(f.bias) {
      IASN(iw,f.iw);
    }

    using RVL::LocalEvaluation<ireal>::operator();
    void operator()(LocalDataContainer<ireal> & x,
		    LocalDataContainer<ireal> const & y);
    
    string getName() const { string tmp = "GridWindoFO"; return tmp; }
    
  };

  /** Affine window operator for grid objects. Apply method outputs 
      windowed increment of background Vector data member: thus 

      \f$ y = x_{bg} + \phi x\f$

      Derivative and adjoint derivative are independent of
      \f$x_{bg}\f$ and implement standard linear injection and
      projection operators.
  */
  class GridWindowOp: public Operator<ireal> {

  private:

    Vector<ireal> const & bg;
    Space<ireal> const & dom;
    IPNT iw;
    GridWindowOp();

  protected:

    void apply(Vector<ireal> const & x,
	       Vector<ireal> & y) const;
    void applyDeriv(Vector<ireal> const & x,
		    Vector<ireal> const & dx,
		    Vector<ireal> & dy) const;
    void applyAdjDeriv(Vector<ireal> const & x,
		       Vector<ireal> const & dy,
		       Vector<ireal> & dx) const;

    Operator<ireal> * clone() const { return new GridWindowOp(*this); }
    
  public:
    
    /** main constructor: takes Grid defining increment window, and width
	parameter defining zone of smooth taper - same on all sides. 
    */
    GridWindowOp(Space<ireal> const & _dom,
		 Vector<ireal> const & _bg,
      		 RPNT const & w = RPNT_0);

    /** Copy constructor - memberwise */
    GridWindowOp(GridWindowOp const & op) 
    : dom(op.dom), bg(op.bg) {
      IASN(iw,op.iw);
    }

    ~GridWindowOp() {}
    
    Space<ireal> const & getDomain() const { return dom; }
    Space<ireal> const & getRange() const { return bg.getSpace(); }

    ostream & write(ostream & str) const;
  };

  class GridFwdDerivFO: public BinaryLocalFunctionObject<ireal> {

  private:

    ireal fac; 
    int dir;

    GridFwdDerivFO();

  public:
    GridFwdDerivFO(int _dir, ireal _fac)
      : dir(_dir), fac(_fac) {}
    GridFwdDerivFO(GridFwdDerivFO const & a)
    : dir(a.dir), fac(a.fac) {}
    
    using RVL::LocalEvaluation<ireal>::operator();
    void operator()(LocalDataContainer<ireal> & x,
		    LocalDataContainer<ireal> const & y);
    
    string getName() const { string tmp = "GridFwdDerivFO"; return tmp; }
  };

  class GridAdjDerivFO: public BinaryLocalFunctionObject<ireal> {

  private:
    ireal fac; 
    int dir;

    GridAdjDerivFO();

  public:
    GridAdjDerivFO(int _dir, ireal _fac)
      : dir(_dir), fac(_fac) {}
    GridAdjDerivFO(GridAdjDerivFO const & a)
    : dir(a.dir), fac(a.fac) {}
    
    using RVL::LocalEvaluation<ireal>::operator();
    void operator()(LocalDataContainer<ireal> & x,
		    LocalDataContainer<ireal> const & y);
    
    string getName() const { string tmp = "GridAdjDerivFO"; return tmp; }
  };

  class GridDerivOp: public LinearOp<ireal> {
    
  private:
    
    int dir;
    std::vector<ireal> fac;
    Space<ireal> const & dom;

    GridDerivOp();

  protected:

    void apply(Vector<ireal> const & x,
	       Vector<ireal> & y) const;
    void applyAdj(Vector<ireal> const & x,
		  Vector<ireal> & y) const;

    Operator<ireal> * clone() const { return new GridDerivOp(*this); }
    
  public:
    
    /** main constructor: takes Grid defining increment window, and width
	parameter defining zone of smooth taper - same on all sides. 
    */
    GridDerivOp(Space<ireal> const & _dom, int _dir, ireal scale = REAL_ONE);

    /** Copy constructor - memberwise */
    GridDerivOp(GridDerivOp const & op);

    ~GridDerivOp() {}
    
    Space<ireal> const & getDomain() const { return dom; }
    Space<ireal> const & getRange() const { return dom; }

    ostream & write(ostream & str) const;
  };	
}

#endif
