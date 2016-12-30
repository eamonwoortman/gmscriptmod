#######################################################################################
				GameMonkey ScriptMod 1.1


Author:				- Apologet
				- Dutchmeat

Credits:			- GameMonkey for their excelent scripting engine.

Current supported games:	- Wolfenstein Enemy Territory(+all mods)

#######################################################################################
(this readme is best viewed maximized)

INDEX

1) Introduction 
2) Features
3) Installation
4) Contact

===============
1) Introduction
===============

In this time you don't see an unmodded/pure gameserver anymore.
People are getting more interested in changing the behavior of the game.
That's why we've decided to help them a hand with GameMonkey ScriptMod,
or GMSM in short.

Some questions/answers:

- What is GameMonkey Script?

GameMonkey is a embedded scripting language that is intended for use in game and tool applications. 
GameMonkey is however suitable for use in any project requiring simple scripting support. 
GameMonkey borrows concepts from Lua (www.lua.org), but uses syntax similar to C, 
making it more accessible to game programmers. 
GameMonkey also natively supports multithreading and the concept of states. 

- What is GameMonkey ScriptMod?

GameMonkey ScriptMod is a QMM Plugin, which allows Server admins to define the server behavior by Script. 

- What is QMM?

Quake3 Multi Mod (short: QMM) is a hook to the Q3 Engine and allows you to extend the Quake3 Engine functions. 
Check http://qmm.planetquake.gamespy.com/ for more details about QMM. 

- Can I use GameMonkeyScriptMod on other Q3 Engines?

Yes, and No. Currently we support only Enemy Territory. 
But we think about a release for all QMM Supported Games. 
QMM currently supports Quake 3 Arena, Jedi Knight II: Jedi Outcast, Jedi Knight: Academy, 
Return to Castle Wolfenstein, and RtCW: Enemy Territory. 

- Can I use GameMonkeyScriptMod on my Windows Server?
Yes, and No. The gmScriptMod is written in Platform independent Code. 
But we haven't compiled a Windows Version yet. 
Most Gameservers are running Linux, so atm. we don't see the need for a Windows Version. 

- Where can i get some Informations about GameMonkey?
Here: http://www.somedude.net/gamemonkey/ 


########################################################################

===============
2) Features
===============

GMSM 1.1 features:

---------------------------
Mod features
---------------------------
- Alot of new hooks: GMSM_GAME_EVENTS, GAME_CLIENT_USERINFO_CHANGED, GAME_SHUTDOWN, GAME_INIT, GAME_CLIENT_BEGIN,
GAME_CLIENT_THINK, GAME_CLIENT_DISCONNECT, GAME_RUN_FRAME, and GET_CLIENT_VELOCITY_Z_IMPACT
- Two new server commands: slap, rslap
- One new client command: hjump
- New binded functions, such as: sscanf, strip, playsound(W:ET based games only), include, and more. 
Check http://gaminggone.net/gmScriptMod/gmScriptMod_functions/ for the function reference.
- The NeelixScript is now being debugged before executed, this way you can see if something is wrong when the game starts.

---------------------------
Neelix Script features
---------------------------
- A re-organised script structure
- Client Session example (you're now able to store player variables, from the playerlist table for example)
- Alot of new player and admin commands, such as: /ignore /unignore /whois /banana /music and much more
- New sound-command examples for ET based games
- A re-organised command system
- The kill event (EV_OBITUARY) is now hooked and used for messages like 'first blood', 'Killing spree', 'multikill',
but also 'last life: kills %d, attackers health: %d'.
- An adverisement system, using messages out of a table, and with a timer.
- And much more, so download the script and find out.

GMSM 1.0 features:

- 100% scriptable Admin Interface
- scriptable Rcon-like Interface
- Interface for Clan Members. (Beside Ref login)
- Interface for Clan Friends. (Beside Ref login)
- Access to your FileSystem by .gm Script Functions
- Works with ALL Enemy Territory Mods (etmain/etpro/etbub/shrubmod/tce/etc.)
- Easy to modify. No C/C++ knowledge neccessary.
- Change your ServerMod Scriptfile without Server restart.
- Easy Script Syntax. Not as complicated as LUA.
- A lot of Script Examples.
- Fast. GameMonkey is one of the fastest Script Languages.
- Define your own "/" commands. Example: "/ban ID", "/kick ID", "drop ID"
- Create an individual Gameserver Mod. Personalize your Server.
- Exchange your Script Files with other Serveradmins.
- Client to Client Private Chat
- IRC Like Channel Chat

#######################################################################

===============
3) Installation
===============

For help installing GMSM, go to:
http://gaminggone.net/gmScriptMod/gmscriptmod_installation_helpfiles/

#######################################################################

===============
4) Contact
===============

Do you have questions about GMSM, or do you want to chat about GMSM,
go to: www.gaminggone.net

#######################################################################


