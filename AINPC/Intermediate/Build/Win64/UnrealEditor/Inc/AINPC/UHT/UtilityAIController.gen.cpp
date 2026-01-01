// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Controller/UtilityAIController.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeUtilityAIController() {}

// ********** Begin Cross Module References ********************************************************
AIMODULE_API UClass* Z_Construct_UClass_AAIController();
AINPC_API UClass* Z_Construct_UClass_AUtilityAIController();
AINPC_API UClass* Z_Construct_UClass_AUtilityAIController_NoRegister();
AINPC_API UClass* Z_Construct_UClass_UNPCMentalState_NoRegister();
UPackage* Z_Construct_UPackage__Script_AINPC();
// ********** End Cross Module References **********************************************************

// ********** Begin Class AUtilityAIController *****************************************************
void AUtilityAIController::StaticRegisterNativesAUtilityAIController()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_AUtilityAIController;
UClass* AUtilityAIController::GetPrivateStaticClass()
{
	using TClass = AUtilityAIController;
	if (!Z_Registration_Info_UClass_AUtilityAIController.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("UtilityAIController"),
			Z_Registration_Info_UClass_AUtilityAIController.InnerSingleton,
			StaticRegisterNativesAUtilityAIController,
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
	return Z_Registration_Info_UClass_AUtilityAIController.InnerSingleton;
}
UClass* Z_Construct_UClass_AUtilityAIController_NoRegister()
{
	return AUtilityAIController::GetPrivateStaticClass();
}
struct Z_Construct_UClass_AUtilityAIController_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "HideCategories", "Collision Rendering Transformation" },
		{ "IncludePath", "Controller/UtilityAIController.h" },
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MentalState_MetaData[] = {
		{ "Category", "AI" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe5\xa3\xb0\xe6\x98\x8e\xe5\x8f\x98\xe9\x87\x8f\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xa3\xb0\xe6\x98\x8e\xe5\x8f\x98\xe9\x87\x8f" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_MentalState;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AUtilityAIController>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_MentalState = { "MentalState", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AUtilityAIController, MentalState), Z_Construct_UClass_UNPCMentalState_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MentalState_MetaData), NewProp_MentalState_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AUtilityAIController_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_MentalState,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AUtilityAIController_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_AUtilityAIController_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AAIController,
	(UObject* (*)())Z_Construct_UPackage__Script_AINPC,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AUtilityAIController_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_AUtilityAIController_Statics::ClassParams = {
	&AUtilityAIController::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_AUtilityAIController_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_AUtilityAIController_Statics::PropPointers),
	0,
	0x009003A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AUtilityAIController_Statics::Class_MetaDataParams), Z_Construct_UClass_AUtilityAIController_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_AUtilityAIController()
{
	if (!Z_Registration_Info_UClass_AUtilityAIController.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AUtilityAIController.OuterSingleton, Z_Construct_UClass_AUtilityAIController_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_AUtilityAIController.OuterSingleton;
}
AUtilityAIController::AUtilityAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(AUtilityAIController);
AUtilityAIController::~AUtilityAIController() {}
// ********** End Class AUtilityAIController *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AUtilityAIController, AUtilityAIController::StaticClass, TEXT("AUtilityAIController"), &Z_Registration_Info_UClass_AUtilityAIController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AUtilityAIController), 1562176218U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_135658546(TEXT("/Script/AINPC"),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
