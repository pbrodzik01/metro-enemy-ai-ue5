import unreal

FOLDER = "/Game/Ocalaly/Wrogowie"
MASTER = "/Game/Ocalaly/M_Ocalaly"
MANNY_TEXTURES = "/Game/AIBehaviorSystem/Demo/Characters/Mannequins/Textures/Manny/"
AI = "/Game/AIBehaviorSystem/Blueprint/AI/Example/"
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def note(text):
    unreal.log_warning("ENEMY: " + text)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Missing " + path)
    return asset


def create_or_load(name, asset_class, factory):
    path = FOLDER + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return load(path)
    return asset_tools.create_asset(name, FOLDER, asset_class, factory)


def material_instance(name, color, roughness, normal):
    instance = create_or_load(name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, load(MASTER))
    mel.set_material_instance_vector_parameter_value(instance, "Kolor", color)
    mel.set_material_instance_scalar_parameter_value(instance, "Szorstkosc", roughness)
    mel.set_material_instance_texture_parameter_value(instance, "Normalna", load(MANNY_TEXTURES + normal))
    mel.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


VARIANTS = [
    {
        "blueprint": "BP_Wrog_SoldierRange",
        "parent": AI + "BP_AI_SoldierRange",
        "materials": [
            ("MI_Wrog_Dystansowy_01", unreal.LinearColor(0.032, 0.038, 0.046, 1.0), 0.8, "T_Manny_01_N"),
            ("MI_Wrog_Dystansowy_02", unreal.LinearColor(0.014, 0.015, 0.017, 1.0), 0.65, "T_Manny_02_N"),
        ],
    },
    {
        "blueprint": "BP_Wrog_SoldierMelee",
        "parent": AI + "BP_AI_SoldierMelee",
        "materials": [
            ("MI_Wrog_Kontaktowy_01", unreal.LinearColor(0.085, 0.036, 0.016, 1.0), 0.9, "T_Manny_01_N"),
            ("MI_Wrog_Kontaktowy_02", unreal.LinearColor(0.024, 0.016, 0.012, 1.0), 0.7, "T_Manny_02_N"),
        ],
    },
]

for variant in VARIANTS:
    materials = [material_instance(*spec) for spec in variant["materials"]]
    parent = load(variant["parent"])
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent.generated_class())
    blueprint = create_or_load(variant["blueprint"], unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    cdo = unreal.get_default_object(blueprint.generated_class())
    mesh = cdo.get_editor_property("mesh")
    for slot, material in enumerate(materials):
        mesh.set_material(slot, material)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    note("%s -> %s" % (blueprint.get_path_name(), [m.get_name() if m else None for m in mesh.get_materials()]))

note("ENEMY_DONE")
