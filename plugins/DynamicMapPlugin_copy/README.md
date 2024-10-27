# GBS-DynamicMapPlugin-alpha
 Dynamic map plugin (alpha)

(WARNING: this plugin works for mono and color mode but not for color-only mode due to the VRAM tileset bank split)
The goal of this plugin is to be able to add the concept of metatiles to GBS
The concept of metatiles is to be able to create a set of specific tiles with specific data on it (like collision, palette, etc) while being able to read and edit those metatile in RAM.
metatiles can be made up of multiple tiles (usualy 4 8x8 tiles) but this plugin is designed for single 8x8 tiles.
This concept allow for things like dynamic collision and other dynamic behaviors.

Usage:
First you must create a tileset for both the metatile and the main scene to use.
the tileset must be 128px x 128px (16 x 16 tiles) for a total of 256 unique tiles (or 192 tiles if you are planning to use GBS default ui system to write text)

![meta_tileset](https://github.com/user-attachments/assets/03c61e5c-c400-46bd-b3cd-3b9e83ebdf87)

This tileset will be assigned as a common tileset to both the metatile scene and main scene.

Then you will create the metatile background, which will create all the unique metatile that will be used in the scene.
(it will usualy look similar to the tileset, but you can have multiple metatile with the same tile visual but have a different palette, collision or behavior for it)

![meta_data_tilemap](https://github.com/user-attachments/assets/b8b75abd-c369-4c00-8053-09ce34ea195e)

![image](https://github.com/user-attachments/assets/f2362d59-70a2-46fa-a9c0-c13c65f4444c)

Then in the metatile scene, you can set the palettes and collision data for each of those tiles.

Then, your main scene's background will use the tile from the tileset that will match the metatile that will be used.

![1-1](https://github.com/user-attachments/assets/463da62f-b9c1-4ae5-9297-887aa6392fd3)

And in the main scene's init script, use the "load metatiles" event to assign the metatile scene to the main scene for it to use.

And when played, the main scene will use the corresponding metatile including the pallete and collision.

![image](https://github.com/user-attachments/assets/c209f9a9-9dfb-4126-991f-022980ad665e)

"get metatile at position" event will return the id of the metatile at a position on screen
For example if I get the metatile at the position where the question block containing the mushroom is, it will return 156 (the 157th metatile, the one marked "9C" in the example tileset)

and "replace metatile" event will change the metatile at a position to the one specified
For example, when I destroy a brick block, I replace the metatile to the id 0 (1st metatile which is a basic empty tile)
