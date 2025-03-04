#pragma once

#include "arrow/array/array_base.h"
#include "arrow/array/array_binary.h"
#include "arrow/array/array_primitive.h"
#include "arrow/scalar.h"
#include "common/types/types.h"
#include "storage/copier/table_copy_executor.h"
#include "storage/storage_structure/in_mem_file.h"

namespace kuzu {
namespace storage {

class InMemColumnChunk {
public:
    InMemColumnChunk(common::DataType dataType, common::offset_t startOffset,
        common::offset_t endOffset, uint16_t numBytesForElement, uint64_t numElementsInAPage);

    inline uint8_t* getPage(common::page_idx_t pageIdx) {
        assert(pageIdx <= endPageIdx && pageIdx >= startPageIdx);
        auto pageIdxInSet = pageIdx - startPageIdx;
        return pages.get() + (pageIdxInSet * common::BufferPoolConstants::PAGE_4KB_SIZE);
    }
    inline void copyValue(
        common::page_idx_t pageIdx, common::offset_t posInPage, const uint8_t* val) {
        auto elemPosInPageInBytes = posInPage * numBytesForElement;
        memcpy(getPage(pageIdx) + elemPosInPageInBytes, val, numBytesForElement);
    }
    inline uint8_t* getValue(common::offset_t nodeOffset) {
        auto cursor = CursorUtils::getPageElementCursor(nodeOffset, numElementsInAPage);
        auto elemPosInPageInBytes = cursor.elemPosInPage * numBytesForElement;
        return getPage(cursor.pageIdx) + elemPosInPageInBytes;
    }

    template<typename T, typename... Args>
    void templateCopyValuesToPage(const PageElementCursor& pageCursor, arrow::Array& array,
        uint64_t posInArray, uint64_t numValues, Args... args) {
        const auto& data = array.data();
        auto valuesInArray = data->GetValues<T>(1);
        auto valuesInPage = (T*)(getPage(pageCursor.pageIdx));
        if (data->MayHaveNulls()) {
            for (auto i = 0u; i < numValues; i++) {
                if (data->IsNull(i + posInArray)) {
                    continue;
                }
                valuesInPage[i + pageCursor.elemPosInPage] = valuesInArray[i + posInArray];
            }
        } else {
            for (auto i = 0u; i < numValues; i++) {
                valuesInPage[i + pageCursor.elemPosInPage] = valuesInArray[i + posInArray];
            }
        }
    }

    template<typename T, typename... Args>
    void templateCopyValuesAsStringToPage(const PageElementCursor& pageCursor, arrow::Array& array,
        uint64_t posInArray, uint64_t numValues, Args... args) {
        auto& stringArray = (arrow::StringArray&)array;
        auto data = stringArray.data();
        if (data->MayHaveNulls()) {
            for (auto i = 0u; i < numValues; i++) {
                if (data->IsNull(i + posInArray)) {
                    continue;
                }
                auto value = stringArray.GetView(i + posInArray);
                setValueFromString<T, Args...>(value.data(), value.length(), pageCursor.pageIdx,
                    i + pageCursor.elemPosInPage, args...);
            }
        } else {
            for (auto i = 0u; i < numValues; i++) {
                auto value = stringArray.GetView(i + posInArray);
                setValueFromString<T, Args...>(value.data(), value.length(), pageCursor.pageIdx,
                    i + pageCursor.elemPosInPage, args...);
            }
        }
    }

    template<typename T, typename... Args>
    void setValueFromString(const char* value, uint64_t length, common::page_idx_t pageIdx,
        uint64_t posInPage, Args... args) {
        throw common::CopyException("Unsupported type to set element for " +
                                    std::string(value, length) + " at pos " +
                                    std::to_string(posInPage));
    }

private:
    common::DataType dataType;
    uint16_t numBytesForElement;
    uint64_t numElementsInAPage;
    common::page_idx_t startPageIdx;
    common::page_idx_t endPageIdx;
    std::unique_ptr<uint8_t[]> pages;
};

template<>
void InMemColumnChunk::templateCopyValuesToPage<bool>(const PageElementCursor& pageCursor,
    arrow::Array& array, uint64_t posInArray, uint64_t numValues);
template<>
void InMemColumnChunk::templateCopyValuesToPage<common::interval_t>(
    const PageElementCursor& pageCursor, arrow::Array& array, uint64_t posInArray,
    uint64_t numValues);
template<>
void InMemColumnChunk::templateCopyValuesToPage<common::ku_list_t>(
    const PageElementCursor& pageCursor, arrow::Array& array, uint64_t posInArray,
    uint64_t numValues);
// Specialized optimization for copy string values from arrow to pages.
// The optimization is to use string_view to avoid creation of std::string.
// Possible switches: date, timestamp, interval, fixed/var list, string
template<>
void InMemColumnChunk::templateCopyValuesToPage<std::string, InMemOverflowFile*, PageByteCursor&,
    common::CopyDescription&>(const PageElementCursor& pageCursor, arrow::Array& array,
    uint64_t posInArray, uint64_t numValues, InMemOverflowFile* overflowFile,
    PageByteCursor& overflowCursor, common::CopyDescription& copyDesc);

template<>
void InMemColumnChunk::setValueFromString<common::ku_string_t, InMemOverflowFile*, PageByteCursor&>(
    const char* value, uint64_t length, common::page_idx_t pageIdx, uint64_t posInPage,
    InMemOverflowFile* overflowFile, PageByteCursor& overflowCursor);
template<>
void InMemColumnChunk::setValueFromString<uint8_t*, common::CopyDescription&>(const char* value,
    uint64_t length, common::page_idx_t pageIdx, uint64_t posInPage,
    common::CopyDescription& copyDescription);
template<>
void InMemColumnChunk::setValueFromString<common::ku_list_t, InMemOverflowFile*, PageByteCursor&,
    common::CopyDescription&>(const char* value, uint64_t length, common::page_idx_t pageIdx,
    uint64_t posInPage, InMemOverflowFile* overflowFile, PageByteCursor& overflowCursor,
    common::CopyDescription& copyDescription);
template<>
void InMemColumnChunk::setValueFromString<common::interval_t>(
    const char* value, uint64_t length, common::page_idx_t pageIdx, uint64_t posInPage);
template<>
void InMemColumnChunk::setValueFromString<common::date_t>(
    const char* value, uint64_t length, common::page_idx_t pageIdx, uint64_t posInPage);
template<>
void InMemColumnChunk::setValueFromString<common::timestamp_t>(
    const char* value, uint64_t length, common::page_idx_t pageIdx, uint64_t posInPage);

} // namespace storage
} // namespace kuzu
