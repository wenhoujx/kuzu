#include "processor/processor_task.h"

namespace kuzu {
namespace processor {

void ProcessorTask::run() {
    // We need the lock when cloning because multiple threads can be accessing to clone,
    // which is not thread safe
    common::lock_t lck{mtx};
    auto clonedPipelineRoot = sink->clone();
    lck.unlock();
    auto currentSink = (Sink*)clonedPipelineRoot.get();
    auto resultSet = populateResultSet(currentSink, executionContext->memoryManager);
    currentSink->execute(resultSet.get(), executionContext);
}

void ProcessorTask::finalizeIfNecessary() {
    sink->finalize(executionContext);
}

std::unique_ptr<ResultSet> ProcessorTask::populateResultSet(
    Sink* op, storage::MemoryManager* memoryManager) {
    auto resultSetDescriptor = op->getResultSetDescriptor();
    if (resultSetDescriptor == nullptr) {
        // Some pipeline does not need a resultSet, e.g. OrderByMerge
        return nullptr;
    }
    auto numDataChunks = resultSetDescriptor->getNumDataChunks();
    auto resultSet = std::make_unique<ResultSet>(numDataChunks);
    for (auto i = 0u; i < numDataChunks; ++i) {
        auto dataChunkDescriptor = resultSetDescriptor->getDataChunkDescriptor(i);
        auto numValueVectors = dataChunkDescriptor->getNumValueVectors();
        auto dataChunk = std::make_unique<common::DataChunk>(numValueVectors);
        if (dataChunkDescriptor->isSingleState()) {
            dataChunk->state = common::DataChunkState::getSingleValueDataChunkState();
        }
        for (auto j = 0u; j < dataChunkDescriptor->getNumValueVectors(); ++j) {
            auto expression = dataChunkDescriptor->getExpression(j);
            auto vector =
                std::make_shared<common::ValueVector>(expression->dataType, memoryManager);
            if (vector->dataType.getTypeID() == common::STRUCT) {
                addStructFieldsVectors(vector.get(), dataChunk.get(), memoryManager);
            }
            dataChunk->insert(j, std::move(vector));
        }
        resultSet->insert(i, std::move(dataChunk));
    }
    return resultSet;
}

void ProcessorTask::addStructFieldsVectors(common::ValueVector* structVector,
    common::DataChunk* dataChunk, storage::MemoryManager* memoryManager) {
    auto structTypeInfo =
        reinterpret_cast<common::StructTypeInfo*>(structVector->dataType.getExtraTypeInfo());
    for (auto& childType : structTypeInfo->getChildrenTypes()) {
        auto childVector = std::make_shared<common::ValueVector>(*childType, memoryManager);
        dataChunk->addValueVector(childVector);
        structVector->addChildVector(childVector);
    }
}

} // namespace processor
} // namespace kuzu
