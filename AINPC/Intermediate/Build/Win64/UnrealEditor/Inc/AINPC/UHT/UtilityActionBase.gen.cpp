// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Base/UtilityActionBase.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeUtilityActionBase() {}

// ********** Begin Cross Module References ********************************************************
AINPC_API UClass* Z_Construct_UClass_UUtilityActionBase();
AINPC_API UClass* Z_Construct_UClass_UUtilityActionBase_NoRegister();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
UPackage* Z_Construct_UPackage__Script_AINPC();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UUtilityActionBase *******************************************************
void UUtilityActionBase::StaticRegisterNativesUUtilityActionBase()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UUtilityActionBase;
UClass* UUtilityActionBase::GetPrivateStaticClass()
{
	using TClass = UUtilityActionBase;
	if (!Z_Registration_Info_UClass_UUtilityActionBase.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("UtilityActionBase"),
			Z_Registration_Info_UClass_UUtilityActionBase.InnerSingleton,
			StaticRegisterNativesUUtilityActionBase,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UUtilityActionBase.InnerSingleton;
}
UClass* Z_Construct_UClass_UUtilityActionBase_NoRegister()
{
	return UUtilityActionBase::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UUtilityActionBase_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Base/UtilityActionBase.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/Base/UtilityActionBase.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ActionName_MetaData[] = {
		{ "Category", "Action Config" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe5\x8a\xa8\xe4\xbd\x9c\xe5\x90\x8d\xe7\xa7\xb0\xef\xbc\x8c\xe6\x96\xb9\xe4\xbe\xbf\xe8\xb0\x83\xe8\xaf\x95\n" },
#endif
		{ "ModuleRelativePath", "Public/Base/UtilityActionBase.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\x8a\xa8\xe4\xbd\x9c\xe5\x90\x8d\xe7\xa7\xb0\xef\xbc\x8c\xe6\x96\xb9\xe4\xbe\xbf\xe8\xb0\x83\xe8\xaf\x95" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ActionName;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UUtilityActionBase>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UUtilityActionBase_Statics::NewProp_ActionName = { "ActionName", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UUtilityActionBase, ActionName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ActionName_MetaData), NewProp_ActionName_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UUtilityActionBase_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UUtilityActionBase_Statics::NewProp_ActionName,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UUtilityActionBase_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UUtilityActionBase_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_AINPC,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UUtilityActionBase_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UUtilityActionBase_Statics::ClassParams = {
	&UUtilityActionBase::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UUtilityActionBase_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UUtilityActionBase_Statics::PropPointers),
	0,
	0x001000A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UUtilityActionBase_Statics::Class_MetaDataParams), Z_Construct_UClass_UUtilityActionBase_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UUtilityActionBase()
{
	if (!Z_Registration_Info_UClass_UUtilityActionBase.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UUtilityActionBase.OuterSingleton, Z_Construct_UClass_UUtilityActionBase_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UUtilityActionBase.OuterSingleton;
}
UUtilityActionBase::UUtilityActionBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UUtilityActionBase);
UUtilityActionBase::~UUtilityActionBase() {}
// ********** End Class UUtilityActionBase *********************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h__Script_AINPC_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UUtilityActionBase, UUtilityActionBase::StaticClass, TEXT("UUtilityActionBase"), &Z_Registration_Info_UClass_UUtilityActionBase, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UUtilityActionBase), 2202318879U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h__Script_AINPC_4071736584(TEXT("/Script/AINPC"),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h__Script_AINPC_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h__Script_AINPC_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
