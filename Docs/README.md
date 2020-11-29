# Save Extension

Save Extension allows your projects to be saved and loaded automatically.

If you like our plugins, consider becoming a Patron. It will go a long way in helping me create more awesome tech!

[![patron](assets/patron_small.png)](https://www.patreon.com/bePatron?u=16503983)

## Intended Usage

Our plugins are designed to work for very different games and needs, but naturally, they are more focused towards satisfying certain needs.

**Save Extension** in particular has been created to help games with high amounts of content in the world like open world or narrative games that need to save world state with the less amount of work or complexity possible.

As an example, a game like Super Mario probably wouldn't need **Save Extension**, because it doesn't need to store world state. It can do it, but may not be worth it. Other games might have items to be picked, player states, AI, or streaming levels that require this serialization and here's where the strength of **Save Extension** really shines.

## Quick Start

Check [Quick Start](quick-start.md) to see how to setup and configure the plugin.

## Supported Features

#### SaveGame tag saving

Any variable tagged as `SaveGame` will be saved.

#### Full world serialization

All actors in the world are susceptible to be saved.

Only exceptions are for example StaticMeshActors

#### Asynchronous Saving and Loading

Loading and saving can run asynchronously, splitting the load between frames. <br>This states can be tracked and shown on UI.

#### Level Streaming and World Composition

Sublevels can be loaded and saved when they get streamed in or out. This allows games to keep the state of the levels even without saving the game.

*If the player exists an area where 2 enemies were damaged, when he gets in again this enemies will keep their damaged state*

#### Data Modularity

All data is structured in the way that levels can be loaded at a minimum cost.

#### Compression

Files can be compressed, getting up to 20 times smaller file sizes.



## About Us

At Piperift we like to release the technology we create for ourselves.

Save Extension has been around for multiple years being used in internal and external projects, and as such, we wanted it to be public so that others can enjoy it too, making the job of the developer easier. 

This plugin was designed to fulfill the needs for automatic world saving and loading. Features that are unfortunately missing in the engine at this point in time. Automatic in the sense that any actor in the world can be saved, including AI, Players, controllers or game logic without any extra components or setups.