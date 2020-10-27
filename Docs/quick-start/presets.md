# Presets

A preset is a blueprint that allows the system to know the specifics about how to save the game. It contains all possible options and customizations the system can use.
That includes when, how and what to save, asset names, multithreading, custom logic,  etc.



## Create a preset

From the content browser, we can create a normal blueprint inheriting **USavePreset**, or just click *Save Extension -> Save Preset*

![Create Preset](img/content_browser_preset.png)

## Setting the active preset

To set the preset that the system will use when starting up, go to *Project Settings -> Game -> Save Extension*

![image-20201027234636490](img/default-preset.png)

Then we assign our preset class we want to use in **Preset**.

### In Blueprints

The preset can also be changed from blueprints in runtime calling:

![Set Active Preset](img/set-preset-bp.png)