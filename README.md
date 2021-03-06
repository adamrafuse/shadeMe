# shadeMe Enhanced

This is a fork of the [original eponymous shadeMe plugin](https://github.com/shadeMe/shadeMe) which fixes the annoying shadow popping bug 
when moving around and adds some additional options and optimizations. You can now get nice dynamic shadows on architecture and landscape
with a low shadow count and minimal performance drop.

For releases and general discussion head on over to this plugin's home on the [Oblivion Nexus](http://www.nexusmods.com/oblivion/mods/47779/?).

## Usage

### General

1. In *Oblivion.ini*, the following options must be set:
```ini
bShadowsOnGrass=0
bActorSelfShadowing=1
```

### Fixing the shadow popping bug

This seemed to be caused by the Oblivion engine trying to optimize the scene when the shadow count approached the maximum. This can be 
mitigated by imposing the shadow limit in the plugin and giving the engine enough headroom to avoid triggering this condition.

1. In *Oblivion.ini*, set these to a high number eg.:  
```ini
iActorShadowCountInt=100
iActorShadowCountExt=100
```
2. Limit the actual number of shadows in *shadeMe.ini*. Give large objects higher priority, and disable exterior player LOS check eg.
```ini
[Shadows::MaxCount]
LargeObject=8
TotalObject=10

[Shadows::LargeObjects]
HigherPriority=1

[Shadows::PlayerLOSCheck]
Exterior=0
```

### Additional options

- `[Shadows::General] PrioritizeActorShadows=1`: Number of actor shadows to prioritize. The default setting of 1 is to always keep the player shadow visible. You can increase this if you always want to see more actor shadows before other small objects. (int value)
- `[Shadows::General] BoundRadiusWeight=2.0`: Apply weight to object bound radius when queuing normal objects, so that larger objects can have higher priority (float multiplier)
- `[Shadows::General] ReduceGrids=1`: Reduce the object search to the 3x3 grid when queueing exterior shadows, can increase performance
while limiting distance. (0 or 1)
- `[Shadows::General] ExteriorDistanceCheck=0`: Optionally disable distance check for exterior shadows and just queue everything up to
the near grid, useful with ReduceGrids=1. (0 or 1)
- `[Shadows::LargeObjects] DistanceWeight=1.0` Apply weight to object distance when queueing large objects, so that nearer large
objects can have priority over those that are further away. (float multiplier)
- `[Shadows::PlayerLOSCheck] ExcludeLightLOSPaths=1` Exclude the same paths for player LOS check as *[Shadows::LightLOSCheck] ExcludePaths*, useful for larger interior objects that have disappearing shadows when facing away.
- `[Shadows::General] EnableDetailedDebugSelection=0`: This option now dumps shadow caster details to shadeMe.log when enabled, useful for determining which objects are casting invalid shadows (along with *EnableDebugShader=1)
