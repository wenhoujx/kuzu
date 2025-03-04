#include "planner/logical_plan/logical_plan_util.h"

#include "planner/logical_plan/logical_operator/logical_extend.h"
#include "planner/logical_plan/logical_operator/logical_hash_join.h"
#include "planner/logical_plan/logical_operator/logical_intersect.h"
#include "planner/logical_plan/logical_operator/logical_recursive_extend.h"
#include "planner/logical_plan/logical_operator/logical_scan_node.h"

using namespace kuzu::binder;

namespace kuzu {
namespace planner {

void LogicalPlanUtil::encodeJoinRecursive(
    LogicalOperator* logicalOperator, std::string& encodeString) {
    switch (logicalOperator->getOperatorType()) {
    case LogicalOperatorType::CROSS_PRODUCT: {
        encodeCrossProduct(logicalOperator, encodeString);
        for (auto i = 0u; i < logicalOperator->getNumChildren(); ++i) {
            encodeString += "{";
            encodeJoinRecursive(logicalOperator->getChild(i).get(), encodeString);
            encodeString += "}";
        }
    } break;
    case LogicalOperatorType::INTERSECT: {
        encodeIntersect(logicalOperator, encodeString);
        for (auto i = 0u; i < logicalOperator->getNumChildren(); ++i) {
            encodeString += "{";
            encodeJoinRecursive(logicalOperator->getChild(i).get(), encodeString);
            encodeString += "}";
        }
    } break;
    case LogicalOperatorType::HASH_JOIN: {
        encodeHashJoin(logicalOperator, encodeString);
        encodeString += "{";
        encodeJoinRecursive(logicalOperator->getChild(0).get(), encodeString);
        encodeString += "}{";
        encodeJoinRecursive(logicalOperator->getChild(1).get(), encodeString);
        encodeString += "}";
    } break;
    case LogicalOperatorType::EXTEND: {
        encodeExtend(logicalOperator, encodeString);
        encodeJoinRecursive(logicalOperator->getChild(0).get(), encodeString);
    } break;
    case LogicalOperatorType::RECURSIVE_EXTEND: {
        encodeRecursiveExtend(logicalOperator, encodeString);
        encodeJoinRecursive(logicalOperator->getChild(0).get(), encodeString);
    } break;
    case LogicalOperatorType::SCAN_NODE: {
        encodeScanNodeID(logicalOperator, encodeString);
    } break;
    default:
        for (auto i = 0u; i < logicalOperator->getNumChildren(); ++i) {
            encodeJoinRecursive(logicalOperator->getChild(i).get(), encodeString);
        }
    }
}

void LogicalPlanUtil::encodeCrossProduct(
    LogicalOperator* logicalOperator, std::string& encodeString) {
    encodeString += "CP()";
}

void LogicalPlanUtil::encodeIntersect(LogicalOperator* logicalOperator, std::string& encodeString) {
    auto logicalIntersect = (LogicalIntersect*)logicalOperator;
    encodeString += "I(" + logicalIntersect->getIntersectNodeID()->toString() + ")";
}

void LogicalPlanUtil::encodeHashJoin(LogicalOperator* logicalOperator, std::string& encodeString) {
    auto logicalHashJoin = (LogicalHashJoin*)logicalOperator;
    encodeString += "HJ(";
    encodeString += logicalHashJoin->getExpressionsForPrinting() + ")";
}

void LogicalPlanUtil::encodeExtend(LogicalOperator* logicalOperator, std::string& encodeString) {
    auto logicalExtend = (LogicalExtend*)logicalOperator;
    encodeString += "E(" + logicalExtend->getNbrNode()->toString() + ")";
}

void LogicalPlanUtil::encodeRecursiveExtend(
    LogicalOperator* logicalOperator, std::string& encodeString) {
    auto logicalExtend = (LogicalRecursiveExtend*)logicalOperator;
    encodeString += "RE(" + logicalExtend->getNbrNode()->toString() + ")";
}

void LogicalPlanUtil::encodeScanNodeID(
    LogicalOperator* logicalOperator, std::string& encodeString) {
    auto logicalScanNode = (LogicalScanNode*)logicalOperator;
    encodeString += "S(" + logicalScanNode->getNode()->toString() + ")";
}

} // namespace planner
} // namespace kuzu
