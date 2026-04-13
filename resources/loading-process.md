# Loading
Loading is divided in many stages.

*Note: Dot lines(- - - -) mean concurrency*
```mermaid
flowchart LR
    classDef CEntry stroke:#ea9999;

    A[Start]:::CEntry --> B[<b>Notify</b><br>OnLoadBegan]
    B --> Load
    Load --> C[<b>Notify</b><br>OnLoadFinished]
    C --> D[Done]

    subgraph Load [ ]
        LoadA[Load Info]:::CEntry -.-> LoadB{Is at map?};
        LoadA -.-> LoadG[Load Data]
        LoadB -->|No| LoadC[Load Map];
        LoadC --> LoadMapLoaded
        LoadB -->|Yes| LoadMapLoaded;
        LoadMapLoaded[ ] -.-> LoadWait(( ))
        LoadG -.-> LoadWait
        LoadWait --> LoadD[Bake Filters];
        LoadD --> LoadE[Prepare Levels];
        LoadE --> LoadF[Deserialize World];
    end
```


## Bake filters
In this step, all level filters and the general one are baked.
We need this to check which actors to prepare in each level, and how to deserialize.

## Prepare levels
We must ensure actor correctness before loading the data into actors.
That means all actors that were saved have to be restored, and actors that should not exist have to be deleted.

```mermaid
flowchart LR
    classDef CEntry stroke:#ea9999;

    Start:::CEntry --> PrepareLevel
    PrepareLevel -->|Loop all levels| PrepareLevel
    PrepareLevel --> Done

    subgraph PrepareLevel [Prepare Level]
      A[Remove not saved actors]:::CEntry --> B[Spawn missing saved actors];
    end
```

## Deserialize World
This is where the magic happens. The system goes through each actor to be loaded and deserializes its data from its record.
```mermaid
flowchart LR
    classDef CEntry stroke:#ea9999;

    A[Deserialize Game Instance]:::CEntry --> Loop;
    Loop{Levels left?} -->|No| Done
    Loop -->|Yes| DeserializeLevel

    subgraph DeserializeLevel [Deserialize Level]
      LevelB -->|Loop all saved actors| LevelA;
      LevelA[Deserialize Actor]:::CEntry --> LevelB[Deserialize Actor Components];
    end

    DeserializeLevel --> Loop;
```
