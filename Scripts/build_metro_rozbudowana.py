import json
import os

import unreal

SOURCE_MAP = "/Game/MetroMaintenanceStation/Maps/Metro1"
TARGET_FOLDER = "/Game/Mapy"
TARGET_MAP = TARGET_FOLDER + "/Metro_Rozbudowana"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "MapTools")
os.makedirs(OUT_DIR, exist_ok=True)

STRUCT = "/Game/MetroMaintenanceStation/Assets/Structure/"
PROPS = "/Game/MetroMaintenanceStation/Assets/Props/"
FENCES = "/Game/MetroMaintenanceStation/Assets/Fences/"
AI = "/Game/AIBehaviorSystem/Blueprint/AI/"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report = {"created": [], "notes": []}


def note(text):
    report["notes"].append(text)
    unreal.log_warning("BUILD: " + text)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Missing asset " + path)
    return asset


def place_mesh(mesh_path, location, yaw=0.0, scale=(1.0, 1.0, 1.0), label=None, material=None):
    trace = False
    note("place %s %s" % (mesh_path.split("/")[-1], label))
    mesh = load(mesh_path)
    if trace:
        note("trace: mesh loaded")
    actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if trace:
        note("trace: spawned")
    actor.static_mesh_component.set_static_mesh(mesh)
    if trace:
        note("trace: mesh set")
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if trace:
        note("trace: scaled")
    if label:
        actor.set_actor_label(label)
        if trace:
            note("trace: labeled")
    if material:
        actor.static_mesh_component.set_material(0, load(material))
        if trace:
            note("trace: material")
    report["created"].append(actor.get_actor_label())
    return actor


def place_class(class_path, location, yaw=0.0, label=None, folder="Rozbudowa"):
    note("spawn %s %s" % (class_path.split("/")[-1], label))
    actor_class = unreal.load_class(None, class_path)
    if not actor_class:
        raise RuntimeError("Missing class " + class_path)
    actor = actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if label:
        actor.set_actor_label(label)
    report["created"].append(actor.get_actor_label())
    return actor


def find_mesh_actor(mesh_name, x, y, tolerance=30.0):
    for actor in actor_subsystem.get_all_level_actors():
        if not isinstance(actor, unreal.StaticMeshActor):
            continue
        mesh = actor.static_mesh_component.static_mesh
        if not mesh or mesh.get_name() != mesh_name:
            continue
        location = actor.get_actor_location()
        if abs(location.x - x) < tolerance and abs(location.y - y) < tolerance:
            return actor
    return None


def behaviors_component(actor):
    for component in actor.get_components_by_class(unreal.ActorComponent):
        if component.get_name() == "AIBehaviorsComponent":
            return component
    return None


def wall_run(axis, fixed, start, count, yaw, prefix):
    for index in range(count):
        center = start + 350.0 + 700.0 * index
        location = (center, fixed, 0.0) if axis == "x" else (fixed, center, 0.0)
        place_mesh(STRUCT + "SM_TunnelWallPanel", location, yaw, label="%s_%d" % (prefix, index))


unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([TARGET_FOLDER], True)
if not level_subsystem.load_level(TARGET_MAP):
    raise RuntimeError("Copy Metro1.umap to Content/Mapy/Metro_Rozbudowana.umap before running this script")
note("editing " + TARGET_MAP)

doorway = find_mesh_actor("SM_TunnelWallPanel", -2365, 1910)
if doorway:
    actor_subsystem.destroy_actor(doorway)
    note("removed wall panel at (-2365, 1910) to open the corridor")
else:
    note("WARNING: doorway panel not found")

CONCRETE = "/Game/MetroMaintenanceStation/Materials/Concrete/MI_Concrete_2"
CEILING = "/Game/MetroMaintenanceStation/Materials/Concrete/MI_Concrete_1"
CUBE = "/Engine/BasicShapes/Cube"

place_mesh(CUBE, (-3415, 1910, -10), 0, (21.2, 7.2, 0.2), "Korytarz_Podloga", CONCRETE)
place_mesh(CUBE, (-3415, 1910, 560), 0, (21.2, 7.2, 0.2), "Korytarz_Strop", CEILING)
wall_run("x", 1560, -4465, 3, -90, "Korytarz_Sciana_S")
wall_run("x", 2260, -4465, 3, 90, "Korytarz_Sciana_N")

HALL_X0, HALL_X1, HALL_Y0, HALL_Y1 = -7265, -4465, 160, 3660
place_mesh(CUBE, (-5865, 1910, -10), 0, (28.4, 35.4, 0.2), "Hala_Podloga", CONCRETE)
place_mesh(CUBE, (-5865, 1910, 560), 0, (28.4, 35.4, 0.2), "Hala_Strop", CEILING)
wall_run("x", HALL_Y0, HALL_X0, 4, -90, "Hala_Sciana_S")
wall_run("x", HALL_Y1, HALL_X0, 4, 90, "Hala_Sciana_N")
wall_run("y", HALL_X0, HALL_Y0, 5, 180, "Hala_Sciana_W")
for index, center in enumerate([510, 1210, 2610, 3310]):
    place_mesh(STRUCT + "SM_TunnelWallPanel", (HALL_X1, center, 0), 0, label="Hala_Sciana_E_%d" % index)

for ix, x in enumerate([-5165, -5865, -6565]):
    for iy, y in enumerate([860, 1910, 2960]):
        place_mesh(STRUCT + "SM_Pillar_01", (x, y - 60, 0), 0, label="Hala_Filar_%d_%d" % (ix, iy))

for index, y in enumerate([660, 1160, 1660, 2160, 2660, 3160, 3660]):
    place_mesh(STRUCT + "SM_Rails_Sunk_01", (-6215, y, 1), 0, label="Hala_Tory_%d" % index)

place_mesh(STRUCT + "SM_Platform_01", (-6765, 2760, 0), 90, (1, 1, 0.75), "Hala_Rampa_N")
place_mesh(STRUCT + "SM_Platform_01", (-6765, 560, 0), 90, (1, 1, 0.75), "Hala_Rampa_S")
place_mesh(STRUCT + "SM_Tunnel_Divider", (-5515, 1180, 0), 90, label="Hala_Murek_1")
place_mesh(STRUCT + "SM_Tunnel_Divider", (-5515, 2640, 0), 90, label="Hala_Murek_2")
place_mesh(STRUCT + "SM_Tunnel_Divider", (-4900, 3000, 0), 0, label="Hala_Murek_3")
place_mesh(STRUCT + "SM_Tunnel_Divider", (-6400, 300, 0), 90, label="Hala_Murek_4")
place_mesh(FENCES + "SM_Fence_01_200", (-4800, 700, 0), 0, label="Hala_Plot_1")
place_mesh(FENCES + "SM_Fence_01_200", (-4800, 900, 0), 0, label="Hala_Plot_2")
place_mesh(FENCES + "SM_Fence_02_150", (-6000, 3450, 115), 90, label="Hala_Plot_3")
place_mesh(FENCES + "SM_Fence_02_150", (-6150, 3450, 115), 90, label="Hala_Plot_4")
place_mesh(FENCES + "SM_InteriorFence_01", (-3000, 1600, 115), -90, (0.79, 1, 1), "Korytarz_Krata")

for index, (x, y, yaw) in enumerate([(-4480, 1300, 0), (-4480, 2500, 0), (-7250, 1500, 180), (-7250, 2300, 180), (-3000, 2245, 90), (-3800, 1575, -90)]):
    place_mesh(PROPS + "SM_FuseBox", (x, y, 5), yaw, (0.8, 0.8, 0.8), "Skrzynka_%d" % index)
place_mesh(PROPS + "SM_Sign", (-4480, 1910, 300), 0, label="Hala_Znak")
place_mesh(PROPS + "SM_Sign", (-2400, 1910, 300), 180, label="Korytarz_Znak")

fixture = find_mesh_actor("Cylinder", -471, 1508, tolerance=60)
fixture_material = None
if fixture:
    material = fixture.static_mesh_component.get_material(0)
    fixture_material = material.get_path_name() if material else None
note("ceiling fixture material: %s" % fixture_material)


def ceiling_light(x, y, label, z=545.0, intensity=25.0, radius=3500.0):
    place_mesh("/Engine/BasicShapes/Cylinder", (x, y, z), 0, (0.3, 0.3, 0.1), label + "_Oprawa", fixture_material)
    spot = actor_subsystem.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(x, y, z - 8.0), unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0))
    component = spot.get_component_by_class(unreal.SpotLightComponent)
    component.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("use_temperature", True)
    component.set_editor_property("temperature", 4500.0)
    component.set_editor_property("attenuation_radius", radius)
    component.set_editor_property("outer_cone_angle", 62.0)
    component.set_editor_property("inner_cone_angle", 18.0)
    spot.set_actor_label(label)
    report["created"].append(label)


def point_light(location, label, color, intensity, radius, units, shadows):
    light = actor_subsystem.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*location), unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    component = light.get_component_by_class(unreal.PointLightComponent)
    component.set_editor_property("intensity_units", units)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("light_color", color)
    component.set_editor_property("attenuation_radius", radius)
    component.set_editor_property("cast_shadows", shadows)
    light.set_actor_label(label)
    report["created"].append(label)


for index, x in enumerate([-2715, -3415, -4115]):
    ceiling_light(x, 1910, "Korytarz_Lampa_%d" % index, intensity=18.0, radius=2000.0)
for ix, x in enumerate([-4815, -5515, -6215, -6915]):
    for iy, y in enumerate([510, 1560, 2610, 3310]):
        ceiling_light(x, y, "Hala_Lampa_%d_%d" % (ix, iy))

WARM_FILL = unreal.Color(r=255, g=239, b=209, a=255)
ORANGE = unreal.Color(r=255, g=53, b=0, a=255)
for ix, x in enumerate([-5165, -6565]):
    for iy, y in enumerate([860, 2960]):
        point_light((x, y, 360), "Hala_Wypelnienie_%d_%d" % (ix, iy), WARM_FILL, 9.0, 1400.0, unreal.LightUnits.CANDELAS, False)
point_light((-3415, 1910, 360), "Korytarz_Wypelnienie", WARM_FILL, 3.0, 900.0, unreal.LightUnits.CANDELAS, False)
for index, (x, y) in enumerate([(-4560, 1300), (-4560, 2500), (-7150, 1500), (-7150, 2300)]):
    point_light((x, y, 190), "Akcent_Pomaranczowy_%d" % index, ORANGE, 5.0, 260.0, unreal.LightUnits.LUMENS, True)
for index, y in enumerate([1500, 2320]):
    place_class("/Game/MetroMaintenanceStation/Blueprints/BP_Light.BP_Light_C", (-4550, y, 270), 0, "Hala_Sygnal_%d" % index)

nav_source = next((a for a in actor_subsystem.get_all_level_actors() if isinstance(a, unreal.NavMeshBoundsVolume)), None)
if nav_source:
    nav_source.set_actor_location(unreal.Vector(-3112.5, 1000, 90), False, False)
    nav_source.set_actor_scale3d(unreal.Vector(43.375, 65.5, 9.25))
    note("NavMeshBoundsVolume extended to X -7450..1225")
for recast in [a for a in actor_subsystem.get_all_level_actors() if a.get_class().get_name() == "RecastNavMesh"]:
    try:
        recast.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
        note("RecastNavMesh runtime generation set to Dynamic")
    except Exception as error:
        note("WARNING runtime generation: " + str(error))

MELEE = "/Game/Ocalaly/Wrogowie/BP_Wrog_SoldierMelee.BP_Wrog_SoldierMelee_C"
RANGE = "/Game/Ocalaly/Wrogowie/BP_Wrog_SoldierRange.BP_Wrog_SoldierRange_C"
REPLACEMENTS = {
    "BP_AI_SoldierMelee_TeamA_C": MELEE,
    "BP_AI_SoldierMelee_C": MELEE,
    "BP_AI_SoldierRange_C": RANGE,
}

for actor in list(actor_subsystem.get_all_level_actors()):
    class_name = actor.get_class().get_name()
    if class_name in REPLACEMENTS:
        location = actor.get_actor_location()
        rotation = actor.get_actor_rotation()
        label = actor.get_actor_label()
        actor_subsystem.destroy_actor(actor)
        place_class(REPLACEMENTS[class_name], (location.x, location.y, location.z), rotation.yaw, label.replace("_TeamA", ""), folder="Przeciwnicy")
        note("replaced %s (%s) with %s" % (label, class_name, REPLACEMENTS[class_name].split(".")[-1]))

patrol = place_class(AI + "Patrol/BP_PatrolPath.BP_PatrolPath_C", (-5000, 650, 90), 0, "Hala_Patrol")
spline = patrol.get_component_by_class(unreal.SplineComponent)
points = [unreal.Vector(-5000, 650, 90), unreal.Vector(-6950, 1250, 90), unreal.Vector(-6950, 2450, 90), unreal.Vector(-5900, 3350, 90), unreal.Vector(-4800, 2300, 90)]
if spline:
    spline.set_spline_points(points, unreal.SplineCoordinateSpace.WORLD, True)
    for flag in ("spline_has_been_edited", "b_spline_has_been_edited"):
        try:
            spline.set_editor_property(flag, True)
            note("spline flag %s set" % flag)
            break
        except Exception as error:
            note("spline flag %s failed: %s" % (flag, error))
    spline.set_closed_loop(True, True)
    note("patrol spline points: %d closed: %s" % (spline.get_number_of_spline_points(), spline.is_closed_loop()))
for name in ["CloseLoop"]:
    try:
        patrol.set_editor_property(name, True)
        note("patrol CloseLoop=True via " + name)
        break
    except Exception as error:
        note("patrol CloseLoop via %s failed: %s" % (name, error))

new_enemies = [
    (RANGE, (-6650, 1100, 92), 90, "Hala_Dystansowy_1", None),
    (RANGE, (-6650, 2750, 92), -90, "Hala_Dystansowy_2", None),
    (RANGE, (-5450, 3300, 92), -90, "Hala_Dystansowy_3", "patrol"),
    (RANGE, (-4250, 2050, 92), 180, "Korytarz_Dystansowy", None),
    (MELEE, (-5200, 700, 92), 90, "Hala_Kontaktowy_1", "patrol"),
    (MELEE, (-5800, 2350, 92), 0, "Hala_Kontaktowy_2", "roam"),
    (MELEE, (-3600, 1800, 92), 180, "Korytarz_Kontaktowy", "roam"),
]

behavior_probe = {}
for class_path, location, yaw, label, mode in new_enemies:
    enemy = place_class(class_path, location, yaw, label, folder="Przeciwnicy")
    component = behaviors_component(enemy)
    if not component:
        note("WARNING no behaviors component on " + label)
        continue
    if not behavior_probe:
        try:
            value = component.get_editor_property("InitialBehavior")
            behavior_probe = {"type": str(type(value)), "repr": repr(value), "members": [m for m in dir(type(value)) if m.isupper()]}
        except Exception as error:
            behavior_probe = {"error": str(error)}
        note("InitialBehavior probe: " + json.dumps(behavior_probe))
    if mode in ("patrol", "roam"):
        behavior_type = type(component.get_editor_property("InitialBehavior"))
        target = behavior_type.PATROL if mode == "patrol" else behavior_type.ROAMING
        try:
            component.set_editor_property("InitialBehavior", target)
            note("%s: InitialBehavior=%s" % (label, target))
        except Exception as error:
            note("%s: InitialBehavior failed: %s" % (label, error))
    if mode == "patrol":
        component = behaviors_component(enemy)
        try:
            component.set_editor_property("PatrolPath", patrol)
            note("%s: PatrolPath assigned" % label)
        except Exception as error:
            note("%s: PatrolPath failed: %s" % (label, error))
        component = behaviors_component(enemy)
        if component.get_editor_property("InitialBehavior") != behavior_type.PATROL:
            component.set_editor_property("InitialBehavior", behavior_type.PATROL)
            note("%s: InitialBehavior re-applied after construction rerun" % label)
        component = behaviors_component(enemy)
        note("%s: final initial=%s patrol=%s" % (label, component.get_editor_property("InitialBehavior"), component.get_editor_property("PatrolPath")))

saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
note("saved dirty packages: %s" % saved)

with open(os.path.join(OUT_DIR, "build_report.json"), "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=1, ensure_ascii=False)

actors = actor_subsystem.get_all_level_actors()
survey = {"actors": []}
for actor in actors:
    transform = actor.get_actor_transform()
    origin, extent = actor.get_actor_bounds(False)
    entry = {"label": actor.get_actor_label(), "class": actor.get_class().get_name(), "loc": [transform.translation.x, transform.translation.y, transform.translation.z],
             "bounds_origin": [origin.x, origin.y, origin.z], "bounds_extent": [extent.x, extent.y, extent.z]}
    if isinstance(actor, unreal.StaticMeshActor) and actor.static_mesh_component.static_mesh:
        entry["mesh"] = actor.static_mesh_component.static_mesh.get_path_name()
    survey["actors"].append(entry)
with open(os.path.join(OUT_DIR, "survey_rozbudowana.json"), "w", encoding="utf-8") as handle:
    json.dump(survey, handle, indent=1, ensure_ascii=False)

note("BUILD_DONE created=%d" % len(report["created"]))
