// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "UtilityAI/UNPCMentalState.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeUNPCMentalState() {}

// ********** Begin Cross Module References ********************************************************
AINPC_API UClass* Z_Construct_UClass_UNPCMentalState();
AINPC_API UClass* Z_Construct_UClass_UNPCMentalState_NoRegister();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
UPackage* Z_Construct_UPackage__Script_AINPC();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNPCMentalState Function ResetState **************************************
struct Z_Construct_UFunction_UNPCMentalState_ResetState_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Mental State" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe8\xbe\x85\xe5\x8a\xa9\xe5\x87\xbd\xe6\x95\xb0\xef\xbc\x9a\xe9\x87\x8d\xe7\xbd\xae\xe7\x8a\xb6\xe6\x80\x81\n" },
#endif
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe8\xbe\x85\xe5\x8a\xa9\xe5\x87\xbd\xe6\x95\xb0\xef\xbc\x9a\xe9\x87\x8d\xe7\xbd\xae\xe7\x8a\xb6\xe6\x80\x81" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UNPCMentalState_ResetState_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UNPCMentalState, nullptr, "ResetState", nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UNPCMentalState_ResetState_Statics::Function_MetaDataParams), Z_Construct_UFunction_UNPCMentalState_ResetState_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_UNPCMentalState_ResetState()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UNPCMentalState_ResetState_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UNPCMentalState::execResetState)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ResetState();
	P_NATIVE_END;
}
// ********** End Class UNPCMentalState Function ResetState ****************************************

// ********** Begin Class UNPCMentalState **********************************************************
void UNPCMentalState::StaticRegisterNativesUNPCMentalState()
{
	UClass* Class = UNPCMentalState::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "ResetState", &UNPCMentalState::execResetState },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
FClassRegistrationInfo Z_Registration_Info_UClass_UNPCMentalState;
UClass* UNPCMentalState::GetPrivateStaticClass()
{
	using TClass = UNPCMentalState;
	if (!Z_Registration_Info_UClass_UNPCMentalState.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("NPCMentalState"),
			Z_Registration_Info_UClass_UNPCMentalState.InnerSingleton,
			StaticRegisterNativesUNPCMentalState,
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
	return Z_Registration_Info_UClass_UNPCMentalState.InnerSingleton;
}
UClass* Z_Construct_UClass_UNPCMentalState_NoRegister()
{
	return UNPCMentalState::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNPCMentalState_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "UtilityAI/UNPCMentalState.h" },
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Anger_MetaData[] = {
		{ "Category", "Mental State" },
		{ "ClampMax", "1.0" },
		{ "ClampMin", "0.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// === \xe6\x83\x85\xe7\xbb\xaa\xe7\xbb\xb4\xe5\xba\xa6 (Emotional Dimensions) ===\n" },
#endif
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "=== \xe6\x83\x85\xe7\xbb\xaa\xe7\xbb\xb4\xe5\xba\xa6 (Emotional Dimensions) ===" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Fear_MetaData[] = {
		{ "Category", "Mental State" },
		{ "ClampMax", "1.0" },
		{ "ClampMin", "0.0" },
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Confidence_MetaData[] = {
		{ "Category", "Mental State" },
		{ "ClampMax", "1.0" },
		{ "ClampMin", "0.0" },
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SocialBattery_MetaData[] = {
		{ "Category", "Mental State" },
		{ "ClampMax", "1.0" },
		{ "ClampMin", "0.0" },
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Hunger_MetaData[] = {
		{ "Category", "Physical State" },
		{ "ClampMax", "1.0" },
		{ "ClampMin", "0.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// === \xe7\x94\x9f\xe7\x90\x86\xe7\xbb\xb4\xe5\xba\xa6 (Physical Dimensions) ===\n" },
#endif
		{ "ModuleRelativePath", "Public/UtilityAI/UNPCMentalState.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "=== \xe7\x94\x9f\xe7\x90\x86\xe7\xbb\xb4\xe5\xba\xa6 (Physical Dimensions) ===" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Anger;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Fear;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Confidence;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_SocialBattery;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Hunger;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UNPCMentalState_ResetState, "ResetState" }, // 1420958596
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNPCMentalState>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Anger = { "Anger", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNPCMentalState, Anger), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Anger_MetaData), NewProp_Anger_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Fear = { "Fear", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNPCMentalState, Fear), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Fear_MetaData), NewProp_Fear_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Confidence = { "Confidence", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNPCMentalState, Confidence), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Confidence_MetaData), NewProp_Confidence_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNPCMentalState_Statics::NewProp_SocialBattery = { "SocialBattery", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNPCMentalState, SocialBattery), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SocialBattery_MetaData), NewProp_SocialBattery_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Hunger = { "Hunger", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNPCMentalState, Hunger), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Hunger_MetaData), NewProp_Hunger_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNPCMentalState_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Anger,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Fear,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Confidence,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNPCMentalState_Statics::NewProp_SocialBattery,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNPCMentalState_Statics::NewProp_Hunger,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNPCMentalState_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UNPCMentalState_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_AINPC,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNPCMentalState_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNPCMentalState_Statics::ClassParams = {
	&UNPCMentalState::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UNPCMentalState_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UNPCMentalState_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNPCMentalState_Statics::Class_MetaDataParams), Z_Construct_UClass_UNPCMentalState_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UNPCMentalState()
{
	if (!Z_Registration_Info_UClass_UNPCMentalState.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNPCMentalState.OuterSingleton, Z_Construct_UClass_UNPCMentalState_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNPCMentalState.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UNPCMentalState);
UNPCMentalState::~UNPCMentalState() {}
// ********** End Class UNPCMentalState ************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h__Script_AINPC_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNPCMentalState, UNPCMentalState::StaticClass, TEXT("UNPCMentalState"), &Z_Registration_Info_UClass_UNPCMentalState, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNPCMentalState), 1644486258U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h__Script_AINPC_1064044221(TEXT("/Script/AINPC"),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h__Script_AINPC_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h__Script_AINPC_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
