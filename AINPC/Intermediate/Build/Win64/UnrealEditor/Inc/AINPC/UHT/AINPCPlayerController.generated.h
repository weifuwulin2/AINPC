// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "AINPCPlayerController.h"

#ifdef AINPC_AINPCPlayerController_generated_h
#error "AINPCPlayerController.generated.h already included, missing '#pragma once' in AINPCPlayerController.h"
#endif
#define AINPC_AINPCPlayerController_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class AAINPCPlayerController ***************************************************
AINPC_API UClass* Z_Construct_UClass_AAINPCPlayerController_NoRegister();

#define FID_AINPC_Source_AINPC_AINPCPlayerController_h_19_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAAINPCPlayerController(); \
	friend struct Z_Construct_UClass_AAINPCPlayerController_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend AINPC_API UClass* Z_Construct_UClass_AAINPCPlayerController_NoRegister(); \
public: \
	DECLARE_CLASS2(AAINPCPlayerController, APlayerController, COMPILED_IN_FLAGS(CLASS_Abstract | CLASS_Config), CASTCLASS_None, TEXT("/Script/AINPC"), Z_Construct_UClass_AAINPCPlayerController_NoRegister) \
	DECLARE_SERIALIZER(AAINPCPlayerController)


#define FID_AINPC_Source_AINPC_AINPCPlayerController_h_19_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API AAINPCPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	AAINPCPlayerController(AAINPCPlayerController&&) = delete; \
	AAINPCPlayerController(const AAINPCPlayerController&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, AAINPCPlayerController); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AAINPCPlayerController); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AAINPCPlayerController) \
	NO_API virtual ~AAINPCPlayerController();


#define FID_AINPC_Source_AINPC_AINPCPlayerController_h_16_PROLOG
#define FID_AINPC_Source_AINPC_AINPCPlayerController_h_19_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_AINPC_Source_AINPC_AINPCPlayerController_h_19_INCLASS_NO_PURE_DECLS \
	FID_AINPC_Source_AINPC_AINPCPlayerController_h_19_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class AAINPCPlayerController;

// ********** End Class AAINPCPlayerController *****************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_AINPC_Source_AINPC_AINPCPlayerController_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
