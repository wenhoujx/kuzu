#include "c_api_test/c_api_test.h"
using namespace kuzu::main;
using namespace kuzu::common;

class CApiDataTypeTest : public CApiTest {
public:
    std::string getInputDir() override {
        return TestHelper::appendKuzuRootPath("dataset/tinysnb/");
    }
};

TEST_F(CApiDataTypeTest, Create) {
    auto dataType = kuzu_data_type_create(kuzu_data_type_id::INT64, nullptr, 0);
    ASSERT_NE(dataType, nullptr);
    auto dataTypeCpp = (DataType*)dataType->_data_type;
    ASSERT_EQ(dataTypeCpp->getTypeID(), DataTypeID::INT64);

    auto dataType2 = kuzu_data_type_create(kuzu_data_type_id::VAR_LIST, dataType, 0);
    ASSERT_NE(dataType2, nullptr);
    auto dataTypeCpp2 = (DataType*)dataType2->_data_type;
    ASSERT_EQ(dataTypeCpp2->getTypeID(), DataTypeID::VAR_LIST);
    ASSERT_EQ(dataTypeCpp2->getChildType()->getTypeID(), DataTypeID::INT64);

    auto dataType3 = kuzu_data_type_create(kuzu_data_type_id::FIXED_LIST, dataType, 100);
    ASSERT_NE(dataType3, nullptr);
    auto dataTypeCpp3 = (DataType*)dataType3->_data_type;
    ASSERT_EQ(dataTypeCpp3->getTypeID(), DataTypeID::FIXED_LIST);
    ASSERT_EQ(dataTypeCpp3->getChildType()->getTypeID(), DataTypeID::INT64);
    auto extraInfo = (FixedListTypeInfo*)dataTypeCpp3->getExtraTypeInfo();
    ASSERT_EQ(extraInfo->getFixedNumElementsInList(), 100);

    // Since child type is copied, we should be able to destroy the original type without an error.
    kuzu_data_type_destroy(dataType);
    kuzu_data_type_destroy(dataType2);
    kuzu_data_type_destroy(dataType3);
}

TEST_F(CApiDataTypeTest, Clone) {
    auto dataType = kuzu_data_type_create(kuzu_data_type_id::INT64, nullptr, 0);
    ASSERT_NE(dataType, nullptr);
    auto dataTypeClone = kuzu_data_type_clone(dataType);
    ASSERT_NE(dataTypeClone, nullptr);
    auto dataTypeCpp = (DataType*)dataType->_data_type;
    auto dataTypeCloneCpp = (DataType*)dataTypeClone->_data_type;
    ASSERT_TRUE(*dataTypeCpp == *dataTypeCloneCpp);

    auto dataType2 = kuzu_data_type_create(kuzu_data_type_id::VAR_LIST, dataType, 0);
    ASSERT_NE(dataType2, nullptr);
    auto dataTypeClone2 = kuzu_data_type_clone(dataType2);
    ASSERT_NE(dataTypeClone2, nullptr);
    auto dataTypeCpp2 = (DataType*)dataType2->_data_type;
    auto dataTypeCloneCpp2 = (DataType*)dataTypeClone2->_data_type;
    ASSERT_TRUE(*dataTypeCpp2 == *dataTypeCloneCpp2);

    auto dataType3 = kuzu_data_type_create(kuzu_data_type_id::FIXED_LIST, dataType, 100);
    ASSERT_NE(dataType3, nullptr);
    auto dataTypeClone3 = kuzu_data_type_clone(dataType3);
    ASSERT_NE(dataTypeClone3, nullptr);
    auto dataTypeCpp3 = (DataType*)dataType3->_data_type;
    auto dataTypeCloneCpp3 = (DataType*)dataTypeClone3->_data_type;
    ASSERT_TRUE(*dataTypeCpp3 == *dataTypeCloneCpp3);

    kuzu_data_type_destroy(dataType);
    kuzu_data_type_destroy(dataType2);
    kuzu_data_type_destroy(dataType3);
    kuzu_data_type_destroy(dataTypeClone);
    kuzu_data_type_destroy(dataTypeClone2);
    kuzu_data_type_destroy(dataTypeClone3);
}

TEST_F(CApiDataTypeTest, Eqauls) {
    auto dataType = kuzu_data_type_create(kuzu_data_type_id::INT64, nullptr, 0);
    ASSERT_NE(dataType, nullptr);
    auto dataTypeClone = kuzu_data_type_clone(dataType);
    ASSERT_NE(dataTypeClone, nullptr);
    ASSERT_TRUE(kuzu_data_type_equals(dataType, dataTypeClone));

    auto dataType2 = kuzu_data_type_create(kuzu_data_type_id::VAR_LIST, dataType, 0);
    ASSERT_NE(dataType2, nullptr);
    auto dataTypeClone2 = kuzu_data_type_clone(dataType2);
    ASSERT_NE(dataTypeClone2, nullptr);
    ASSERT_TRUE(kuzu_data_type_equals(dataType2, dataTypeClone2));

    auto dataType3 = kuzu_data_type_create(kuzu_data_type_id::FIXED_LIST, dataType, 100);
    ASSERT_NE(dataType3, nullptr);
    auto dataTypeClone3 = kuzu_data_type_clone(dataType3);
    ASSERT_NE(dataTypeClone3, nullptr);
    ASSERT_TRUE(kuzu_data_type_equals(dataType3, dataTypeClone3));

    ASSERT_FALSE(kuzu_data_type_equals(dataType, dataType2));
    ASSERT_FALSE(kuzu_data_type_equals(dataType, dataType3));
    ASSERT_FALSE(kuzu_data_type_equals(dataType2, dataType3));

    kuzu_data_type_destroy(dataType);
    kuzu_data_type_destroy(dataType2);
    kuzu_data_type_destroy(dataType3);
    kuzu_data_type_destroy(dataTypeClone);
    kuzu_data_type_destroy(dataTypeClone2);
    kuzu_data_type_destroy(dataTypeClone3);
}

TEST_F(CApiDataTypeTest, GetID) {
    auto dataType = kuzu_data_type_create(kuzu_data_type_id::INT64, nullptr, 0);
    ASSERT_NE(dataType, nullptr);
    ASSERT_EQ(kuzu_data_type_get_id(dataType), kuzu_data_type_id::INT64);

    auto dataType2 = kuzu_data_type_create(kuzu_data_type_id::VAR_LIST, dataType, 0);
    ASSERT_NE(dataType2, nullptr);
    ASSERT_EQ(kuzu_data_type_get_id(dataType2), kuzu_data_type_id::VAR_LIST);

    auto dataType3 = kuzu_data_type_create(kuzu_data_type_id::FIXED_LIST, dataType, 100);
    ASSERT_NE(dataType3, nullptr);
    ASSERT_EQ(kuzu_data_type_get_id(dataType3), kuzu_data_type_id::FIXED_LIST);

    kuzu_data_type_destroy(dataType);
    kuzu_data_type_destroy(dataType2);
    kuzu_data_type_destroy(dataType3);
}

TEST_F(CApiDataTypeTest, GetChildType) {
    auto dataType = kuzu_data_type_create(kuzu_data_type_id::INT64, nullptr, 0);
    ASSERT_NE(dataType, nullptr);
    ASSERT_EQ(kuzu_data_type_get_child_type(dataType), nullptr);

    auto dataType2 = kuzu_data_type_create(kuzu_data_type_id::VAR_LIST, dataType, 0);
    ASSERT_NE(dataType2, nullptr);
    auto childType2 = kuzu_data_type_get_child_type(dataType2);
    ASSERT_NE(childType2, nullptr);
    ASSERT_EQ(kuzu_data_type_get_id(childType2), kuzu_data_type_id::INT64);
    kuzu_data_type_destroy(childType2);
    kuzu_data_type_destroy(dataType2);

    auto dataType3 = kuzu_data_type_create(kuzu_data_type_id::FIXED_LIST, dataType, 100);
    ASSERT_NE(dataType3, nullptr);
    auto childType3 = kuzu_data_type_get_child_type(dataType3);
    kuzu_data_type_destroy(dataType3);
    // Destroying dataType3 should not destroy childType3.
    ASSERT_NE(childType3, nullptr);
    ASSERT_EQ(kuzu_data_type_get_id(childType3), kuzu_data_type_id::INT64);
    kuzu_data_type_destroy(childType3);

    kuzu_data_type_destroy(dataType);
}

TEST_F(CApiDataTypeTest, GetFixedNumElementsInList) {
    auto dataType = kuzu_data_type_create(kuzu_data_type_id::INT64, nullptr, 0);
    ASSERT_NE(dataType, nullptr);
    ASSERT_EQ(kuzu_data_type_get_fixed_num_elements_in_list(dataType), 0);

    auto dataType2 = kuzu_data_type_create(kuzu_data_type_id::VAR_LIST, dataType, 0);
    ASSERT_NE(dataType2, nullptr);
    ASSERT_EQ(kuzu_data_type_get_fixed_num_elements_in_list(dataType2), 0);

    auto dataType3 = kuzu_data_type_create(kuzu_data_type_id::FIXED_LIST, dataType, 100);
    ASSERT_NE(dataType3, nullptr);
    ASSERT_EQ(kuzu_data_type_get_fixed_num_elements_in_list(dataType3), 100);

    kuzu_data_type_destroy(dataType);
    kuzu_data_type_destroy(dataType2);
    kuzu_data_type_destroy(dataType3);
}
