#include "RetroScreenTextureBridge.h"

#include "Engine/Texture2D.h"
#include "RetroScreenVideoBridge.h"

UTexture2D* FRetroScreenTextureBridge::EnsureTexture(UObject* Outer, UTexture2D* ExistingTexture, int32 Width, int32 Height) const
{
    if (ExistingTexture != nullptr && ExistingTexture->GetSizeX() == Width && ExistingTexture->GetSizeY() == Height)
    {
        return ExistingTexture;
    }

    if (Outer == nullptr || Width <= 0 || Height <= 0)
    {
        return nullptr;
    }

    UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (NewTexture == nullptr)
    {
        return nullptr;
    }

    NewTexture->SRGB = false;
    NewTexture->Filter = TF_Nearest;
    NewTexture->UpdateResource();

    return NewTexture;
}

bool FRetroScreenTextureBridge::UploadFrame(UTexture2D* Texture, const FRetroScreenVideoFrame& Frame) const
{
    if (Texture == nullptr || !Frame.IsValid())
    {
        return false;
    }

    if (Texture->GetSizeX() != Frame.Width || Texture->GetSizeY() != Frame.Height)
    {
        return false;
    }

    FTexturePlatformData* PlatformData = Texture->GetPlatformData();
    if (PlatformData == nullptr || PlatformData->Mips.Num() == 0)
    {
        return false;
    }

    FTexture2DMipMap& Mip = PlatformData->Mips[0];
    const int32 DestPitch = Frame.Width * 4;
    const int64 DestSize = static_cast<int64>(DestPitch) * static_cast<int64>(Frame.Height);

    uint8* DestPixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
    if (DestPixels == nullptr)
    {
        Mip.BulkData.Unlock();
        return false;
    }

    FMemory::Memzero(DestPixels, static_cast<SIZE_T>(DestSize));

    const uint8* SrcPixels = Frame.Pixels.GetData();
    const int32 CopyBytesPerRow = FMath::Min(Frame.Pitch, DestPitch);
    for (int32 Row = 0; Row < Frame.Height; ++Row)
    {
        const int64 SrcOffset = static_cast<int64>(Row) * static_cast<int64>(Frame.Pitch);
        const int64 DestOffset = static_cast<int64>(Row) * static_cast<int64>(DestPitch);
        FMemory::Memcpy(DestPixels + DestOffset, SrcPixels + SrcOffset, static_cast<SIZE_T>(CopyBytesPerRow));
    }

    Mip.BulkData.Unlock();
    Texture->UpdateResource();

    return true;
}
