#pragma once
#include "defs.h"
#include "tabletid.h"
#include "localdb.h"

#include <ydb/core/protos/blobstorage_config.pb.h>

#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>

namespace NKikimrBlobStorage {
    class TDefineStoragePool;
}

namespace NKikimr {

struct TDomainsInfo : public TThrRefBase {
    static constexpr ui32 FirstUserTag = 32;
    static constexpr ui32 FakeRootTag = 0xFFFFE;
    static constexpr ui32 MaxUserTag = 0xFFFFF;
    static constexpr ui32 BadDomainId = 0xFFFFFFFFu;
    static constexpr ui64 BadTabletId = 0xFFFFFFFFFFFFFFFFull;
    static constexpr ui32 DomainBits = 5;
    static constexpr ui32 MaxDomainId = (1 << DomainBits) - 1;

    // for testing purposes only
    static ui64 MakeTxCoordinatorIDFixed(ui32 uid) {
        Y_DEBUG_ABORT_UNLESS(uid < 256);
        const ui64 uniqPart = 0x800000 | uid;
        return MakeTabletID(false, uniqPart);
    }

    static ui64 MakeTxMediatorIDFixed(ui32 uid) {
        Y_DEBUG_ABORT_UNLESS(uid < 256);
        const ui64 uniqPart = 0x810000 | uid;
        return MakeTabletID(false, uniqPart);
    }

    static ui64 MakeTxAllocatorIDFixed(ui32 uid) {
        Y_DEBUG_ABORT_UNLESS(uid > 0 && uid < 4096);
        const ui64 uniqPart = 0x820000 | uid;
        return MakeTabletID(false, uniqPart);
    }

    static constexpr const char* SystemTableDefaultPoicyName() {
        return "SystemTableDefault";
    }

    static constexpr const char* UserTableDefaultPoicyName() {
        return "UserTableDefault";
    }

    typedef THashMap<TString, TIntrusiveConstPtr<NLocalDb::TCompactionPolicy>> TNamedCompactionPolicies;

    struct TDomain : public TThrRefBase {
        using TPtr = TIntrusivePtr<TDomain>;

        using TVectorUi64 = TVector<ui64>;
        using TVectorUi32 = TVector<ui32>;
        using TStoragePoolKinds = THashMap<TString, NKikimrBlobStorage::TDefineStoragePool>;

        const ui32 DomainUid;
        const ui64 SchemeRoot;
        const TString Name;
        const TVector<ui64> Coordinators;
        const TVector<ui64> Mediators;
        const TVector<ui64> TxAllocators;
        const ui64 DomainPlanResolution;
        const TStoragePoolKinds StoragePoolTypes;

        static constexpr ui32 TimecastBucketsPerMediator = 2; // <- any sense in making this configurable? may be for debug?..

    private:
        //don't reinterpret any data
        TDomain(const TString &name, ui32 domainUid, ui64 schemeRootId,
                TVectorUi64 coordinators, TVectorUi64 mediators, TVectorUi64 allocators,
                ui64 domainPlanResolution, const TStoragePoolKinds *poolTypes);

     public:
        ~TDomain();

        //no any tablets setted
        static TDomain::TPtr ConstructEmptyDomain(const TString &name, ui32 domainId = 0)
        {
            const ui64 schemeRoot = 0;
            ui64 planResolution = 500;
            return new TDomain(name, domainId, schemeRoot, {}, {}, {}, planResolution, nullptr);
        }

        template <typename TUidsContainerUi64>
        static TDomain::TPtr ConstructDomainWithExplicitTabletIds(const TString &name, ui32 domainUid, ui64 schemeRoot,
                                            ui64 planResolution,
                                            const TUidsContainerUi64 &coordinatorUids,
                                            const TUidsContainerUi64 &mediatorUids,
                                            const TUidsContainerUi64 &allocatorUids,
                                            const TStoragePoolKinds &poolTypes)
        {
            return new TDomain(name, domainUid, schemeRoot,
                            TVectorUi64(coordinatorUids.begin(), coordinatorUids.end()),
                            TVectorUi64(mediatorUids.begin(), mediatorUids.end()),
                            TVectorUi64(allocatorUids.begin(), allocatorUids.end()),
                            planResolution, &poolTypes);
        }

        template <typename TUidsContainerUi64>
        static TDomain::TPtr ConstructDomainWithExplicitTabletIds(const TString &name, ui32 domainUid, ui64 schemeRoot,
                                            ui64 planResolution,
                                            const TUidsContainerUi64 &coordinatorUids,
                                            const TUidsContainerUi64 &mediatorUids,
                                            const TUidsContainerUi64 &allocatorUids)
        {
            return new TDomain(name, domainUid, schemeRoot,
                            TVectorUi64(coordinatorUids.begin(), coordinatorUids.end()),
                            TVectorUi64(mediatorUids.begin(), mediatorUids.end()),
                            TVectorUi64(allocatorUids.begin(), allocatorUids.end()),
                            planResolution, nullptr);
        }

        ui32 DomainRootTag() const {
            return DomainUid + FirstUserTag;
        }
    };

    TIntrusivePtr<TDomain> Domain;
    std::optional<ui64> HiveTabletId;
    TNamedCompactionPolicies NamedCompactionPolicies;

    TDomainsInfo() {
        // Add default configs. They can be overriden by user
        NamedCompactionPolicies[SystemTableDefaultPoicyName()] = NLocalDb::CreateDefaultTablePolicy();
        NamedCompactionPolicies[UserTableDefaultPoicyName()] = NLocalDb::CreateDefaultUserTablePolicy();
    }

    TIntrusiveConstPtr<NLocalDb::TCompactionPolicy> GetDefaultSystemTablePolicy() const {
        return *NamedCompactionPolicies.FindPtr(SystemTableDefaultPoicyName());
    }

    TIntrusiveConstPtr<NLocalDb::TCompactionPolicy> GetDefaultUserTablePolicy() const {
        return *NamedCompactionPolicies.FindPtr(UserTableDefaultPoicyName());
    }

    void AddCompactionPolicy(TString name, TIntrusiveConstPtr<NLocalDb::TCompactionPolicy> policy) {
        NamedCompactionPolicies[name] = policy;
    }

    void AddDomain(TDomain *domain) {
        Y_ABORT_UNLESS(!Domain);
        Domain = domain;
    }

    void AddHive(ui64 hive) {
        Y_ABORT_UNLESS(!HiveTabletId);
        HiveTabletId = hive;
    }

    void ClearDomainsAndHive() {
        Domain.Reset();
        HiveTabletId.reset();
    }

    const TDomain& GetDomain(ui32 domainUid) const {
        Y_ABORT_UNLESS(Domain);
        Y_ABORT_UNLESS(domainUid == Domain->DomainUid);
        return *Domain;
    }

    const TDomain *GetDomain() const {
        Y_ABORT_UNLESS(Domain);
        return Domain.Get();
    }

    const TDomain* GetDomainByName(TStringBuf name) const {
        return Domain && name == Domain->Name ? Domain.Get() : nullptr;
    }

    ui64 GetHive() const {
        return HiveTabletId.value_or(BadTabletId);
    }
};

}
