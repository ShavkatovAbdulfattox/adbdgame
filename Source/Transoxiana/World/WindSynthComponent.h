#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "WindSynthComponent.generated.h"

/**
 * Procedural steppe wind: filtered noise with slow, irregular gusts.
 * Gives the prototype an ambient soundscape before any audio files are imported.
 * Runs on the audio render thread; only touch the members below from OnGenerateAudio.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class TRANSOXIANA_API UWindSynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UWindSynthComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	FRandomStream Random;
	float SampleRateHz = 48000.f;
	float LowPassA = 0.f;
	float LowPassB = 0.f;
	float GustPhase = 0.f;
	float GustTarget = 0.5f;
	float GustLevel = 0.5f;
	float GustTimer = 0.f;
};
