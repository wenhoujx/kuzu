#include "src/planner/include/property_scan_pushdown.h"

#include "src/planner/include/logical_plan/operator/aggregate/logical_aggregate.h"
#include "src/planner/include/logical_plan/operator/distinct/logical_distinct.h"
#include "src/planner/include/logical_plan/operator/exists/logical_exist.h"
#include "src/planner/include/logical_plan/operator/extend/logical_extend.h"
#include "src/planner/include/logical_plan/operator/hash_join/logical_hash_join.h"
#include "src/planner/include/logical_plan/operator/nested_loop_join/logical_left_nested_loop_join.h"
#include "src/planner/include/logical_plan/operator/order_by/logical_order_by.h"
#include "src/planner/include/logical_plan/operator/result_collector/logical_result_collector.h"
#include "src/planner/include/logical_plan/operator/scan_node_id/logical_scan_node_id.h"
#include "src/planner/include/logical_plan/operator/scan_property/logical_scan_node_property.h"
#include "src/planner/include/logical_plan/operator/scan_property/logical_scan_rel_property.h"

namespace graphflow {
namespace planner {

shared_ptr<LogicalOperator> PropertyScanPushDown::rewrite(
    shared_ptr<LogicalOperator> op, Schema& schema) {
    switch (op->getLogicalOperatorType()) {
    case LOGICAL_SCAN_NODE_ID:
        return rewriteScanNodeID(op, schema);
    case LOGICAL_EXTEND:
        return rewriteExtend(op, schema);
    case LOGICAL_SCAN_NODE_PROPERTY:
        return rewriteScanNodeProperty(op, schema);
    case LOGICAL_SCAN_REL_PROPERTY:
        return rewriteScanRelProperty(op, schema);
    case LOGICAL_AGGREGATE:
        return rewriteAggregate(op, schema);
    case LOGICAL_DISTINCT:
        return rewriteDistinct(op, schema);
    case LOGICAL_ORDER_BY:
        return rewriteOrderBy(op, schema);
    case LOGICAL_HASH_JOIN:
        return rewriteHashJoin(op, schema);
    case LOGICAL_EXISTS:
        return rewriteExists(op, schema);
    case LOGICAL_LEFT_NESTED_LOOP_JOIN:
        return rewriteLeftNestedLoopJoin(op, schema);
    case LOGICAL_RESULT_COLLECTOR:
        return rewriteResultCollector(op, schema);
    default:
        rewriteChildrenOperators(op, schema);
        return op;
    }
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteScanNodeID(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& scanNodeID = (LogicalScanNodeID&)*op;
    return applyPropertyScansIfNecessary(scanNodeID.nodeID, op, schema);
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteExtend(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& extend = (LogicalExtend&)*op;
    return applyPropertyScansIfNecessary(extend.nbrNodeID, op, schema);
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteScanNodeProperty(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& scanNodeProperty = (LogicalScanNodeProperty&)*op;
    addPropertyScan(scanNodeProperty.nodeID, op);
    schema.removeExpression(scanNodeProperty.getPropertyExpressionName());
    return rewrite(scanNodeProperty.getChild(0), schema);
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteScanRelProperty(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& scanRelProperty = (LogicalScanRelProperty&)*op;
    addPropertyScan(scanRelProperty.nbrNodeID, op);
    schema.removeExpression(scanRelProperty.getPropertyExpressionName());
    return rewrite(scanRelProperty.getChild(0), schema);
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteAggregate(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalAggregate = (LogicalAggregate&)*op;
    op->setChild(0, rewrite(op->getChild(0), *logicalAggregate.getSchemaBeforeAggregate()));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteDistinct(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalDistinct = (LogicalDistinct&)*op;
    op->setChild(0, rewrite(op->getChild(0), *logicalDistinct.getSchemaBeforeDistinct()));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteOrderBy(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalOrderBy = (LogicalOrderBy&)*op;
    addPropertyScansToSchema(schema);
    for (auto& expressionName : getRemainingPropertyExpressionNames()) {
        logicalOrderBy.addExpressionToMaterialize(expressionName);
    }
    op->setChild(0, rewrite(op->getChild(0), *logicalOrderBy.getSchemaBeforeOrderBy()));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteHashJoin(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalHashJoin = (LogicalHashJoin&)*op;
    op->setChild(0, rewrite(op->getChild(0), schema));
    addPropertyScansToSchema(schema);
    for (auto& expressionName : getRemainingPropertyExpressionNames()) {
        logicalHashJoin.addExpressionToMaterialize(expressionName);
    }
    op->setChild(1, rewrite(op->getChild(1), *logicalHashJoin.buildSideSchema));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteExists(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalExists = (LogicalExists&)*op;
    op->setChild(0, rewrite(op->getChild(0), schema));
    op->setChild(1, rewrite(op->getChild(1), *logicalExists.subPlanSchema));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteLeftNestedLoopJoin(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalLeftNLJ = (LogicalLeftNestedLoopJoin&)*op;
    op->setChild(0, rewrite(op->getChild(0), schema));
    addPropertyScansToSchema(schema);
    op->setChild(1, rewrite(op->getChild(1), *logicalLeftNLJ.subPlanSchema));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::rewriteResultCollector(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    auto& logicalResultCollector = (LogicalResultCollector&)*op;
    op->setChild(0, rewrite(op->getChild(0), *logicalResultCollector.getSchema()));
    return op;
}

shared_ptr<LogicalOperator> PropertyScanPushDown::applyPropertyScansIfNecessary(
    const string& nodeID, const shared_ptr<LogicalOperator>& op, Schema& schema) {
    if (!nodeIDToPropertyScansMap.contains(nodeID)) {
        // nothing needs to be applied
        rewriteChildrenOperators(op, schema);
        return op;
    }
    auto& propertyScans = nodeIDToPropertyScansMap.at(nodeID);
    // chain property scans
    for (auto i = 0u; i < propertyScans.size() - 1; ++i) {
        propertyScans[i]->setChild(0, propertyScans[i + 1]);
    }
    propertyScans.back()->setChild(0, op);
    auto result = propertyScans[0];
    // update schema
    auto groupPos = schema.getGroupPos(nodeID);
    for (auto& propertyScan : propertyScans) {
        auto& scanProperty = (LogicalScanProperty&)*propertyScan;
        schema.insertToGroup(scanProperty.getPropertyExpressionName(), groupPos);
    }
    nodeIDToPropertyScansMap.erase(nodeID);
    rewriteChildrenOperators(op, schema);
    return result;
}

void PropertyScanPushDown::rewriteChildrenOperators(
    const shared_ptr<LogicalOperator>& op, Schema& schema) {
    if (op->getNumChildren() == 0) {
        return;
    }
    for (auto i = 0u; i < op->getNumChildren(); i++) {
        // If any property scanner is successfully pushed down, it will be removed from
        // nodeIDtoPropertyScanMap.
        op->setChild(i, rewrite(op->getChild(i), schema));
    }
}

void PropertyScanPushDown::addPropertyScan(
    const string& nodeID, const shared_ptr<LogicalOperator>& op) {
    if (!nodeIDToPropertyScansMap.contains(nodeID)) {
        nodeIDToPropertyScansMap.insert({nodeID, vector<shared_ptr<LogicalOperator>>()});
    }
    nodeIDToPropertyScansMap.at(nodeID).push_back(op);
}

void PropertyScanPushDown::addPropertyScansToSchema(Schema& schema) {
    for (auto& [nodeID, propertyScanners] : nodeIDToPropertyScansMap) {
        auto groupPos = schema.getGroupPos(nodeID);
        for (auto& propertyScanner : propertyScanners) {
            auto& scanProperty = (LogicalScanProperty&)*propertyScanner;
            schema.insertToGroup(scanProperty.getPropertyExpressionName(), groupPos);
        }
    }
}

unordered_set<string> PropertyScanPushDown::getRemainingPropertyExpressionNames() {
    unordered_set<string> result;
    for (auto& [nodeID, propertyScanners] : nodeIDToPropertyScansMap) {
        for (auto& propertyScanner : propertyScanners) {
            auto& scanProperty = (LogicalScanProperty&)*propertyScanner;
            result.insert(scanProperty.getPropertyExpressionName());
        }
    }
    return result;
}

} // namespace planner
} // namespace graphflow
