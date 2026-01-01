// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "AINPCGameMode.h"

#ifdef AINPC_AINPCGameMode_generated_h
#error "AINPCGameMode.generated.h already included, missing '#pragma once' in AINPCGameMode.h"
#endif
#define AINPC_AINPCGameMode_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class AAINPCGameMode ***********************************************************
AINPC_API UClass* Z_Construct_UClass_AAINPCGameMode_NoRegister();

#define FID_AINPC_Source_AINPC_AINPCGameMode_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAAINPCGameMode(); \
	friend struct Z_Construct_UClass_AAINPCGameMode_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend AINPC_API UClass* Z_Construct_UClass_AAINPCGameMode_NoRegister(); \
public: \
	DECLARE_CLASS2(AAINPCGameMode, AGameModeBase, COMPILED_IN_FLAGS(CLASS_Abstract | CLASS_Transient | CLASS_Config), CASTCLASS_None, TEXT("/Script/AINPC"), Z_Construct_UClass_AAINPCGameMode_NoRegister) \
	DECLARE_SERIALIZER(AAINPCGameMode)


#define FID_AINPC_Source_AINPC_AINPCGameMode_h_15_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	AAINPCGameMode(AAINPCGameMode&&) = delete; \
	AAINPCGameMode(const AAINPCGameMode&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AAINPCGameMode); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AAINPCGameMode); \
	DEFINE_ABSTRACT_DEFAULT_CONSTRUCTOR_CALL(AAINPCGameMode) \
	NO_API virtual ~AAINPCGameMode();


#define FID_AINPC_Source_AINPC_AINPCGameMode_h_12_PROLOG
#define FID_AINPC_Source_AINPC_AINPCGameMode_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_AINPC_Source_AINPC_AINPCGameMode_h_15_INCLASS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_AINPCGameMode_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class AAINPCGameMode;

// ********** End Class AAINPCGameMode *************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_AINPC_Source_AINPC_AINPCGameMode_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
