#pragma once
#include <ydb/core/tx/columnshard/engines/scheme/statistics/abstract/operator.h>

namespace NKikimr::NOlap::NStatistics::NMax {

class TOperator: public IOperator {
private:
    using TBase = IOperator;
    ui32 EntityId = 0;
    static inline auto Registrator = TFactory::TRegistrator<TOperator>(::ToString(EType::Max));
protected:
    virtual void DoFillStatisticsData(const THashMap<ui32, std::vector<std::shared_ptr<IPortionDataChunk>>>& data, TPortionStorage& portionStats, const TIndexInfo& index) const override;
    virtual void DoShiftCursor(TPortionStorageCursor& cursor) const override {
        cursor.AddScalarsPosition(1);
    }
    virtual std::vector<ui32> GetEntityIds() const override {
        return {EntityId};
    }
    virtual bool DoDeserializeFromProto(const NKikimrColumnShardStatisticsProto::TOperatorContainer& proto) override;
    virtual void DoSerializeToProto(NKikimrColumnShardStatisticsProto::TOperatorContainer& proto) const override;
public:

    static bool IsAvailableType(const NScheme::TTypeInfo type) {
        switch (type.GetTypeId()) {
            case NScheme::NTypeIds::Int8:
            case NScheme::NTypeIds::Uint8:
            case NScheme::NTypeIds::Int16:
            case NScheme::NTypeIds::Uint16:
            case NScheme::NTypeIds::Int32:
            case NScheme::NTypeIds::Uint32:
            case NScheme::NTypeIds::Int64:
            case NScheme::NTypeIds::Uint64:
            case NScheme::NTypeIds::Timestamp:
            case NScheme::NTypeIds::Double:
            case NScheme::NTypeIds::Datetime:
            case NScheme::NTypeIds::Date:
                return true;
            default:
                break;
        }
        return false;
    }

    TOperator()
        : TBase(EType::Max)
    {

    }

    TOperator(const ui32 entityId)
        : TBase(EType::Max)
        , EntityId(entityId) {

    }
};

}