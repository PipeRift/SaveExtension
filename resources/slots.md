# Slots

In Save Extension, we refer to "slots" as an **instance of a saved game**. They can either be identified by a name or a number.

## Slots as files

Slots are saved into a single file named after the slot name.

To avoid having to load all save files entirely when, for example, displaying a list of slots in a menu, data is stored in two sections:

### Slot Info

Contains **lightweight** information about the saved game. Information we want to obtain without needing to load the rest of the slot.
Some examples are player level, xp, progress, player name, zone or area, current objective or thumbnail.

The game can access all slot infos very quickly without requiring to load the rest of the data.

### Slot Data

The bulk of any saved game.
All information serialized from actors and components is stored in the Slot Data, as well as any other data the game may need.

All levels, players, AIs and game systems configured to be saved are contained here.

## Slots in memory

However, an slot can exist in the game memory before being saved.
When an slot gets loaded, it will be "active" until we load another slot, allowing the plugin to, for example, support **Auto-Save** (save the current slot if any) or **Auto-Load** (load the last active slot).

It also allows us to save in memory at any time and then decide when we want to dump this information into a file.

Why would we want this?
Options are endless but, well, one example is saving specific levels at specific times and saving the current data into a file when a player reaches a checkpoint.