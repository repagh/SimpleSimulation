# SimpleSimulation, A DUECA Project

## Introduction

This is a very simple simulation project that uses [DUECA
middleware](https://github.com/dueca/dueca). It is intended as a
demonstration project, you are encouraged not to look at this project
unless you are really stuck following the instructions in the DUECA
documentation on
[Setting up a simulation project](https://dueca.tudelft.nl/doc/example2.html).

## Application

This project uses the "flexi-stick" module from the FlexiStick project
to read joystick input, adds a simple dynamics module to demonstrate
development and implementation of DUECA/DUSIME modules, and uses the
"world-view" module from the WorldView project to show a 3D image.

This project also doubles as a part of DUECA's CI process, and it is checked
out and run

## The sequel

In an [add-on](https://dueca.tudelft.nl/doc/example2b.html) to the
initial tutorial, this project is expanded to a multi-player simulation,
in which multiple DUECA processes can be combined into a simulation
where multiple players occupy the same virtual world. A monitoring module
shows how this can be converted into an overview.

### Not in the tutorial

There are a few extensions that are not (yet) in the tutorial.

- The monitoring module has a visualization GUI using gtk4.
- There is a compass module that uses OpenGL to create a simple compass.
- Instead of the GL-base OpenSceneGraph as backend, this project can also
  use the Vulkan based VulkanSceneGraph

## Author(s)

René van Paassen

## LICENSE

EUPL-1.2
