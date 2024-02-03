#include "pl/internal/FakeSymbol.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "demangler/MicrosoftDemangle.h"
#include "demangler/MicrosoftDemangleNodes.h"
#include "demangler/StringViewExtras.h"

namespace pl::fake_symbol {

static bool consumeFront(std::string_view& S, char C) {
    if (!demangler::itanium_demangle::starts_with(S, C)) return false;
    S.remove_prefix(1);
    return true;
}

static bool consumeFront(std::string_view& S, std::string_view C) {
    if (!demangler::itanium_demangle::starts_with(S, C)) return false;
    S.remove_prefix(C.size());
    return true;
}

static demangler::ms_demangle::SpecialIntrinsicKind consumeSpecialIntrinsicKind(std::string_view& MangledName) {
    using namespace demangler::ms_demangle;
    if (consumeFront(MangledName, "?_7")) return SpecialIntrinsicKind::Vftable;
    if (consumeFront(MangledName, "?_8")) return SpecialIntrinsicKind::Vbtable;
    if (consumeFront(MangledName, "?_9")) return SpecialIntrinsicKind::VcallThunk;
    if (consumeFront(MangledName, "?_A")) return SpecialIntrinsicKind::Typeof;
    if (consumeFront(MangledName, "?_B")) return SpecialIntrinsicKind::LocalStaticGuard;
    if (consumeFront(MangledName, "?_C")) return SpecialIntrinsicKind::StringLiteralSymbol;
    if (consumeFront(MangledName, "?_P")) return SpecialIntrinsicKind::UdtReturning;
    if (consumeFront(MangledName, "?_R0")) return SpecialIntrinsicKind::RttiTypeDescriptor;
    if (consumeFront(MangledName, "?_R1")) return SpecialIntrinsicKind::RttiBaseClassDescriptor;
    if (consumeFront(MangledName, "?_R2")) return SpecialIntrinsicKind::RttiBaseClassArray;
    if (consumeFront(MangledName, "?_R3")) return SpecialIntrinsicKind::RttiClassHierarchyDescriptor;
    if (consumeFront(MangledName, "?_R4")) return SpecialIntrinsicKind::RttiCompleteObjLocator;
    if (consumeFront(MangledName, "?_S")) return SpecialIntrinsicKind::LocalVftable;
    if (consumeFront(MangledName, "?__E")) return SpecialIntrinsicKind::DynamicInitializer;
    if (consumeFront(MangledName, "?__F")) return SpecialIntrinsicKind::DynamicAtexitDestructor;
    if (consumeFront(MangledName, "?__J")) return SpecialIntrinsicKind::LocalStaticThreadGuard;
    return SpecialIntrinsicKind::None;
}

// generate fakeSymbol for virtual functions
std::optional<std::string> getFakeSymbol(const std::string& fn, bool removeVirtual) {
    using namespace demangler::ms_demangle;
    using namespace demangler;
    Demangler        demangler;
    std::string_view name = fn;

    if (!consumeFront(name, '?')) return std::nullopt;

    SpecialIntrinsicKind specialIntrinsicKind = consumeSpecialIntrinsicKind(name);

    if (specialIntrinsicKind != SpecialIntrinsicKind::None) return std::nullopt;

    SymbolNode* symbolNode = demangler.demangleDeclarator(name);
    if (symbolNode == nullptr
        || (symbolNode->kind() != NodeKind::FunctionSymbol && symbolNode->kind() != NodeKind::VariableSymbol)
        || demangler.Error)
        return std::nullopt;

    if (symbolNode->kind() == NodeKind::FunctionSymbol) {
        auto&  funcNode     = reinterpret_cast<FunctionSymbolNode*>(symbolNode)->Signature->FunctionClass;
        bool   modified     = false;
        size_t funcNodeSize = funcNode.toString().size();
        if (removeVirtual) {
            if (funcNode.has(FC_Virtual)) {
                funcNode.remove(FC_Virtual);
                modified = true;
            } else {
                return std::nullopt;
            }
        }
        if (funcNode.has(FC_Protected)) {
            funcNode.remove(FC_Protected);
            funcNode.add(FC_Public);
            modified = true;
        }
        if (funcNode.has(FC_Private)) {
            funcNode.remove(FC_Private);
            funcNode.add(FC_Public);
            modified = true;
        }
        if (modified) {
            std::string fakeSymbol   = fn;
            std::string fakeFuncNode = funcNode.toString();
            size_t      offset       = fn.size() - funcNode.pos.size();
            fakeSymbol.erase(offset, funcNodeSize);
            fakeSymbol.insert(offset, fakeFuncNode);
            return fakeSymbol;
        }
    } else if (symbolNode->kind() == NodeKind::VariableSymbol) {
        auto& storageClass = reinterpret_cast<VariableSymbolNode*>(symbolNode)->SC;
        bool  modified     = false;
        if (storageClass == StorageClass::PrivateStatic) {
            storageClass.set(StorageClass::PublicStatic);
            modified = true;
        }
        if (storageClass == StorageClass::ProtectedStatic) {
            storageClass.set(StorageClass::PublicStatic);
            modified = true;
        }
        if (modified) {
            std::string fakeSymbol       = fn;
            char        fakeStorageClass = storageClass.toChar();
            size_t      offset           = fn.size() - storageClass.pos.size();
            fakeSymbol[offset]           = fakeStorageClass;
            return fakeSymbol;
        }
    }

    return std::nullopt;
}

} // namespace pl::fake_symbol
