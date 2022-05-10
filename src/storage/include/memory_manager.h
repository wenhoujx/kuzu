#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <stack>

#include "src/common/include/configs.h"
#include "src/storage/include/buffer_manager.h"

using namespace graphflow::storage;

namespace graphflow {
namespace storage {

struct MemoryBlock {

public:
    explicit MemoryBlock(uint32_t pageIdx, uint8_t* data)
        : size(LARGE_PAGE_SIZE), pageIdx(pageIdx), data(data) {}

public:
    uint64_t size;
    uint32_t pageIdx;
    uint8_t* data;
};

// Memory manager for allocating/reclaiming large intermediate memory blocks. It can allocate a
// memory block with fixed size of LARGE_PAGE_SIZE from the buffer manager.
class MemoryManager {
public:
    explicit MemoryManager(BufferManager* bm) : bm(bm) {
        // Because the memory manager only manages blocks in memory, this file should never be
        // created, opened, or written to. It's a place holder name. We keep the name for logging
        // purposes.
        fh = make_shared<FileHandle>(
            "mm-place-holder-file-name", FileHandle::O_LargePagedInMemoryTmpFile);
    }

    unique_ptr<MemoryBlock> allocateBlock(bool initializeToZero = false);

    void freeBlock(uint32_t pageIdx);

private:
    shared_ptr<FileHandle> fh;
    BufferManager* bm;
    stack<uint32_t> freePages;
    mutex memMgrLock;
};
} // namespace storage
} // namespace graphflow
