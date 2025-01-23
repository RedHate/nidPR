a kernel mode plugin for tracing module / thread information and chasing down NIDS on sony psp 

How to use:
	Add the line ms0:/seplugins/nidPR.prx 1 to your seplugins/game.txt
	
	To open the plugin you must open and close the home menu once, 
	then use the volume up and down buttons together to draw the main menu.
	
	Triangle terminates threads, (working on terminating modules, not 
	currently working). It also opens the keyboard when you're in the 
	stub viewer portion of the module list.
	
	Cross will take you to the entry point of a thread and will take you into sub menus for modules.
	
	Start will display extra information on the various menus.
	
Note:
	when brute forcing nids with the debug keyboard, if you get a hit it will notify you on the bottom
	portion of the screen with a message, the prototypename and the nid value.
