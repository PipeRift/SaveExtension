# Slots

In Save Extension, we refer to "slots" as an **instance of a saved game** and they are identified by a number.
Most of the times we will think of slots as the actual saved files.

## Slots as files

An slot is composed of two individual files. One is the slot info, and the other the slot data.

### Slot Info

Contains **lightweight** information about the saved game. Information we want to obtain without needing to load the rest of the slot.
Some examples are player level, xp, progress, player name, zone or area, current objective or thumbnail.

The game can access all slot infos very quickly without requiring to load game, which is very useful for UI when we want to display a list of all saved games.

### Slot Data

It is here were all heavy data is contained!
All information serialized from actors and components is stored in the Slot Data, as well as any other data the game may need.

All levels, players, AIs and game systems configured to be saved are contained here.

## Slots in memory

However, an slot can exist in the game memory before being saved.
When an slot gets loaded, it will be "active" until we load another slot, allowing the plugin to, for example, support **Auto-Save** (save the current slot if any) or **Auto-Load** (load the last active slot).

It also allows us to save in memory at any time and then decide when we want to dump this information into a file.

Why would we want this?
Options are endless but, well, one example is saving specific levels at specific times and saving the current data into a file when a player reaches a checkpoint.