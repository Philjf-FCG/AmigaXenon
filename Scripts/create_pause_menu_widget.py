import unreal

ASSET_NAME = "WBP_RetroScreenPauseMenu"
ASSET_PATH = "/Game/RetroScreen/UI"
ASSET_OBJECT_PATH = f"{ASSET_PATH}/{ASSET_NAME}.{ASSET_NAME}"
PARENT_CLASS_PATH = "/Script/RetroScreen.RetroScreenPauseMenuWidget"


def ensure_folder(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def main() -> None:
    unreal.log("[RetroScreen] Creating pause menu widget blueprint...")

    ensure_folder("/Game/RetroScreen")
    ensure_folder("/Game/RetroScreen/UI")

    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_OBJECT_PATH):
        unreal.log(f"[RetroScreen] Asset already exists: {ASSET_OBJECT_PATH}")
        return

    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        raise RuntimeError(f"Failed to resolve parent class: {PARENT_CLASS_PATH}")

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    created_asset = asset_tools.create_asset(
        ASSET_NAME,
        ASSET_PATH,
        unreal.WidgetBlueprint,
        factory,
    )

    if created_asset is None:
        raise RuntimeError("Widget Blueprint creation returned None")

    unreal.EditorAssetLibrary.save_loaded_asset(created_asset)
    unreal.log(f"[RetroScreen] Created asset: {ASSET_OBJECT_PATH}")


if __name__ == "__main__":
    main()
