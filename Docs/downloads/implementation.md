# Implementation

## Blueprints

*Check the **[Test Project](https://mega.nz/#!JMowlKCA!wZv-L6oNSJCwDw1CUbTFyjPOXvd6viB-QLgK-u36xtY)** for a detailed example*

### Adding the interface

For an actor to have a faction, we need to add a FactionAgentInterface.
This will allow the system to set and get the current interface.

 <img align="right" width="300" height="100%" src="https://i.imgur.com/adet7y6.png"> <img align="right" width="200" height="100%" src="https://i.imgur.com/IJS8qG6.png">

To add an interface we have to go into **Class Settings**

From here we click on **Add**, inside Interfaces and search for FactionAgentInterface

### Getting and Settings the Faction

The system needs you to tell him how to set the faction and how to get it. This is done by overriding two functions as seen below.
The Faction variable that is seen is a normal blueprint variable, that can be edited normally.

 ![](https://i.imgur.com/imuFDXn.png)

 ![](https://i.imgur.com/Cf7QZju.png)

### Factions in Controllers

Sometimes you may want a controller to share a faction with its controlled pawn or character.

This process is exactly the same as before, except that instead of getting and setting a variable, we will get and set the faction from our pawn.

## C++