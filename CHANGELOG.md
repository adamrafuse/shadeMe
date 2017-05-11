### 2017-05-11
* Added option to reduce exterior cell grid check to 3x3 as optimization ([Shadows::General] ReduceGridSearch=1)
* Added option to disable exterior shadow distance check ([Shadows::General] ExteriorDistanceCheck=0, useful with ReduceGridSearch=1)
* Added option to weight shadow sorting by distance for large objects ([Shadows::LargeObjects] DistanceWeight)
* Added option to weight shadow sorting by bound radius for normal objects ([Shadows::General] BoundRadiusWeight)
* Added option to limit max count for large objects ([Shadows::MaxCount] LargeObjects)
