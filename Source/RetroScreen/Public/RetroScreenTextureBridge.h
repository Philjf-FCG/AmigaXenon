#pragma once

#include "CoreMinimal.h"

class UTexture2D;
struct FRetroScreenVideoFrame;

class RETROSCREEN_API FRetroScreenTextureBridge
{
public:
    UTexture2D* EnsureTexture(UObject* Outer, UTexture2D* ExistingTexture, int32 Width, int32 Height) const;
    bool UploadFrame(UTexture2D* Texture, const FRetroScreenVideoFrame& Frame) const;
};
