import json
import os

import unreal

OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "MapTools")
FOLDER = "/Game/Ocalaly"
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
report = {}


def note(text):
    unreal.log_warning("SKIN: " + text)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Missing " + path)
    return asset


def describe(material):
    return {
        "vectors": {str(n): str(mel.get_material_instance_vector_parameter_value(material, n)) for n in mel.get_vector_parameter_names(material)},
        "scalars": {str(n): mel.get_material_instance_scalar_parameter_value(material, n) for n in mel.get_scalar_parameter_names(material)},
        "textures": [str(n) for n in mel.get_texture_parameter_names(material)],
    }


def create_or_load(name, asset_class, factory):
    path = FOLDER + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return load(path)
    return asset_tools.create_asset(name, FOLDER, asset_class, factory)


SOURCES = [
    "/Game/AIBehaviorSystem/Demo/Characters/Mannequins/Materials/Instances/Manny/MI_Manny_01",
    "/Game/AIBehaviorSystem/Demo/Characters/Mannequins/Materials/Instances/Manny/MI_Manny_02",
]
PALETTE = {
    "base": unreal.LinearColor(0.045, 0.05, 0.032, 1.0),
    "accent": unreal.LinearColor(0.16, 0.055, 0.02, 1.0),
    "dark": unreal.LinearColor(0.012, 0.012, 0.012, 1.0),
}

MANNY_TEXTURES = "/Game/AIBehaviorSystem/Demo/Characters/Mannequins/Textures/Manny/"
GRUNGE = "/Game/MetroMaintenanceStation/Materials/Utility/T_Grunge"


def build_master_material():
    path = FOLDER + "/M_Ocalaly"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    material = asset_tools.create_asset("M_Ocalaly", FOLDER, unreal.Material, unreal.MaterialFactoryNew())

    color = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, -200)
    color.set_editor_property("parameter_name", "Kolor")
    color.set_editor_property("default_value", PALETTE["base"])

    uv = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
    uv.set_editor_property("u_tiling", 3.0)
    uv.set_editor_property("v_tiling", 3.0)
    grunge = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -1050, 0)
    grunge.set_editor_property("texture", load(GRUNGE))
    mel.connect_material_expressions(uv, "", grunge, "UVs")

    dirt = mel.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -700, 0)
    dirt.set_editor_property("const_a", 0.45)
    dirt.set_editor_property("const_b", 1.0)
    mel.connect_material_expressions(grunge, "R", dirt, "Alpha")

    tinted = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -450, -100)
    mel.connect_material_expressions(color, "", tinted, "A")
    mel.connect_material_expressions(dirt, "", tinted, "B")
    mel.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -450, 150)
    roughness.set_editor_property("parameter_name", "Szorstkosc")
    roughness.set_editor_property("default_value", 0.85)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metallic = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -450, 250)
    metallic.set_editor_property("parameter_name", "Metalicznosc")
    metallic.set_editor_property("default_value", 0.05)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

    normal = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -450, 400)
    normal.set_editor_property("parameter_name", "Normalna")
    normal.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    normal.set_editor_property("texture", load(MANNY_TEXTURES + "T_Manny_01_N"))
    mel.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    note("master material %s" % material.get_path_name())
    return material


master = build_master_material()
SLOTS = [
    {"color": PALETTE["base"], "normal": MANNY_TEXTURES + "T_Manny_01_N", "roughness": 0.85},
    {"color": unreal.LinearColor(0.03, 0.026, 0.022, 1.0), "normal": MANNY_TEXTURES + "T_Manny_02_N", "roughness": 0.7},
]

created = []
for index, slot in enumerate(SLOTS):
    factory = unreal.MaterialInstanceConstantFactoryNew()
    instance = create_or_load("MI_Ocalaly_%02d" % (index + 1), unreal.MaterialInstanceConstant, factory)
    mel.set_material_instance_parent(instance, master)
    mel.set_material_instance_vector_parameter_value(instance, "Kolor", slot["color"])
    mel.set_material_instance_scalar_parameter_value(instance, "Szorstkosc", slot["roughness"])
    mel.set_material_instance_texture_parameter_value(instance, "Normalna", load(slot["normal"]))
    mel.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    report["created_%d" % index] = describe(instance)
    created.append(instance)
    note("material %s" % instance.get_path_name())

player_bp = load("/Game/AIBehaviorSystem/Blueprint/BP_CharacterExample")
bp_factory = unreal.BlueprintFactory()
bp_factory.set_editor_property("parent_class", player_bp.generated_class())
skin_bp = create_or_load("BP_Ocalaly", unreal.Blueprint, bp_factory)
unreal.BlueprintEditorLibrary.compile_blueprint(skin_bp)
skin_cdo = unreal.get_default_object(skin_bp.generated_class())
mesh_component = skin_cdo.get_editor_property("mesh")
for slot, material in enumerate(created):
    mesh_component.set_material(slot, material)
note("BP_Ocalaly materials: %s" % [m.get_name() if m else None for m in mesh_component.get_materials()])
unreal.EditorAssetLibrary.save_loaded_asset(skin_bp)

mode_bp_source = load("/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode")
mode_factory = unreal.BlueprintFactory()
mode_factory.set_editor_property("parent_class", mode_bp_source.generated_class())
mode_bp = create_or_load("BP_TrybGry_Ocalaly", unreal.Blueprint, mode_factory)
unreal.BlueprintEditorLibrary.compile_blueprint(mode_bp)
mode_cdo = unreal.get_default_object(mode_bp.generated_class())
mode_cdo.set_editor_property("default_pawn_class", skin_bp.generated_class())
note("GameMode default pawn: %s" % mode_cdo.get_editor_property("default_pawn_class").get_name())
unreal.EditorAssetLibrary.save_loaded_asset(mode_bp)

with open(os.path.join(OUT_DIR, "skin_report.json"), "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=1, ensure_ascii=False)
note("SKIN_DONE")
