Serval-DNA plugin for commotiond
================================
This plugin builds three components:

1. libcommotion_serval-dna - This is the main plugin library that 
     re-implements the [Serval][] Distributed Numbering Architecture
     (serval-dna) daemon.
2. serval-client - This is a client program that offers all of 
     serval-dna's built-in commands, as well as signing and signature 
     verification using Serval keys.
3. libcommotion_serval-sas - A library that implements a minimal
     MDP client for fetching an SAS signing key over the MDP overlay
     network. This will be deprecated once commotiond has a more full-
     featured event loop.

Dependencies
------------
This plugin depends on the serval-dna library, libservald.

Changelog
---------
12/18/2013 - Initial release

Contact
-------
Dan Staples (dismantl) <danstaples AT opentechinstitute DOT org>
https://commotionwireless.net

[Serval]: https://github.com/servalproject/serval-dna/