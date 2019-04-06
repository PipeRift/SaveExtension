# Lifetime Component

Lifetime components are an optional feature. Their single purpose is to offer **events called in relation to the saved lifetime** of the actor.

They are **not necessary** by any means to save your game since they only add some useful events.

## Why can't I just use BeginPlay?

BeginPlay gets called every time game starts for that actor including when game starts, when you load, when the actor didn't exist...

It's not a good representative of the lifetime of the actor. That is why Lifetime events are called only during the actor's lifetime. It doesn't matter if it was loaded, saved, destroyed, etc. It is kind of more deterministic.

<img src="img/lifetime_events.png" alt="Lifetime Events" width="600px" />

## Events

#### Started

Gets called only first time an actor is created.

- When you start a new game
- When you spawn the actor
- Wont be called when you load any game (**Resume** will be called instead)

#### Saved

Called when this actor is saved

#### Resume

Called when this actor is loaded. When opening a saved game from any level in any situation

#### Finish

Similar to EndPlay, but gets called when this actor **gets destroyed during gameplay or at normal endplay**. But wont be called when you load a game and this actor gets destroyed as a consequence.