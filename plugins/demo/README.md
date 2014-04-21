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
int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);

OPTIONAL:
int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_shutdown(co_obj_t *self, co_obj_t **output, co_obj_t *params);


