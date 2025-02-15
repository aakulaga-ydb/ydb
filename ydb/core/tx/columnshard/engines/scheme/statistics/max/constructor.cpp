#include "constructor.h"
#include "operator.h"

namespace NKikimr::NOlap::NStatistics::NMax {

NKikimr::TConclusion<TOperatorContainer> TConstructor::DoCreateOperator(const NSchemeShard::TOlapSchema& currentSchema) const {
    auto column = currentSchema.GetColumns().GetByName(ColumnName);
    if (!TOperator::IsAvailableType(column->GetType())) {
        return TConclusionStatus::Fail("incorrect type for stat calculation");
    }
    return TOperatorContainer(std::make_shared<TOperator>(column->GetId()));
}

bool TConstructor::DoDeserializeFromProto(const NKikimrColumnShardStatisticsProto::TConstructorContainer& proto) {
    if (!proto.HasMax()) {
        return false;
    }
    ColumnName = proto.GetMax().GetColumnName();
    if (!ColumnName) {
        return false;
    }
    return true;
}

void TConstructor::DoSerializeToProto(NKikimrColumnShardStatisticsProto::TConstructorContainer& proto) const {
    AFL_VERIFY(!!ColumnName);
    proto.MutableMax()->SetColumnName(ColumnName);
}

NKikimr::TConclusionStatus TConstructor::DoDeserializeFromJson(const NJson::TJsonValue& jsonData) {
    if (!jsonData.Has("column_name")) {
        return TConclusionStatus::Fail("no column_name field in json description");
    }
    TString columnNameLocal;
    if (!jsonData["column_name"].GetString(&columnNameLocal)) {
        return TConclusionStatus::Fail("incorrect column_name field in json description (no string)");
    }
    if (!columnNameLocal) {
        return TConclusionStatus::Fail("empty column_name field in json description");
    }
    ColumnName = columnNameLocal;
    return TConclusionStatus::Success();
}

}