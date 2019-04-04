/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file   SubgraphSolver.cpp
 * @brief  Subgraph Solver from IROS 2010
 * @date   2010
 * @author Frank Dellaert
 * @author Yong Dian Jian
 */

#include <gtsam/linear/SubgraphSolver.h>
#include <gtsam/linear/GaussianBayesNet.h>
#include <gtsam/linear/iterative-inl.h>
#include <gtsam/linear/GaussianFactorGraph.h>
#include <gtsam/linear/SubgraphPreconditioner.h>
#include <gtsam_unstable/base/DSFMap.h>

#include <iostream>

using namespace std;

namespace gtsam {

/**************************************************************************************************/
SubgraphSolver::SubgraphSolver(const GaussianFactorGraph &gfg,
    const Parameters &parameters, const Ordering& ordering) :
    parameters_(parameters), ordering_(ordering) {
  initialize(gfg);
}

/**************************************************************************************************/
SubgraphSolver::SubgraphSolver(const GaussianFactorGraph::shared_ptr &jfg,
    const Parameters &parameters, const Ordering& ordering) :
    parameters_(parameters), ordering_(ordering) {
  initialize(*jfg);
}

/**************************************************************************************************/
SubgraphSolver::SubgraphSolver(const GaussianFactorGraph &Ab1,
    const GaussianFactorGraph &Ab2, const Parameters &parameters,
    const Ordering& ordering) :
    parameters_(parameters), ordering_(ordering) {

  GaussianBayesNet::shared_ptr Rc1 = Ab1.eliminateSequential(ordering_,
      EliminateQR);
  initialize(Rc1, boost::make_shared<GaussianFactorGraph>(Ab2));
}

/**************************************************************************************************/
SubgraphSolver::SubgraphSolver(const GaussianFactorGraph::shared_ptr &Ab1,
    const GaussianFactorGraph::shared_ptr &Ab2, const Parameters &parameters,
    const Ordering& ordering) :
    parameters_(parameters), ordering_(ordering) {

  GaussianBayesNet::shared_ptr Rc1 = Ab1->eliminateSequential(ordering_,
      EliminateQR);
  initialize(Rc1, Ab2);
}

/**************************************************************************************************/
SubgraphSolver::SubgraphSolver(const GaussianBayesNet::shared_ptr &Rc1,
    const GaussianFactorGraph &Ab2, const Parameters &parameters,
    const Ordering& ordering) :
    parameters_(parameters), ordering_(ordering) {
  initialize(Rc1, boost::make_shared<GaussianFactorGraph>(Ab2));
}

/**************************************************************************************************/
SubgraphSolver::SubgraphSolver(const GaussianBayesNet::shared_ptr &Rc1,
    const GaussianFactorGraph::shared_ptr &Ab2, const Parameters &parameters,
    const Ordering& ordering) :
    parameters_(parameters), ordering_(ordering) {
  initialize(Rc1, Ab2);
}

/**************************************************************************************************/
VectorValues SubgraphSolver::optimize() {
  VectorValues ybar = conjugateGradients<SubgraphPreconditioner, VectorValues,
      Errors>(*pc_, pc_->zero(), parameters_);
  return pc_->x(ybar);
}

/**************************************************************************************************/
VectorValues SubgraphSolver::optimize(const VectorValues &initial) {
  // the initial is ignored in this case ...
  return optimize();
}

/**************************************************************************************************/
void SubgraphSolver::initialize(const GaussianFactorGraph &jfg) {
  GaussianFactorGraph::shared_ptr Ab1 =
      boost::make_shared<GaussianFactorGraph>(), Ab2 = boost::make_shared<
      GaussianFactorGraph>();

  boost::tie(Ab1, Ab2) = splitGraph(jfg);
  if (parameters_.verbosity())
    cout << "Split A into (A1) " << Ab1->size() << " and (A2) " << Ab2->size()
        << " factors" << endl;

  GaussianBayesNet::shared_ptr Rc1 = Ab1->eliminateSequential(ordering_,
      EliminateQR);
  VectorValues::shared_ptr xbar = boost::make_shared<VectorValues>(
      Rc1->optimize());
  pc_ = boost::make_shared<SubgraphPreconditioner>(Ab2, Rc1, xbar);
}

/**************************************************************************************************/
void SubgraphSolver::initialize(const GaussianBayesNet::shared_ptr &Rc1,
    const GaussianFactorGraph::shared_ptr &Ab2) {
  VectorValues::shared_ptr xbar = boost::make_shared<VectorValues>(
      Rc1->optimize());
  pc_ = boost::make_shared<SubgraphPreconditioner>(Ab2, Rc1, xbar);
}

/**************************************************************************************************/
// Run Kruskal algorithm to create a spanning tree of factor "edges".
// Edges are not weighted, and will only work if factors are binary.
// Unary factors are ignored for this purpose and added to tree graph.
boost::tuple<GaussianFactorGraph::shared_ptr, GaussianFactorGraph::shared_ptr> //
SubgraphSolver::splitGraph(const GaussianFactorGraph &factorGraph) {

  // Create disjoint set forest data structure for Kruskal algorithm
  DSFMap<Key> dsf;

  // Allocate two output graphs
  auto tree = boost::make_shared<GaussianFactorGraph>();
  auto constraints = boost::make_shared<GaussianFactorGraph>();

  // Loop over all "edges"
  for ( const auto &factor: factorGraph ) {

    // Fail on > binary factors
    const auto& keys = factor->keys();
    if (keys.size() > 2) {
      throw runtime_error(
          "SubgraphSolver::splitGraph the graph is not simple, sanity check failed ");
    }

    // check whether this factor should be augmented to the "tree" graph
    if (keys.size() == 1)
      tree->push_back(factor);
    else if (dsf.find(keys[0]) != dsf.find(keys[1])) {
      // We merge two trees joined by this edge if they are still disjoint
      tree->push_back(factor);
      // Record this fact in DSF
      dsf.merge(keys[0], keys[1]);
    } else {
      // This factor would create a loop, so we add it to non-tree edges...
      constraints->push_back(factor);
    }
  }

  return boost::tie(tree, constraints);
}

/****************************************************************************/
VectorValues SubgraphSolver::optimize(const GaussianFactorGraph &gfg,
    const KeyInfo &keyInfo, const std::map<Key, Vector> &lambda,
    const VectorValues &initial) {
  return VectorValues();
}
} // \namespace gtsam
