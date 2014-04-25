Demo plugin for commotiond
================================
This is a demo plugin for commotiond. It is intended to provide a skeleton for developing new plugins,
as well as demonstrating their basic structure. 

Dependencies
------------
This plugin has no dependencies

Changelog
---------
4/17/2014 - Initial release

Contact
-------
Grady Johnson <grady AT opentechinstitute DOT org>
https://commotionwireless.net


GETTING STARTED

Create a new directory in /commotiond/plugins/YOUR_PLUGIN

This will house your source code and CMake file.

BUILDING MAKEFILES WITH CMAKE

Copy in the CMakeLists.txt file from the demo directory, then edit the variables for PLUGIN_NAME and PLUGIN_SRC.

Next, go up one directory to /commotiond/plugins and add your plugin to the CMakeLists.txt file there, under:

  SET(PLUGINS serval-dna demo NEW_PLUGIN)


PLUGIN FUNCTIONS

REQUIRED:
int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params);
  Called by the Commotion daemon via co_plugins_load. This function registers the plugin (and 
  its included functions) with the list of available plugins.

int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);
  Called by the Commotion daemon via co_plugins_init. This function is used to 
  initialize the rest of your plugin (setting up data structures, registering commands, etc.)

OPTIONAL:
int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params);
  Called by the Commotion daemon via co_plugins_register. This function loads your plugin's
  schema to the existing commotion configuration or profile schemas.

int co_plugin_shutdown(co_obj_t *self, co_obj_t **output, co_obj_t *params);
  Called by the Commotion daemon via co_plugins_shutdown. This function is used to gracefully
  end any running plugin processes and free allocated memory.


