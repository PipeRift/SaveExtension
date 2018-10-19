# Configuration

## Presets

---

**A preset is an asset that serves as a configuration preset for Save Extension.**

Within other settings, presets define how the world is saved, what is saved and what is not.

### Default Preset

Under *Project Settings* -> *Game* -> *Save Extension: Default Preset* you will find the default values for all presets.

![Default Preset](img/default_preset.png)

This default settings page is useful in case you have many presets, or in case you have none (because without any preset, default is used).

{% hint style='hint' %} All settings have defined tooltips describing what they are used for. Check them moving your mouse over the property. {% endhint %}

* **Gameplay**: Configures the runtime behavior of the plugin. Debug settings are also inside Gameplay. [Check Saving & Loading](saving&loading.md)
* **Serialization**: Toggle what to save from the world.
  * **Compression**: This settings can heavily reduce saved file sizes, but adds an extra cost to performance.
* **Asynchronous**: Should save & load be [asynchronous](asynchronous.md)?
* **Level Streaming**: Configures [Level Streaming](level-streaming.md) serialization

### Custom Presets

#### Setting a custom Preset

You can define which preset to use in editor inside *Project Settings* -> *Game* -> *Save Extension* 

![Active Preset](img/active_preset.png)

Because presets are assets, the active preset can be switched in runtime allowing different saving setups for different maps or game modes.

#### Creating a Custom Preset

![Creating a Preset](img\creating_preset.png)

You can create a new preset by right-clicking on the content browser -> *Save Extension* -> *Preset*

## Per-actor settings

---

Each actor blueprint can have its own configuration which is edited directly inside the blueprint:

<img  width=450 src="img\actor_settings.png">

<img width=300 align="left" src="img\open_actor_settings.png">

<br>

If you can't see *"Save Settings"* window opened it can be manually opened from **Window -> Save Settings**

<br><br><br>

#### Save settings

<img width=300 align="left" src="img\save_settings_zoom.png">

- **Save**: If false, this actor will be completely ignored while saving. *Disable this on all actor classes you don't want to save for performance.*
  - **Components**: Should components be considered for saving? *(Most components will still not be serialized for performance)*
  - **Transform**: Should save position, rotation and scale of this actor?
    - **Physics**: Should physics be saved? **Transform** is required to be enabled to save physics.
  - **Tags**: Should save actor tags?