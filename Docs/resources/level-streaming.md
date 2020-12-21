# Level Streaming & World Composition

Save Extension support this engine features.

Saving will adapt, load and unload sublevels seamlessly so that is a sublevel is unloaded, its data is cached and if it gets loaded, its data gets restored.

This means sublevel data is still only saved when the game saves or loads, but their state is persistent in memory.