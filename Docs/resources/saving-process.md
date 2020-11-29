# Saving

Saving is divided in multiple stages:

```mermaid
graph LR
    A[Start] --> B{Valid Map?};
    B -->|Yes| D[Prepare Levels];
    B -->|No| C[Load Map];
    C --> D;
    D --> E[Deserialize];
```
