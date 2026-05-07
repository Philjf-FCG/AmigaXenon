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

    const uint8* SrcPixels = Frame.Pixels.GetData();

    // Fast path: when source and dest strides match the entire frame is contiguous.
    if (Frame.Pitch == DestPitch)
    {
        FMemory::Memcpy(DestPixels, SrcPixels, static_cast<SIZE_T>(DestSize));
    }
    else
    {
        // Source has padding rows; copy each row individually and zero any trailing columns.
        const int32 CopyBytesPerRow = FMath::Min(Frame.Pitch, DestPitch);
        const int32 PadBytesPerRow = DestPitch - CopyBytesPerRow;
        for (int32 Row = 0; Row < Frame.Height; ++Row)
        {
            uint8* Dest = DestPixels + static_cast<int64>(Row) * DestPitch;
            const uint8* Src = SrcPixels + static_cast<int64>(Row) * Frame.Pitch;
            FMemory::Memcpy(Dest, Src, static_cast<SIZE_T>(CopyBytesPerRow));
            if (PadBytesPerRow > 0)
            {
                FMemory::Memzero(Dest + CopyBytesPerRow, static_cast<SIZE_T>(PadBytesPerRow));
            }
        }
    }

    Mip.BulkData.Unlock();
    Texture->UpdateResource();

    return true;
}
