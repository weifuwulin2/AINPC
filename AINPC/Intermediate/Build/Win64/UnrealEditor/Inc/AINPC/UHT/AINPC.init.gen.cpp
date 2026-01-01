// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAINPC_init() {}
	AINPC_API UFunction* Z_Construct_UDelegateFunction_AINPC_OnEnemyDied__DelegateSignature();
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_AINPC;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_AINPC()
	{
		if (!Z_Registration_Info_UPackage__Script_AINPC.OuterSingleton)
		{
			static UObject* (*const SingletonFuncArray[])() = {
				(UObject* (*)())Z_Construct_UDelegateFunction_AINPC_OnEnemyDied__DelegateSignature,
			};
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/AINPC",
				SingletonFuncArray,
				UE_ARRAY_COUNT(SingletonFuncArray),
				PKG_CompiledIn | 0x00000000,
				0x655EFE28,
				0x3976C493,
				METADATA_PARAMS(0, nullptr)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_AINPC.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_AINPC.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_AINPC(Z_Construct_UPackage__Script_AINPC, TEXT("/Script/AINPC"), Z_Registration_Info_UPackage__Script_AINPC, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x655EFE28, 0x3976C493));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
