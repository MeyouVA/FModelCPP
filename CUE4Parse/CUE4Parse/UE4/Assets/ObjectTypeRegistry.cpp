// Ported from CUE4Parse/UE4/Assets/ObjectTypeRegistry.cs (manual, reflection-free registration).
#include "ObjectTypeRegistry.h"

#include <map>
#include <mutex>

#include "Exports/Engine/UDataTable.h"
#include "Exports/Engine/UCurveTable.h"
#include "Exports/Internationalization/UStringTable.h"
#include "Exports/UObjectRedirector.h"
#include "../Objects/UObject/UScriptStruct.h"
#include "../Objects/UObject/UEnum.h"
#include "../Objects/UObject/UFunction.h"
#include "../Objects/Engine/UBlueprintGeneratedClass.h"
#include "../Objects/Engine/UUserDefinedStruct.h"
#include "../Objects/Engine/UUserDefinedEnum.h"
#include "../Objects/Engine/Curves/UCurveFloat.h"
#include "../Objects/Engine/Curves/UCurveVector.h"
#include "../Objects/Engine/Curves/UCurveLinearColor.h"

namespace CUE4Parse::UE4::Assets
{
    namespace
    {
        // Meyers-singleton map (avoids static-init-order issues across translation units).
        std::map<std::string, ObjectTypeRegistry::Factory>& Registry()
        {
            static std::map<std::string, ObjectTypeRegistry::Factory> registry;
            return registry;
        }

        // C#'s static ctor calls RegisterEngine(assembly), reflecting over every UObject subclass. Without
        // reflection we enumerate the ported engine export types by hand here. TODO: add types as ported.
        void RegisterEngineTypes()
        {
            ObjectTypeRegistry::RegisterClass("DataTable",
                [] { return std::unique_ptr<Exports::UObject>(new Exports::Engine::UDataTable()); });
            ObjectTypeRegistry::RegisterClass("CurveTable",
                [] { return std::unique_ptr<Exports::UObject>(new Exports::Engine::UCurveTable()); });
            // FProperties reflection types (C# registers these; UStruct/UClass are [SkipObjectRegistration]).
            ObjectTypeRegistry::RegisterClass("ScriptStruct",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::UObject::UScriptStruct()); });
            ObjectTypeRegistry::RegisterClass("Enum",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::UObject::UEnum()); });
            ObjectTypeRegistry::RegisterClass("Function",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::UObject::UFunction()); });
            ObjectTypeRegistry::RegisterClass("StringTable",
                [] { return std::unique_ptr<Exports::UObject>(new Exports::Internationalization::UStringTable()); });
            ObjectTypeRegistry::RegisterClass("ObjectRedirector",
                [] { return std::unique_ptr<Exports::UObject>(new Exports::UObjectRedirector()); });
            // A cooked Blueprint class export (its class import is "BlueprintGeneratedClass"). Unlike UStruct/
            // UClass this concrete engine class IS registered in C#. Instances of a Blueprint (class name
            // "SomeBP_C") still fall through to a base UObject — resolving those needs the SuperStruct-chain
            // walk, which stays deferred (see Package::ConstructObject).
            ObjectTypeRegistry::RegisterClass("BlueprintGeneratedClass",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::Engine::UBlueprintGeneratedClass()); });
            // Blueprint-authored struct/enum. Concrete engine classes (registered, unlike UStruct/UEnum which
            // are only registered under their reflection names "ScriptStruct"/"Enum").
            ObjectTypeRegistry::RegisterClass("UserDefinedStruct",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::Engine::UUserDefinedStruct()); });
            ObjectTypeRegistry::RegisterClass("UserDefinedEnum",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::Engine::UUserDefinedEnum()); });
            // Curve-asset exports (UCurveBase is abstract -> not registered). Each wraps the S25 FRichCurve/
            // FSimpleCurve eval subsystem; their curves come from the object's own tagged StructProperties.
            ObjectTypeRegistry::RegisterClass("CurveFloat",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::Engine::Curves::UCurveFloat()); });
            ObjectTypeRegistry::RegisterClass("CurveVector",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::Engine::Curves::UCurveVector()); });
            ObjectTypeRegistry::RegisterClass("CurveLinearColor",
                [] { return std::unique_ptr<Exports::UObject>(new CUE4Parse::UE4::Objects::Engine::Curves::UCurveLinearColor()); });
        }

        void EnsureRegistered()
        {
            static std::once_flag flag;
            std::call_once(flag, RegisterEngineTypes);
        }
    }

    void ObjectTypeRegistry::RegisterClass(const std::string& serializedName, Factory factory)
    {
        Registry()[serializedName] = std::move(factory);
    }

    ObjectTypeRegistry::Factory ObjectTypeRegistry::Get(const std::string& serializedName)
    {
        EnsureRegistered();
        auto& reg = Registry();
        auto it = reg.find(serializedName);
        if (it == reg.end() && serializedName.size() > 2 &&
            serializedName.compare(serializedName.size() - 2, 2, "_C") == 0)
        {
            it = reg.find(serializedName.substr(0, serializedName.size() - 2));
        }
        return it != reg.end() ? it->second : Factory{};
    }
}
