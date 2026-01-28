#include "Config/AINPCSettings.h"

UAINPCSettings::UAINPCSettings()
{
	// Default values are set in the header via UPROPERTY initializers.
	// This constructor exists for UObject requirements.
}

const UAINPCSettings* UAINPCSettings::Get()
{
	// GetDefault is safe and always returns a valid pointer.
	// It uses the CDO (Class Default Object) which is always available.
	return GetDefault<UAINPCSettings>();
}
