import json
import os

import unreal

OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "MapTools")
os.makedirs(OUT_DIR, exist_ok=True)

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def vec(v):
    return [round(v.x, 2), round(v.y, 2), round(v.z, 2)]


def describe_actor(actor):
    transform = actor.get_actor_transform()
    rotator = transform.rotation.rotator()
    origin, extent = actor.get_actor_bounds(False)
    info = {
        "name": actor.get_name(),
        "label": actor.get_actor_label(),
        "class": actor.get_class().get_name(),
        "loc": vec(transform.translation),
        "rot": [round(rotator.pitch, 2), round(rotator.yaw, 2), round(rotator.roll, 2)],
        "scale": vec(transform.scale3d),
        "bounds_origin": vec(origin),
        "bounds_extent": vec(extent),
    }
    if isinstance(actor, unreal.StaticMeshActor):
        mesh = actor.static_mesh_component.static_mesh
        info["mesh"] = mesh.get_path_name() if mesh else None
    return info


def component_props(actor, component_name, names):
    result = {}
    for component in actor.get_components_by_class(unreal.ActorComponent):
        if component.get_name() != component_name:
            continue
        for name in names:
            try:
                value = component.get_editor_property(name)
                if isinstance(value, unreal.Array):
                    value = [str(v) for v in value]
                elif hasattr(value, "get_path_name"):
                    value = value.get_path_name()
                else:
                    value = str(value)
                result[name] = value
            except Exception as error:
                result[name] = "ERROR: " + str(error)
    return result


def try_set_target_tags(actor):
    for component in actor.get_components_by_class(unreal.ActorComponent):
        if component.get_name() == "AIBehaviorsComponent":
            try:
                component.set_editor_property("TargetTags", ["Player", "Companion"])
                return "ok"
            except Exception as error:
                return "ERROR: " + str(error)
    return "no component"


def list_property_names(obj):
    names = []
    for name in dir(obj):
        if name.startswith("_"):
            continue
        names.append(name)
    return names


level_subsystem.load_level("/Game/MetroMaintenanceStation/Maps/Metro1")
actors = actor_subsystem.get_all_level_actors()
survey = {"actors": [describe_actor(a) for a in actors]}

meshes = {}
for path in unreal.EditorAssetLibrary.list_assets("/Game/MetroMaintenanceStation/Assets", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, unreal.StaticMesh):
        box = asset.get_bounding_box()
        meshes[asset.get_path_name()] = {"min": vec(box.min), "max": vec(box.max)}
survey["meshes"] = meshes

behavior_props = ["TargetTags", "TargetTagCompanion", "PatrolPath", "Behavior", "BehaviorType", "PossessedBehaviors", "CurrentBehaviorConfig"]
enemy_report = []
for actor in actors:
    class_name = actor.get_class().get_name()
    if "BP_AI_" in class_name:
        entry = {"label": actor.get_actor_label(), "class": class_name, "loc": vec(actor.get_actor_location())}
        entry["behaviors"] = component_props(actor, "AIBehaviorsComponent", behavior_props)
        enemy_report.append(entry)
survey["enemies"] = enemy_report

blueprints = {
    "melee": "/Game/AIBehaviorSystem/Blueprint/AI/Example/BP_AI_SoldierMelee.BP_AI_SoldierMelee_C",
    "melee_team_a": "/Game/AIBehaviorSystem/Blueprint/AI/Example/BP_AI_SoldierMelee_TeamA.BP_AI_SoldierMelee_TeamA_C",
    "range": "/Game/AIBehaviorSystem/Blueprint/AI/Example/BP_AI_SoldierRange.BP_AI_SoldierRange_C",
    "patrol_path": "/Game/AIBehaviorSystem/Blueprint/AI/Patrol/BP_PatrolPath.BP_PatrolPath_C",
}
spawned_report = {}
for key, class_path in blueprints.items():
    actor_class = unreal.load_class(None, class_path)
    if not actor_class:
        spawned_report[key] = "class not found"
        continue
    temp = actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(0, 0, -100000))
    report = {"components": [c.get_name() + ":" + c.get_class().get_name() for c in temp.get_components_by_class(unreal.ActorComponent)]}
    if key != "patrol_path":
        report["behaviors"] = component_props(temp, "AIBehaviorsComponent", behavior_props)
        report["set_target_tags"] = try_set_target_tags(temp)
        for component in temp.get_components_by_class(unreal.ActorComponent):
            if component.get_name() == "AIBehaviorsComponent":
                report["behavior_component_attributes"] = [n for n in list_property_names(component) if n[0].isupper() or "patrol" in n.lower() or "tag" in n.lower()]
    else:
        report["attributes"] = [n for n in list_property_names(temp) if n[0].isupper() or "point" in n.lower() or "spline" in n.lower()]
    actor_subsystem.destroy_actor(temp)
    spawned_report[key] = report
survey["blueprints"] = spawned_report

with open(os.path.join(OUT_DIR, "survey.json"), "w", encoding="utf-8") as handle:
    json.dump(survey, handle, indent=1, ensure_ascii=False)

unreal.log("SURVEY_DONE actors=%d meshes=%d" % (len(survey["actors"]), len(meshes)))
