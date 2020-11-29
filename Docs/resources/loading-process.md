# Loading
Loading is divided in the following stages:

```mermaid
flowchart LR
    classDef CEntry stroke:#ea9999;

    A[Start]:::CEntry --> B[<b>Notify</b><br>OnLoadBegin]
    B --> Load
    Load --> C[<b>Notify</b><br>OnLoadFinish]
    C --> D[Done]

    subgraph Load [ ]
        LoadB{Is at map?}:::CEntry;
        LoadB -->|No| LoadC[Load Map];
        LoadB -->|Yes| LoadD[Bake Filters];
        LoadC --> LoadD;
        LoadD --> LoadE[Prepare Levels];
        LoadE --> LoadF[Deserialize World];

        click LoadE "./?id=deserialize-world" "Deserialize World" _blank
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
    A[Deserialize Game Instance] --> B[Deserialize Actors];
```
