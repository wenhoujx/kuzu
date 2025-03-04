#include "processor/operator/scan_node_id.h"

using namespace kuzu::common;

namespace kuzu {
namespace processor {

// Note: blindly update mask does not parallelize well, so we minimize write by first checking
// if the mask is set to true (mask value is equal to the expected currentMaskValue) or not.
void NodeTableSemiMask::incrementMaskValue(uint64_t nodeOffset, uint8_t currentMaskValue) {
    if (nodeMask->isMasked(nodeOffset, currentMaskValue)) {
        nodeMask->setMask(nodeOffset, currentMaskValue + 1);
    }
    auto morselIdx = nodeOffset >> DEFAULT_VECTOR_CAPACITY_LOG_2;
    if (morselMask->isMasked(morselIdx, currentMaskValue)) {
        morselMask->setMask(morselIdx, currentMaskValue + 1);
    }
}

std::pair<offset_t, offset_t> NodeTableState::getNextRangeToRead() {
    // Note: we use maxNodeOffset=UINT64_MAX to represent an empty table.
    if (currentNodeOffset > maxNodeOffset || maxNodeOffset == INVALID_NODE_OFFSET) {
        return std::make_pair(currentNodeOffset, currentNodeOffset);
    }
    if (isSemiMaskEnabled()) {
        auto currentMorselIdx = currentNodeOffset >> DEFAULT_VECTOR_CAPACITY_LOG_2;
        assert(currentNodeOffset % DEFAULT_VECTOR_CAPACITY == 0);
        while (currentMorselIdx <= maxMorselIdx && !semiMask->isMorselMasked(currentMorselIdx)) {
            currentMorselIdx++;
        }
        currentNodeOffset = std::min(currentMorselIdx * DEFAULT_VECTOR_CAPACITY, maxNodeOffset);
    }
    auto startOffset = currentNodeOffset;
    auto range = std::min(DEFAULT_VECTOR_CAPACITY, maxNodeOffset + 1 - currentNodeOffset);
    currentNodeOffset += range;
    return std::make_pair(startOffset, startOffset + range);
}

std::tuple<NodeTableState*, offset_t, offset_t> ScanNodeIDSharedState::getNextRangeToRead() {
    std::unique_lock lck{mtx};
    if (currentStateIdx == tableStates.size()) {
        return std::make_tuple(nullptr, INVALID_NODE_OFFSET, INVALID_NODE_OFFSET);
    }
    auto [startOffset, endOffset] = tableStates[currentStateIdx]->getNextRangeToRead();
    while (startOffset >= endOffset) {
        currentStateIdx++;
        if (currentStateIdx == tableStates.size()) {
            return std::make_tuple(nullptr, INVALID_NODE_OFFSET, INVALID_NODE_OFFSET);
        }
        auto [_startOffset, _endOffset] = tableStates[currentStateIdx]->getNextRangeToRead();
        startOffset = _startOffset;
        endOffset = _endOffset;
    }
    assert(currentStateIdx < tableStates.size());
    return std::make_tuple(tableStates[currentStateIdx].get(), startOffset, endOffset);
}

void ScanNodeID::initLocalStateInternal(ResultSet* resultSet, ExecutionContext* context) {
    outValueVector = resultSet->getValueVector(outDataPos);
    outValueVector->setSequential();
}

bool ScanNodeID::getNextTuplesInternal(ExecutionContext* context) {
    do {
        auto [state, startOffset, endOffset] = sharedState->getNextRangeToRead();
        if (state == nullptr) {
            return false;
        }
        auto nodeIDValues = (nodeID_t*)(outValueVector->getData());
        auto size = endOffset - startOffset;
        for (auto i = 0u; i < size; ++i) {
            nodeIDValues[i].offset = startOffset + i;
            nodeIDValues[i].tableID = state->getTable()->getTableID();
        }
        outValueVector->state->initOriginalAndSelectedSize(size);
        setSelVector(state, startOffset, endOffset);
    } while (outValueVector->state->selVector->selectedSize == 0);
    metrics->numOutputTuple.increase(outValueVector->state->selVector->selectedSize);
    return true;
}

void ScanNodeID::setSelVector(
    NodeTableState* tableState, offset_t startOffset, offset_t endOffset) {
    if (tableState->isSemiMaskEnabled()) {
        outValueVector->state->selVector->resetSelectorToValuePosBuffer();
        // Fill selected positions based on node mask for nodes between the given startOffset and
        // endOffset. If the node is masked (i.e., valid for read), then it is set to the selected
        // positions. Finally, we update the selectedSize for selVector.
        sel_t numSelectedValues = 0;
        for (auto i = 0u; i < (endOffset - startOffset); i++) {
            outValueVector->state->selVector->selectedPositions[numSelectedValues] = i;
            numSelectedValues += tableState->getSemiMask()->isNodeMasked(i + startOffset);
        }
        outValueVector->state->selVector->selectedSize = numSelectedValues;
    } else {
        // By default, the selected positions is set to the const incremental pos array.
        outValueVector->state->selVector->resetSelectorToUnselected();
    }
    // Apply changes to the selVector from nodes metadata.
    tableState->getTable()->setSelVectorForDeletedOffsets(transaction, outValueVector);
}

} // namespace processor
} // namespace kuzu
