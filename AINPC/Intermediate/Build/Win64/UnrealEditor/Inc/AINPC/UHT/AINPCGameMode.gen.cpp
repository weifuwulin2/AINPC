// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "AINPCGameMode.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeAINPCGameMode() {}

// ********** Begin Cross Module References ********************************************************
AINPC_API UClass* Z_Construct_UClass_AAINPCGameMode();
AINPC_API UClass* Z_Construct_UClass_AAINPCGameMode_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
UPackage* Z_Construct_UPackage__Script_AINPC();
// ********** End Cross Module References **********************************************************

// ********** Begin Class AAINPCGameMode ***********************************************************
void AAINPCGameMode::StaticRegisterNativesAAINPCGameMode()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_AAINPCGameMode;
UClass* AAINPCGameMode::GetPrivateStaticClass()
{
	using TClass = AAINPCGameMode;
	if (!Z_Registration_Info_UClass_AAINPCGameMode.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("AINPCGameMode"),
			Z_Registration_Info_UClass_AAINPCGameMode.InnerSingleton,
			StaticRegisterNativesAAINPCGameMode,
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
	return Z_Registration_Info_UClass_AAINPCGameMode.InnerSingleton;
}
UClass* Z_Construct_UClass_AAINPCGameMode_NoRegister()
{
	return AAINPCGameMode::GetPrivateStaticClass();
}
struct Z_Construct_UClass_AAINPCGameMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n *  Simple GameMode for a third person game\n */" },
#endif
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "AINPCGameMode.h" },
		{ "ModuleRelativePath", "AINPCGameMode.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Simple GameMode for a third person game" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AAINPCGameMode>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_AAINPCGameMode_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AGameModeBase,
	(UObject* (*)())Z_Construct_UPackage__Script_AINPC,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AAINPCGameMode_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_AAINPCGameMode_Statics::ClassParams = {
	&AAINPCGameMode::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x008003ADu,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AAINPCGameMode_Statics::Class_MetaDataParams), Z_Construct_UClass_AAINPCGameMode_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_AAINPCGameMode()
{
	if (!Z_Registration_Info_UClass_AAINPCGameMode.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AAINPCGameMode.OuterSingleton, Z_Construct_UClass_AAINPCGameMode_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_AAINPCGameMode.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(AAINPCGameMode);
AAINPCGameMode::~AAINPCGameMode() {}
// ********** End Class AAINPCGameMode *************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_AINPCGameMode_h__Script_AINPC_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AAINPCGameMode, AAINPCGameMode::StaticClass, TEXT("AAINPCGameMode"), &Z_Registration_Info_UClass_AAINPCGameMode, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AAINPCGameMode), 1089971441U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_AINPCGameMode_h__Script_AINPC_4022351911(TEXT("/Script/AINPC"),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_AINPCGameMode_h__Script_AINPC_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_AINPCGameMode_h__Script_AINPC_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
