

# Saving & Loading

## Early concepts

### Save Manager

The save manager is in charge of loading, saving and all the rest of the logic (auto load, auto save, level streaming...). It will be initialized the first time **GetSaveManager** gets called. Usually at BeginPlay.

#### GetSaveManager

 ![Get Save Manager](img\get_save_manager.png)

### Slots

When we say **slot** we refer to the **integer that identifies a saved game**.

This slot identifies a saved game even during gameplay, meaning that if an slot gets loaded it will be "active" until we load another slot. 

With this we can for example do "*SaveCurrentSlot*" to pick the current slot if any and save it.

**Auto-Save** will also save the current slot, and **Auto-Load** will load the last current slot

## Saving

To save a game you can just get the Save Manager and then call *SaveGame to Slot*

#### Save to Slot

*Save into certain slot*

![Save Game to Slot](img\save_game_to_slot.png) 

#### Save Current Slot

*Save into the last slot that was loaded*

 ![Save Current Slot](img\save_current_slot.png)

#### Save Game from Info

*Saves a game based on a Slot-Info (Check [Slot Infos](slot-templates.md#slot-info))*

 ![SaveGame from Info](img\save_game_from_info.png) 

