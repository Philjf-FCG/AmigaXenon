import unreal

ASSET_OBJECT_PATH = "/Game/RetroScreen/UI/WBP_RetroScreenPauseMenu.WBP_RetroScreenPauseMenu"
PARENT_CLASS_PATH = "/Script/RetroScreen.RetroScreenPauseMenuWidget"


def fail(message: str) -> None:
    unreal.log_error(f"[RetroScreen] {message}")
    raise RuntimeError(message)


def main() -> None:
    unreal.log("[RetroScreen] Validating pause menu widget asset...")

    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        fail(f"Unable to load parent class: {PARENT_CLASS_PATH}")

    asset = unreal.EditorAssetLibrary.load_asset(ASSET_OBJECT_PATH)
    if asset is None:
        fail(f"Unable to load asset: {ASSET_OBJECT_PATH}")

    generated_class = asset.generated_class()
    if generated_class is None:
        fail("Widget blueprint has no generated class")

    cdo = unreal.get_default_object(generated_class)
    if cdo is None:
        fail("Unable to access generated class default object")
    unreal.log("[RetroScreen] Pause menu widget validation passed.")


if __name__ == "__main__":
    main()
