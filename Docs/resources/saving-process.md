# Saving
Saving is divided in many stages:

*Note: Dot lines(- - - -) mean concurrency*
```mermaid
flowchart LR
    classDef CEntry stroke:#ea9999;

    A[Start]:::CEntry --> B[Delete old save];
    B --> C[<b>Notify</b><br>OnSaveBegan];
    C -.-> D[Capture Thumbnail];
    C -.-> E[Capture stats];
    E --> SerializeWorld[Serialize World];
    SerializeWorld --> H[<b>Notify</b><br>OnSaveFinished];
    H --> End;
```

## Capture thumbnail
Runs independently from the rest of the save process. An screenshot of the desired characteristics will be queued in ue4's system and then saved with its correct name after x frames.

## Capture stats
Store game time, current map, filters and more inside a new SlotData object.

## Serialize World
We iterate all actors to be saved and serialize each of them.

```mermaid
flowchart TB
    classDef CEntry stroke:#ea9999;

    A[Bake Filters]:::CEntry --> Levels;
    subgraph Levels [Serialize Levels]
        subgraph Level [Serialize Level]
            LevelB -->|Loop all saved actors| LevelA;
            LevelA[Serialize Actor]:::CEntry --> LevelB[Serialize Actor Components];
        end

        Level -->|Loop all levels| Level;
    end
    Levels --> Done
```

If **MultithreadedSerialization** is *SaveAsync* or *SaveAndLoadAsync*, actors to be deserialized will be distributed between all available threads.
