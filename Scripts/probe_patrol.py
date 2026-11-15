import unreal
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_subsystem.load_level("/Game/Mapy/Metro_Rozbudowana")
for actor in actor_subsystem.get_all_level_actors():
    label = actor.get_actor_label()
    if label in ("Hala_Dystansowy_3", "Hala_Kontaktowy_1", "Hala_Kontaktowy_2"):
        for c in actor.get_components_by_class(unreal.ActorComponent):
            if c.get_name() == "AIBehaviorsComponent":
                pp = c.get_editor_property("PatrolPath")
                unreal.log_warning("PROBE %s initial=%s patrol=%s" % (label, c.get_editor_property("InitialBehavior"), pp.get_actor_label() if pp else None))
    if label == "Hala_Patrol":
        spline = actor.get_component_by_class(unreal.SplineComponent)
        n = spline.get_number_of_spline_points()
        pts = [spline.get_location_at_spline_point(i, unreal.SplineCoordinateSpace.WORLD) for i in range(n)]
        unreal.log_warning("PROBE patrol points=%d closed=%s pts=%s" % (n, spline.is_closed_loop(), [(round(p.x), round(p.y), round(p.z)) for p in pts]))
        for name in ("CloseLoop",):
            unreal.log_warning("PROBE patrol %s=%s" % (name, actor.get_editor_property(name)))
