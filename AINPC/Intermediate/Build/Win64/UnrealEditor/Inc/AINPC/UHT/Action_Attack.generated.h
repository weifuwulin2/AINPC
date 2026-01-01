// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Actions/Action_Attack.h"

#ifdef AINPC_Action_Attack_generated_h
#error "Action_Attack.generated.h already included, missing '#pragma once' in Action_Attack.h"
#endif
#define AINPC_Action_Attack_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UAction_Attack ***********************************************************
AINPC_API UClass* Z_Construct_UClass_UAction_Attack_NoRegister();

#define FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUAction_Attack(); \
	friend struct Z_Construct_UClass_UAction_Attack_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend AINPC_API UClass* Z_Construct_UClass_UAction_Attack_NoRegister(); \
public: \
	DECLARE_CLASS2(UAction_Attack, UUtilityActionBase, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/AINPC"), Z_Construct_UClass_UAction_Attack_NoRegister) \
	DECLARE_SERIALIZER(UAction_Attack)


#define FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAction_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UAction_Attack(UAction_Attack&&) = delete; \
	UAction_Attack(const UAction_Attack&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAction_Attack); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAction_Attack); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAction_Attack) \
	NO_API virtual ~UAction_Attack();


#define FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h_12_PROLOG
#define FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h_15_INCLASS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UAction_Attack;

// ********** End Class UAction_Attack *************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_AINPC_Source_AINPC_Public_Actions_Action_Attack_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
