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
AINPC_API UClass* Z_Construct_UClass_UUtilityActionBase_NoRegister();
AINPC_API UScriptStruct* Z_Construct_UScriptStruct_FMentalStateJson();
COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
UPackage* Z_Construct_UPackage__Script_AINPC();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FMentalStateJson **************************************************
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FMentalStateJson;
class UScriptStruct* FMentalStateJson::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FMentalStateJson.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FMentalStateJson.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FMentalStateJson, (UObject*)Z_Construct_UPackage__Script_AINPC(), TEXT("MentalStateJson"));
	}
	return Z_Registration_Info_UScriptStruct_FMentalStateJson.OuterSingleton;
}
struct Z_Construct_UScriptStruct_FMentalStateJson_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe5\xae\x9a\xe4\xb9\x89\xe4\xb8\x80\xe4\xb8\xaa\xe7\xbb\x93\xe6\x9e\x84\xe4\xbd\x93\xe6\x9d\xa5\xe5\x8c\xb9\xe9\x85\x8d JSON \xe6\xa0\xbc\xe5\xbc\x8f\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xae\x9a\xe4\xb9\x89\xe4\xb8\x80\xe4\xb8\xaa\xe7\xbb\x93\xe6\x9e\x84\xe4\xbd\x93\xe6\x9d\xa5\xe5\x8c\xb9\xe9\x85\x8d JSON \xe6\xa0\xbc\xe5\xbc\x8f" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Anger_MetaData[] = {
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Fear_MetaData[] = {
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Confidence_MetaData[] = {
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SocialBattery_MetaData[] = {
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Hunger_MetaData[] = {
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Anger;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Fear;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Confidence;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_SocialBattery;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Hunger;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FMentalStateJson>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
};
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Anger = { "Anger", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMentalStateJson, Anger), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Anger_MetaData), NewProp_Anger_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Fear = { "Fear", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMentalStateJson, Fear), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Fear_MetaData), NewProp_Fear_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Confidence = { "Confidence", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMentalStateJson, Confidence), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Confidence_MetaData), NewProp_Confidence_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_SocialBattery = { "SocialBattery", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMentalStateJson, SocialBattery), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SocialBattery_MetaData), NewProp_SocialBattery_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Hunger = { "Hunger", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMentalStateJson, Hunger), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Hunger_MetaData), NewProp_Hunger_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FMentalStateJson_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Anger,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Fear,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Confidence,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_SocialBattery,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewProp_Hunger,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMentalStateJson_Statics::PropPointers) < 2048);
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FMentalStateJson_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_AINPC,
	nullptr,
	&NewStructOps,
	"MentalStateJson",
	Z_Construct_UScriptStruct_FMentalStateJson_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMentalStateJson_Statics::PropPointers),
	sizeof(FMentalStateJson),
	alignof(FMentalStateJson),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMentalStateJson_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FMentalStateJson_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FMentalStateJson()
{
	if (!Z_Registration_Info_UScriptStruct_FMentalStateJson.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FMentalStateJson.InnerSingleton, Z_Construct_UScriptStruct_FMentalStateJson_Statics::StructParams);
	}
	return Z_Registration_Info_UScriptStruct_FMentalStateJson.InnerSingleton;
}
// ********** End ScriptStruct FMentalStateJson ****************************************************

// ********** Begin Class AUtilityAIController Function RequestUpdateFromLLM ***********************
struct Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics
{
	struct UtilityAIController_eventRequestUpdateFromLLM_Parms
	{
		FString Prompt;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "AI|Brain" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe8\xa7\xa6\xe5\x8f\x91 HTTP \xe8\xaf\xb7\xe6\xb1\x82\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe8\xa7\xa6\xe5\x8f\x91 HTTP \xe8\xaf\xb7\xe6\xb1\x82" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_Prompt;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::NewProp_Prompt = { "Prompt", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UtilityAIController_eventRequestUpdateFromLLM_Parms, Prompt), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::NewProp_Prompt,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_AUtilityAIController, nullptr, "RequestUpdateFromLLM", Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::PropPointers), sizeof(Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::UtilityAIController_eventRequestUpdateFromLLM_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::Function_MetaDataParams), Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::UtilityAIController_eventRequestUpdateFromLLM_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(AUtilityAIController::execRequestUpdateFromLLM)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_Prompt);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->RequestUpdateFromLLM(Z_Param_Prompt);
	P_NATIVE_END;
}
// ********** End Class AUtilityAIController Function RequestUpdateFromLLM *************************

// ********** Begin Class AUtilityAIController *****************************************************
void AUtilityAIController::StaticRegisterNativesAUtilityAIController()
{
	UClass* Class = AUtilityAIController::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "RequestUpdateFromLLM", &AUtilityAIController::execRequestUpdateFromLLM },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
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
		{ "Comment", "// === \xe6\x95\xb0\xe6\x8d\xae\xe5\xb1\x82 ===\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "=== \xe6\x95\xb0\xe6\x8d\xae\xe5\xb1\x82 ===" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ActionClasses_MetaData[] = {
		{ "Category", "AI" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe5\x8a\xa8\xe4\xbd\x9c\xe9\x85\x8d\xe7\xbd\xae\xe8\xa1\xa8\xef\xbc\x9a\xe5\x9c\xa8\xe8\x93\x9d\xe5\x9b\xbe\xe9\x87\x8c\xe6\x8a\x8a Action_Attack \xe5\xa1\xab\xe8\xbf\x9b\xe5\x8e\xbb\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\x8a\xa8\xe4\xbd\x9c\xe9\x85\x8d\xe7\xbd\xae\xe8\xa1\xa8\xef\xbc\x9a\xe5\x9c\xa8\xe8\x93\x9d\xe5\x9b\xbe\xe9\x87\x8c\xe6\x8a\x8a Action_Attack \xe5\xa1\xab\xe8\xbf\x9b\xe5\x8e\xbb" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AvailableActions_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe8\xbf\x90\xe8\xa1\x8c\xe6\x97\xb6\xe5\xae\x9e\xe4\xbe\x8b\xe5\x8c\x96\xe7\x9a\x84\xe5\x8a\xa8\xe4\xbd\x9c\xe5\x88\x97\xe8\xa1\xa8\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe8\xbf\x90\xe8\xa1\x8c\xe6\x97\xb6\xe5\xae\x9e\xe4\xbe\x8b\xe5\x8c\x96\xe7\x9a\x84\xe5\x8a\xa8\xe4\xbd\x9c\xe5\x88\x97\xe8\xa1\xa8" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CurrentAction_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xe5\xbd\x93\xe5\x89\x8d\xe6\xad\xa3\xe5\x9c\xa8\xe6\x89\xa7\xe8\xa1\x8c\xe7\x9a\x84\xe5\x8a\xa8\xe4\xbd\x9c\n" },
#endif
		{ "ModuleRelativePath", "Controller/UtilityAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xbd\x93\xe5\x89\x8d\xe6\xad\xa3\xe5\x9c\xa8\xe6\x89\xa7\xe8\xa1\x8c\xe7\x9a\x84\xe5\x8a\xa8\xe4\xbd\x9c" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_MentalState;
	static const UECodeGen_Private::FClassPropertyParams NewProp_ActionClasses_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ActionClasses;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_AvailableActions_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_AvailableActions;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CurrentAction;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_AUtilityAIController_RequestUpdateFromLLM, "RequestUpdateFromLLM" }, // 2325442135
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AUtilityAIController>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_MentalState = { "MentalState", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AUtilityAIController, MentalState), Z_Construct_UClass_UNPCMentalState_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MentalState_MetaData), NewProp_MentalState_MetaData) };
const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_ActionClasses_Inner = { "ActionClasses", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UClass, Z_Construct_UClass_UUtilityActionBase_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_ActionClasses = { "ActionClasses", nullptr, (EPropertyFlags)0x0014000000010001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AUtilityAIController, ActionClasses), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ActionClasses_MetaData), NewProp_ActionClasses_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_AvailableActions_Inner = { "AvailableActions", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UUtilityActionBase_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_AvailableActions = { "AvailableActions", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AUtilityAIController, AvailableActions), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AvailableActions_MetaData), NewProp_AvailableActions_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AUtilityAIController_Statics::NewProp_CurrentAction = { "CurrentAction", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AUtilityAIController, CurrentAction), Z_Construct_UClass_UUtilityActionBase_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CurrentAction_MetaData), NewProp_CurrentAction_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AUtilityAIController_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_MentalState,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_ActionClasses_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_ActionClasses,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_AvailableActions_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_AvailableActions,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AUtilityAIController_Statics::NewProp_CurrentAction,
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
	FuncInfo,
	Z_Construct_UClass_AUtilityAIController_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
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
DEFINE_VTABLE_PTR_HELPER_CTOR(AUtilityAIController);
AUtilityAIController::~AUtilityAIController() {}
// ********** End Class AUtilityAIController *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FMentalStateJson::StaticStruct, Z_Construct_UScriptStruct_FMentalStateJson_Statics::NewStructOps, TEXT("MentalStateJson"), &Z_Registration_Info_UScriptStruct_FMentalStateJson, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMentalStateJson), 1913653016U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AUtilityAIController, AUtilityAIController::StaticClass, TEXT("AUtilityAIController"), &Z_Registration_Info_UClass_AUtilityAIController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AUtilityAIController), 2053503835U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_4074839580(TEXT("/Script/AINPC"),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Controller_UtilityAIController_h__Script_AINPC_Statics::ScriptStructInfo),
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
