#include "World/WindSynthComponent.h"

UWindSynthComponent::UWindSynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Non-spatial ambience: plays equally in both ears wherever the listener is.
	bAllowSpatialization = false;
	Random.Initialize(1387);
}

bool UWindSynthComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(SampleRate > 0 ? SampleRate : 48000);
	return true;
}

int32 UWindSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const float Dt = 1.f / SampleRateHz;
	for (int32 i = 0; i < NumSamples; ++i)
	{
		// Every few seconds pick a new gust strength and glide toward it.
		GustTimer -= Dt;
		if (GustTimer <= 0.f)
		{
			GustTimer = Random.FRandRange(2.5f, 7.f);
			GustTarget = Random.FRandRange(0.25f, 1.f);
		}
		GustLevel += (GustTarget - GustLevel) * Dt * 0.6f;
		GustPhase += Dt * 0.37f;
		const float Flutter = 0.85f + 0.15f * FMath::Sin(GustPhase * 2.f * UE_PI * 3.1f);

		// White noise -> two one-pole low-pass filters. Stronger gusts open the filter,
		// which makes the wind "whistle" brighter.
		const float White = Random.FRandRange(-1.f, 1.f);
		const float Cutoff = 0.004f + 0.03f * GustLevel;
		LowPassA += Cutoff * (White - LowPassA);
		LowPassB += Cutoff * (LowPassA - LowPassB);

		const float Sample = LowPassB * 9.f * GustLevel * Flutter;
		OutAudio[i] = FMath::Clamp(Sample, -0.6f, 0.6f);
	}
	return NumSamples;
}
