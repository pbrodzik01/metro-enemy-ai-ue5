#include "ThesisReflection.h"

#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

namespace ThesisReflection
{
	bool GetNumber(const UObject* Object, FName PropertyName, double& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
		if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			OutValue = DoubleProperty->GetPropertyValue_InContainer(Object);
			return true;
		}
		if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
		{
			OutValue = FloatProperty->GetPropertyValue_InContainer(Object);
			return true;
		}
		if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
		{
			OutValue = IntProperty->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}

	bool SetNumber(UObject* Object, FName PropertyName, double Value)
	{
		if (!Object)
		{
			return false;
		}

		FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
		if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			DoubleProperty->SetPropertyValue_InContainer(Object, Value);
			return true;
		}
		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
		{
			FloatProperty->SetPropertyValue_InContainer(Object, static_cast<float>(Value));
			return true;
		}
		return false;
	}

	bool GetNameArray(const UObject* Object, FName PropertyName, TArray<FName>& OutNames)
	{
		if (!Object)
		{
			return false;
		}

		const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Object->GetClass()->FindPropertyByName(PropertyName));
		if (!ArrayProperty)
		{
			if (const AActor* Actor = Cast<AActor>(Object))
			{
				for (const UActorComponent* Component : Actor->GetComponents())
				{
					if (GetNameArray(Component, PropertyName, OutNames))
					{
						return true;
					}
				}
			}
			return false;
		}

		const FNameProperty* NameInner = CastField<FNameProperty>(ArrayProperty->Inner);
		const FStrProperty* StringInner = CastField<FStrProperty>(ArrayProperty->Inner);
		if (!NameInner && !StringInner)
		{
			return false;
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			OutNames.Add(NameInner ? NameInner->GetPropertyValue(Helper.GetRawPtr(Index)) : FName(*StringInner->GetPropertyValue(Helper.GetRawPtr(Index))));
		}
		return true;
	}

	bool CallBoolFunction(UObject* Object, FName FunctionName, bool& OutValue)
	{
		UFunction* Function = Object ? Object->FindFunction(FunctionName) : nullptr;
		if (!Function || Function->ParmsSize <= 0)
		{
			return false;
		}

		uint8* Params = static_cast<uint8*>(FMemory_Alloca_Aligned(Function->ParmsSize, Function->GetMinAlignment()));
		FMemory::Memzero(Params, Function->ParmsSize);
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->InitializeValue_InContainer(Params);
		}

		Object->ProcessEvent(Function, Params);

		bool bFound = false;
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			const FBoolProperty* BoolProperty = CastField<FBoolProperty>(*It);
			if (!bFound && BoolProperty && It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
			{
				OutValue = BoolProperty->GetPropertyValue_InContainer(Params);
				bFound = true;
			}
			It->DestroyValue_InContainer(Params);
		}
		return bFound;
	}
}
