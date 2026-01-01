// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Controller/UtilityAIController.h"

#ifdef AINPC_UtilityAIController_generated_h
#error "UtilityAIController.generated.h already included, missing '#pragma once' in UtilityAIController.h"
#endif
#define AINPC_UtilityAIController_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class AUtilityAIController *****************************************************
AINPC_API UClass* Z_Construct_UClass_AUtilityAIController_NoRegister();

#define FID_AINPC_Source_AINPC_Controller_UtilityAIController_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAUtilityAIController(); \
	friend struct Z_Construct_UClass_AUtilityAIController_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend AINPC_API UClass* Z_Construct_UClass_AUtilityAIController_NoRegister(); \
public: \
	DECLARE_CLASS2(AUtilityAIController, AAIController, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/AINPC"), Z_Construct_UClass_AUtilityAIController_NoRegister) \
	DECLARE_SERIALIZER(AUtilityAIController)


#define FID_AINPC_Source_AINPC_Controller_UtilityAIController_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AUtilityAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	AUtilityAIController(AUtilityAIController&&) = delete; \
	AUtilityAIController(const AUtilityAIController&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AUtilityAIController); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AUtilityAIController); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AUtilityAIController) \
	NO_API virtual ~AUtilityAIController();


#define FID_AINPC_Source_AINPC_Controller_UtilityAIController_h_13_PROLOG
#define FID_AINPC_Source_AINPC_Controller_UtilityAIController_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_AINPC_Source_AINPC_Controller_UtilityAIController_h_16_INCLASS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_Controller_UtilityAIController_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class AUtilityAIController;

// ********** End Class AUtilityAIController *******************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_AINPC_Source_AINPC_Controller_UtilityAIController_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
