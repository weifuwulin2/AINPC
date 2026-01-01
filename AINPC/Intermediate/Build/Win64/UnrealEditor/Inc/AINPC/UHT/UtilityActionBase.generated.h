// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Base/UtilityActionBase.h"

#ifdef AINPC_UtilityActionBase_generated_h
#error "UtilityActionBase.generated.h already included, missing '#pragma once' in UtilityActionBase.h"
#endif
#define AINPC_UtilityActionBase_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UUtilityActionBase *******************************************************
AINPC_API UClass* Z_Construct_UClass_UUtilityActionBase_NoRegister();

#define FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h_14_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUUtilityActionBase(); \
	friend struct Z_Construct_UClass_UUtilityActionBase_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend AINPC_API UClass* Z_Construct_UClass_UUtilityActionBase_NoRegister(); \
public: \
	DECLARE_CLASS2(UUtilityActionBase, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/AINPC"), Z_Construct_UClass_UUtilityActionBase_NoRegister) \
	DECLARE_SERIALIZER(UUtilityActionBase)


#define FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h_14_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UUtilityActionBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UUtilityActionBase(UUtilityActionBase&&) = delete; \
	UUtilityActionBase(const UUtilityActionBase&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UUtilityActionBase); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UUtilityActionBase); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UUtilityActionBase) \
	NO_API virtual ~UUtilityActionBase();


#define FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h_11_PROLOG
#define FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h_14_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h_14_INCLASS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h_14_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UUtilityActionBase;

// ********** End Class UUtilityActionBase *********************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_AINPC_Source_AINPC_Public_Base_UtilityActionBase_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
