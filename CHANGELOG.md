### 2017-05-11 (1.0beta1)
* Added option to reduce exterior cell grid check to 3x3 as optimization ([Shadows::General] ReduceGrids=1)
* Added option to disable exterior shadow distance check ([Shadows::General] ExteriorDistanceCheck=0, useful with ReduceGrids=1)
* Added option to weight shadow sorting by distance for large objects ([Shadows::LargeObjects] DistanceWeight)
* Added option to weight shadow sorting by bound radius for normal objects ([Shadows::General] BoundRadiusWeight)
* Added option to limit max count for large objects ([Shadows::MaxCount] LargeObjects)
* Added option to limit total objects (([Shadows::MaxCount] TotalObjects)), fixes shadow popping bug when used with [Shadows::MaxCount] LargeObjects and a high enough ShadowCount in Oblivion.ini
* Added option to exclude same paths as LightLOSCheck for PlayerLOSCheck ([Shadows::MaxCount] ExcludeLightLOSPaths)
