/*
 * AllDiff.h
 * @brief General "all-different" constraint
 * @date Feb 6, 2012
 * @author Frank Dellaert
 */

#pragma once

#include <gtsam_unstable/discrete/BinaryAllDiff.h>
#include <gtsam/discrete/DiscreteKey.h>

namespace gtsam {

  /**
   * General AllDiff constraint.
   * Returns 1 if values for all keys are different, 0 otherwise.
   */
  class GTSAM_UNSTABLE_EXPORT AllDiff: public Constraint {

    std::map<Key,size_t> cardinalities_;

    DiscreteKey discreteKey(size_t i) const {
      Key j = keys_[i];
      return DiscreteKey(j,cardinalities_.at(j));
    }

  public:

    /// Construct from keys.
    AllDiff(const DiscreteKeys& dkeys);

    // print
    void print(const std::string& s = "",
        const KeyFormatter& formatter = DefaultKeyFormatter) const override;

    /// equals
    bool equals(const DiscreteFactor& other, double tol) const override {
      if(!dynamic_cast<const AllDiff*>(&other))
        return false;
      else {
        const AllDiff& f(static_cast<const AllDiff&>(other));
        return cardinalities_.size() == f.cardinalities_.size()
            && std::equal(cardinalities_.begin(), cardinalities_.end(),
                          f.cardinalities_.begin());
      }
    }

    /// Calculate value = expensive !
    double operator()(const Values& values) const override;

    /// Convert into a decisiontree, can be *very* expensive !
    DecisionTreeFactor toDecisionTreeFactor() const override;

    /// Multiply into a decisiontree
    DecisionTreeFactor operator*(const DecisionTreeFactor& f) const override;

    /*
     * Ensure Arc-consistency
     * Arc-consistency involves creating binaryAllDiff constraints
     * In which case the combinatorial hyper-arc explosion disappears.
     * @param j domain to be checked
     * @param (in/out) domains all other domains
     */
    bool ensureArcConsistency(size_t j,
                              std::vector<Domain>* domains) const override;

    /// Partially apply known values
    Constraint::shared_ptr partiallyApply(const Values&) const override;

    /// Partially apply known values, domain version
    Constraint::shared_ptr partiallyApply(
        const std::vector<Domain>&) const override;
  };

} // namespace gtsam
