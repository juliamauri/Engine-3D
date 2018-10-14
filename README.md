# Red Eye Engine
[![Build status](https://ci.appveyor.com/api/projects/status/swrp9sgx89yxl493?svg=true)](https://ci.appveyor.com/project/cumus/redeye-engine)

3D Game Engine Sofware for academic purposes.
* Repository [Github](https://github.com/juliamauri/RedEye-Engine)
* Authors: [Julià Mauri Costa](https://github.com/juliamauri) & [Rubén Sardón](https://github.com/cumus)
* Professor: [Ricard Pillosu](https://github.com/d0n3val)
* University: [CITM UPC](https://www.citm.upc.edu/)
* License: [GNU General Public License v3.0](https://github.com/juliamauri/RedEye-Engine/blob/master/LICENSE)

## User Interface
### File
* **Exit**: closes engine.
### View
Each option toggles a hide/view window from a list of available windows:
* **Console**: Shows Engine's steps/errors. Used for debugging.
* **Configuration**: Shows information about engine's modules and allows the user to edit some engine utility.
* **Propieties**: Shows information about mesh droped.
* **Random Test**: For testing random number generation with ranges using int and float.
* **Texture Manager**: Shows textures loaded.
### Help
* **Open/Close ImGui Demo**: Opens/closes ImGui Demo window
* **Documentation**: opens browser to repository's wiki page
* **Download Latest**: opens browser to repository's realeses page
* **Report a Bug**: opens a browser to repository's issues page
* **About**: Shows engine info and 3rd party software.

## Advice
* Gameobject's mesh component kept causing troubles and for now, the class RE_UnregisteredMesh holds dropped mesh as an auxiliary patch. GameObject in SceneModule::drop holds it's transform and uses SceneModule::mesh_droped to draw and set properties.
* At the moment we still couldn't find the source of 10 memory leaks

## Release Versions
### v1.0
* Propieties: Shows mesh drop info and can change mesh texture
* Drop mesh
* Drop texture
* Config save/load
* Console with filters
* Scene with grid
* Camera with GLMath

### v0.0.1
* Random Test
* Renderer Test
* Geometry Test

##
![University Logo](https://www.citm.upc.edu/templates/new/img/logoCITM.png?1401879059)    
