// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "UtilityAI/UNPCMentalState.h"

#ifdef AINPC_UNPCMentalState_generated_h
#error "UNPCMentalState.generated.h already included, missing '#pragma once' in UNPCMentalState.h"
#endif
#define AINPC_UNPCMentalState_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UNPCMentalState **********************************************************
#define FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execResetState);


AINPC_API UClass* Z_Construct_UClass_UNPCMentalState_NoRegister();

#define FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUNPCMentalState(); \
	friend struct Z_Construct_UClass_UNPCMentalState_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend AINPC_API UClass* Z_Construct_UClass_UNPCMentalState_NoRegister(); \
public: \
	DECLARE_CLASS2(UNPCMentalState, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/AINPC"), Z_Construct_UClass_UNPCMentalState_NoRegister) \
	DECLARE_SERIALIZER(UNPCMentalState)


#define FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	UNPCMentalState(UNPCMentalState&&) = delete; \
	UNPCMentalState(const UNPCMentalState&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UNPCMentalState); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UNPCMentalState); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UNPCMentalState) \
	NO_API virtual ~UNPCMentalState();


#define FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_12_PROLOG
#define FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_INCLASS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UNPCMentalState;

// ********** End Class UNPCMentalState ************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_AINPC_Source_AINPC_Public_UtilityAI_UNPCMentalState_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
