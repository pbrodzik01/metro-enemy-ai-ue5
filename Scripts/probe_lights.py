import unreal
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_subsystem.load_level("/Game/Mapy/Metro_Rozbudowana")
for actor in actor_subsystem.get_all_level_actors():
    cls = actor.get_class().get_name()
    if cls not in ("PointLight", "SpotLight"):
        continue
    comp = actor.get_component_by_class(unreal.LocalLightComponent)
    loc = actor.get_actor_location()
    info = {
        "intensity": round(comp.get_editor_property("intensity"), 1),
        "units": str(comp.get_editor_property("intensity_units")),
        "color": str(comp.get_editor_property("light_color")),
        "radius": round(comp.get_editor_property("attenuation_radius")),
        "temp": comp.get_editor_property("use_temperature"),
        "temperature": comp.get_editor_property("temperature"),
        "shadows": comp.get_editor_property("cast_shadows"),
        "mobility": str(comp.get_editor_property("mobility")),
    }
    unreal.log_warning("PROBE %s %s (%d,%d,%d) %s" % (cls, actor.get_actor_label(), loc.x, loc.y, loc.z, info))
