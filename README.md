# shadeMe Enhanced

This is a fork of the [original eponymous shadeMe plugin](https://github.com/shadeMe/shadeMe) which fixes the annoying shadow popping bug 
when moving around and adds some additional options and optimizations. You can now get nice dynamic shadows on architecture and landscape
with a low iActorShadowCount setting and minimal performance drop.

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
2. Limit the actual number of shadows in *shadeMe.ini* eg.
```ini
[Shadows::MaxCount]
LargeObject=8
TotalObject=10
```

### Additional options

- `[Shadows::General] BoundRadiusWeight=2.0`: Apply weight to object bound radius when queuing normal objects, so that larger objects can
have higher priority (float multiplier)
- `[Shadows::General] ReduceGrids=1`: Reduce the object search to the 3x3 grid when queueing exterior shadows, can increase performance
while limiting distance. (0 or 1)
- `[Shadows::General] ExteriorDistanceCheck=0`: Optionally disable distance check for exterior shadows and just queue everything up to
the near grid, useful with ReduceGrids=1. (0 or 1)
- `[Shadows::LargeObjects] DistanceWeight=1.0` Apply weight to object distance when queueing large objects, so that nearer large
objects can have priority over those that are further away. (float multiplier)
- `[Shadows::PlayerLOSCheck] ExcludeLightLOSPaths=1` Exclude the same paths for player LOS check as *[Shadows::LightLOSCheck] ExcludePaths*, useful for larger interior objects that have disappearing shadows when facing away.
