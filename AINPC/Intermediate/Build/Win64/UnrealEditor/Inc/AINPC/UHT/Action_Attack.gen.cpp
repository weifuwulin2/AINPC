// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Actions/Action_Attack.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeAction_Attack() {}

// ********** Begin Cross Module References ********************************************************
AINPC_API UClass* Z_Construct_UClass_UAction_Attack();
AINPC_API UClass* Z_Construct_UClass_UAction_Attack_NoRegister();
AINPC_API UClass* Z_Construct_UClass_UUtilityActionBase();
UPackage* Z_Construct_UPackage__Script_AINPC();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UAction_Attack ***********************************************************
void UAction_Attack::StaticRegisterNativesUAction_Attack()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UAction_Attack;
UClass* UAction_Attack::GetPrivateStaticClass()
{
	using TClass = UAction_Attack;
	if (!Z_Registration_Info_UClass_UAction_Attack.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("Action_Attack"),
			Z_Registration_Info_UClass_UAction_Attack.InnerSingleton,
			StaticRegisterNativesUAction_Attack,
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
	return Z_Registration_Info_UClass_UAction_Attack.InnerSingleton;
}
UClass* Z_Construct_UClass_UAction_Attack_NoRegister()
{
	return UAction_Attack::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UAction_Attack_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \xe5\x85\xb7\xe4\xbd\x93\xe7\x9a\x84\xe6\x94\xbb\xe5\x87\xbb\xe8\xa1\x8c\xe4\xb8\xba\n * \xe9\x80\xbb\xe8\xbe\x91\xef\xbc\x9a\xe6\x84\xa4\xe6\x80\x92\xe5\x80\xbc\xe8\xb6\x8a\xe9\xab\x98\xef\xbc\x8c\xe5\xbe\x97\xe5\x88\x86\xe8\xb6\x8a\xe9\xab\x98\n */" },
#endif
		{ "IncludePath", "Actions/Action_Attack.h" },
		{ "ModuleRelativePath", "Public/Actions/Action_Attack.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\x85\xb7\xe4\xbd\x93\xe7\x9a\x84\xe6\x94\xbb\xe5\x87\xbb\xe8\xa1\x8c\xe4\xb8\xba\n\xe9\x80\xbb\xe8\xbe\x91\xef\xbc\x9a\xe6\x84\xa4\xe6\x80\x92\xe5\x80\xbc\xe8\xb6\x8a\xe9\xab\x98\xef\xbc\x8c\xe5\xbe\x97\xe5\x88\x86\xe8\xb6\x8a\xe9\xab\x98" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAction_Attack>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UAction_Attack_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UUtilityActionBase,
	(UObject* (*)())Z_Construct_UPackage__Script_AINPC,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAction_Attack_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UAction_Attack_Statics::ClassParams = {
	&UAction_Attack::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAction_Attack_Statics::Class_MetaDataParams), Z_Construct_UClass_UAction_Attack_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UAction_Attack()
{
	if (!Z_Registration_Info_UClass_UAction_Attack.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAction_Attack.OuterSingleton, Z_Construct_UClass_UAction_Attack_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UAction_Attack.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UAction_Attack);
UAction_Attack::~UAction_Attack() {}
// ********** End Class UAction_Attack *************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h__Script_AINPC_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UAction_Attack, UAction_Attack::StaticClass, TEXT("UAction_Attack"), &Z_Registration_Info_UClass_UAction_Attack, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAction_Attack), 256413416U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h__Script_AINPC_1063531983(TEXT("/Script/AINPC"),
	Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h__Script_AINPC_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h__Script_AINPC_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
