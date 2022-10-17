#pragma once

#include "src/binder/query/updating_clause/include/bound_create_clause.h"
#include "src/binder/query/updating_clause/include/bound_delete_clause.h"
#include "src/binder/query/updating_clause/include/bound_set_clause.h"
#include "src/binder/query/updating_clause/include/bound_updating_clause.h"
#include "src/catalog/include/catalog.h"
#include "src/planner/logical_plan/include/logical_plan.h"

namespace graphflow {
namespace planner {

using namespace graphflow::catalog;

class Enumerator;

class UpdatePlanner {
public:
    UpdatePlanner(const catalog::Catalog& catalog, Enumerator* enumerator)
        : catalog{catalog}, enumerator{enumerator} {};

    inline void planUpdatingClause(
        BoundUpdatingClause& updatingClause, vector<unique_ptr<LogicalPlan>>& plans) {
        for (auto& plan : plans) {
            planUpdatingClause(updatingClause, *plan);
        }
    }

private:
    void planUpdatingClause(BoundUpdatingClause& updatingClause, LogicalPlan& plan);
    void planSetItem(expression_pair setItem, LogicalPlan& plan);

    void appendCreate(BoundCreateClause& createClause, LogicalPlan& plan);
    inline void appendSet(BoundSetClause& setClause, LogicalPlan& plan) {
        appendSet(getSetItems(setClause), plan);
    }
    void appendSet(vector<expression_pair> setItems, LogicalPlan& plan);
    void appendDelete(BoundDeleteClause& deleteClause, LogicalPlan& plan);

    vector<expression_pair> getSetItems(BoundCreateClause& createClause);
    vector<expression_pair> getSetItems(BoundSetClause& setClause);
    vector<expression_pair> splitSetItems(vector<expression_pair> setItems, bool isStructured);

private:
    const Catalog& catalog;
    Enumerator* enumerator;
};

} // namespace planner
} // namespace graphflow
