## -*-python-*-
## this is an example dueca_mod.py file, for you to start out with and adapt
## according to your needs. Note that you need a dueca_mod.py file only for the
## node with number 0
compass = True
outside = True
virtual_stick = True
use_vsg = True
claim_thread = True

## in general, it is a good idea to clearly document your set up
## this is an excellent place.

## node set-up
ecs_node = 0   # dutmms1, send order 3
#aux_node = 1   # dutmms3, send order 1
#pfd_node = 2   # dutmms5, send order 2
#cl_node = 3    # dutmms4, send order 0

## priority set-up
# normal nodes: 0 administration
#               1 hdf5 logging
#               2 simulation, unpackers
#               3 communication
#               4 ticker

# administration priority. Run the interface and logging here
admin_priority = dueca.PrioritySpec(0, 0)

# logging prio. Keep this free from time-critical other processes
log_priority = dueca.PrioritySpec(1, 0)

# priority of simulation, just above log
sim_priority = dueca.PrioritySpec(3, 0)

if claim_thread:
    graphics_priority = dueca.PrioritySpec(2, 0)
else:
    graphics_priority = dueca.PrioritySpec(0, 0)


# nodes with a different priority scheme
# control loading node has 0, 1, 2 and 3 as above and furthermore
#               4 stick priority
#               5 ticker priority
# priority of the stick. Higher than prio of communication
# stick_priority = dueca.PrioritySpec(4, 0)

# timing set-up
# timing of the stick calculations. Assuming 100 usec ticks, this gives 2500 Hz
# stick_timing = dueca.TimeSpec(0, 4)

# this is normally 100, giving 100 Hz timing
sim_timing = dueca.TimeSpec(0, 100)

## for now, display on 20 Hz
display_timing = dueca.TimeSpec(0, 500)

## log a bit more economical, 25 Hz
log_timing = dueca.TimeSpec(0, 400)


if virtual_stick:
    stick_device = (
        # virtual, gtk driven stick
        ('add-virtual', ("logi",)),
        # axes 0 and 1, roll and pitch
        ('add-virtual-slider-2d',
            (15, 15, 185, 185, 3)),
        # axis 2, yaw
        ('add-virtual-slider',
            (10, 195, 190, 195, 3)),
        # axis 3, throttle
        ('add-virtual-slider',
            (5, 190, 5, 10, 3, 1)),
        # place it on the screen
        ('virtual-position-size', (0, 370)),
    )
else:
    stick_device = (
        # logitech stick, first SDL device
        ('add_device', "logi:0"),
    )


## ---------------------------------------------------------------------
### the modules needed for dueca itself
if this_node_id == ecs_node:

    # create a list of modules:
    DUECA_mods = []
    DUECA_mods.append(dueca.Module("dusime", "", admin_priority))
    DUECA_mods.append(dueca.Module("dueca-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("activity-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("timing-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("log-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("channel-view", "", admin_priority))
    # uncomment for web-based graph, see DUECA documentation
    # DUECA_mods.append(dueca.Module("config-storage", "", admin_priority))

    if no_of_nodes > 1 and not classic_ip:
        DUECA_mods.append(dueca.Module("net-view", "", admin_priority))

    # remove the quotes to enable DUSIME initial condition recording and
    # setting, and simulation recording and replay
    for e in ("SIMPLE",):
        DUECA_mods.append(
            dueca.Module("initials-inventory", e, admin_priority).param(
                # reference_file=f"initials-{e}.toml",
                store_file=f"initials-{e}-%Y%m%d_%H%M.toml"))
        DUECA_mods.append(
            dueca.Module("replay-master", e, admin_priority).param(
                #reference_files=f"recordings-{e}.ddff",
                store_files=f"recordings-{e}-%Y%m%d_%H%M%S.ddff"))

    # create the DUECA entity with that list
    DUECA_entity = dueca.Entity("dueca", DUECA_mods)

## ---------------------------------------------------------------------
# modules for your project (example)
mymods = []

if this_node_id == ecs_node:
    mymods.append(
        dueca.Module(
            "flexi-stick", "", sim_priority).param(
            set_timing = sim_timing,
            enable_record_replay = True,
            check_timing = (1000, 2000)).param(
                *stick_device,

            # by default, axes go from -1 to 1, convert throttle to
            # run from -1 to 5 (slow back-up to forward 5m/s), with
            # a polynomial. The throttle is on axis 2 of the stick
            ('create_poly', ('throttle', 'logi.a[3]')),
            ('poly_params', (2, -3)),
            ('create_poly', ('roll', 'logi.a[0]')),
            ('poly_params', (0, -1)),
            ('create_poly', ('pitch', 'logi.a[1]')),
            ('poly_params', (0, -1)),
            ('create_poly', ('yaw', 'logi.a[2]')),
            ('poly_params', (0, -1)),

            # define that we write a channel
            ('add_channel',
             ('controls',              # variable
              'ControlInput://SIMPLE', # channel name
              'ControlInput',          # data type
              'control input')),       # label

            # link axis 0 to control roll, etc, etc
            ("add_link", ("controls.roll", "roll")),
            ("add_link", ("controls.pitch", "pitch")),
            ("add_link", ("controls.yaw", "yaw")),
            ("add_link", ("controls.throttle", "throttle"))
            )
    )

    # our new dynamics module
    mymods.append(dueca.Module(
        "ufo-dynamics", "", sim_priority).param(
            set_timing = sim_timing,
            check_timing = (1000, 2000)))

    if compass:
        mymods.append(dueca.Module(
            "compass", "", admin_priority).param(
                set_timing = display_timing,
                check_timing = (1000, 2000)))

    # the visual output
    if outside and not use_vsg:
        mymods.append(
            dueca.Module(
                "world-view", "", admin_priority).param(
                set_timing = display_timing,
                check_timing = (8000, 9000),
                set_viewer =
                dueca.OSGViewer().param(
                    # set up window
                    ('add_window', 'front'),
                    ('window_size+pos', (800, 600, 10, 10)),
                    ('add_viewport', 'front'),
                    ('viewport_window', 'front'),
                    ('viewport_pos+size', (0, 0, 800, 600)),

                    # add visual objects (classes, then instantiation)
                    ('add-object-class-data',
                    ("static:sunlight", "sunlight", "static-light")),
                    ('add-object-class-coordinates',
                    (0.48, 0.48, 0.48, 1,   # ambient
                    0.48, 0.48, 0.48, 1,   # diffuse
                    0.0, 0.0, 0.0, 1,      # specular
                    0.4, 0.0, 1.0, 0,      # south??
                    0, 0, 0,               # direction not used
                    0.2, 0, 0)),           # no attenuation for sun
                    ('add-object-class-data',
                    ("static:terrain", "terrain", "static", "terrain.obj")),
                    ('add-object-class-data',
                    ("centered:skydome", "skydome", "centered", "skydome.obj")),
                    ('add-object-class-coordinates',
                    (0.0, 0.0, 50.0)),

                    # make the objects
                    ('static-object', ('static:sunlight', 'sunlight')),
                    ('static-object', ('static:terrain', 'terrain')),
                    ('static-object', ('centered:skydome', 'skydome'))
                )
                )
            )

    if outside and use_vsg:
        mymods.append(
            dueca.Module(
                "world-view", "", graphics_priority).param(
                set_timing = display_timing,
                claim_thread = claim_thread,
                check_timing = (8000, 9000),
                set_viewer =
                dueca.VSGViewer().param(
                    # set up window
                    ('add_window', 'front'),
                    ('window-size+pos', (800, 600, 10, 10)),
                    ('add_viewport', 'front'),
                    ('viewport_window', 'front'),
                    ('viewport-pos+size', (0, 0, 800, 600)),
                    ('set-frustum', (1.0, 1000.0, 40.0)),

                    # add visual objects (classes, then instantiation)
                    ('add-object-class',
                     ("static:sunlight:dir", "sunlight", "directional-light")),
                    ('add-object-class-parameters',
                    (1.0, 1.0, 1.0,           # color white
                     0.2,                     # intensity
                     0.1, 0.1, 1.0            # direction
                     )),
                    ('add-object-class',
                     ("static:sunlight:amb", "ambient", "ambient-light")),
                    ('add-object-class-parameters',
                    (1.0, 1.0, 1.0,           # color white
                     0.5                      # intensity
                    )),
                    ('add-object-class',
                    ('static:terrain:base', "root/tbase", "static-transform")),
                    ('add-object-class-parameters',
                    (0, 0, 0, 0, 0, 0, 100, 100, 100)),
                    ('add-object-class',
                    ("static:terrain", "tbase/terrain", "static-model", "terrain.vsgb")),
                    ('add-object-class',
                    ('static:skydome:base', "observer/sbase", "centered-transform")),
                    ('add-object-class-parameters',
                    (0, 0, 50, 0, 0, 0, 200, 200, 200)),

                    ('add-object-class',
                    ("centered:skydome", "sbase/skydome", "static-model", "skydome.vsgb")),

                    # make the objects
                    ('create-static', ('static:terrain:base',)),
                    ('create-static', ('static:skydome:base',)),
                    ('create-static', ('static:sunlight:dir', 'sunlight-dir')),
                    ('create-static', ('static:sunlight:amb', 'sunlight-amb')),

                    ('create-static', ('static:terrain', 'terrain')),
                    ('create-static', ('centered:skydome', 'skydome')),
                 #('set-xml-definitions',
                 #'../../../../WorldView/vsg-viewer/vsgobjects.xml'),
                #('read-xml-definitions', 'exampleworld.xml'),
               )
                )
            )


    # replay filer for the "simple" entity's recordable/replayable data
    # (basically the flexi-stick)
    filer = dueca.ReplayFiler("SIMPLE")

    # this simply prints joining of teams and current position
    mymods.append(dueca.Module(
        "monitor-teams", "", sim_priority).param(
            set_timing = log_timing,
            check_timing = (10000, 20000)))


# then combine in an entity (one "copy" per node)
if mymods:
    myentity = dueca.Entity("SIMPLE", mymods)
