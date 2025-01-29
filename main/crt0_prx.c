/**
 * nidPR - 2025
 * github.com/redhate
 **/

#include "headers/cdefs.h"

//Includes
#define _PSP_FW_VERSION 150
//kernel
#include <pspkernel.h>
#include <pspkerneltypes.h>
//module
#include <pspmodulemgr.h>
#include <pspmoduleinfo.h>
//thread
#include <pspthreadman.h>
#include <pspthreadman_kernel.h>
//ctrl
#include <pspctrl.h>
#include <pspctrl_kernel.h>
//sysmem
#include <pspsysmem.h>
#include <pspsysmem_kernel.h>
//display
#include <pspdisplay.h>
#include <pspdisplay_kernel.h>
//debug
#include <pspdebug.h>
//mem
#include <pspsysmem.h>
#include <pspsysmem_kernel.h>
//debug keyboard
#include "pspdebugkb/pspdebugkb.h"
//standard libs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//extras
#include "float/float.h"
#include "mips/mips.h"
#include "sha1/sha1.h"
#include "headers/nidlist.h"

enum PspModuleInfoAttrUltros{
	ULTROS_PSP_MODULE_ATTR_CANT_STOP		= 0x0001, //verified
	ULTROS_PSP_MODULE_ATTR_EXCLUSIVE_LOAD	= 0x0002, //verified
	ULTROS_PSP_MODULE_ATTR_EXCLUSIVE_START	= 0x0004, //verified
	ULTROS_PSP_MODULE_ATTR_UNK0             = 0x0006, //network and sound modules use this regardless of kernel or userspace. 0x1006, 0x0006
	ULTROS_PSP_MODULE_ATTR_UNK1             = 0x0007, //the most widely used.
};
	
enum PspModuleInfoTypeUltros{
	ULTROS_PSP_MODULE_USER					= 0x0000, //verified -not all userspace things... attributes arent inherited
	ULTROS_PSP_MODULE_ATTR_KERNEL			= 0x1000, //verified -most things initialized by system are this.
	ULTROS_PSP_MODULE_ATTR_KERNEL_PRX		= 0x3000, //sysexec modulemanager loadercore
	ULTROS_PSP_MODULE_UNK0					= 0x5000, //amctrl_driver and openpsid		
	ULTROS_PSP_MODULE_UNK1					= 0x7000, //only used by memlnd and mesgled 
};	

/*
//here for me...
// Attribute for threads.
enum PspThreadAttributesUltros
{
	//** Enable VFPU access for the thread.
	PSP_THREAD_ATTR_VFPU = 0x00004000,
	
	//** Start the thread in user mode (done automatically if the thread creating it is in user mode). 
	PSP_THREAD_ATTR_USER = 0x80000000,
	
	//** Thread is part of the USB/WLAN API. 
	PSP_THREAD_ATTR_USBWLAN = 0xa0000000,
	
	//** Thread is part of the VSH API. 
	PSP_THREAD_ATTR_VSH = 0xc0000000,
	
	//** Allow using scratchpad memory for a thread, NOT USABLE ON V1.0 
	PSP_THREAD_ATTR_SCRATCH_SRAM = 0x00008000,
	
	//** Disables filling the stack with 0xFF on creation 
	PSP_THREAD_ATTR_NO_FILLSTACK = 0x00100000,
	
	//** Clear the stack when the thread is deleted 
	PSP_THREAD_ATTR_CLEAR_STACK = 0x00200000,
};
*/

enum PspModuleVersion{
	MODULE_MAJOR = 1,
	MODULE_MINOR = 3,
};

//Defines
PSP_MODULE_INFO("nidPR", ULTROS_PSP_MODULE_ATTR_KERNEL_PRX|ULTROS_PSP_MODULE_ATTR_UNK1, MODULE_MAJOR, MODULE_MINOR); //0x3007
PSP_MAIN_THREAD_ATTR(0); //0 for kernel mode too
PSP_HEAP_SIZE_KB(-1024);

//thread id
SceUID mainThid,
       searchThid, 
       patchThid;

unsigned int module_start(SceSize args, void *argp) __attribute__((alias("_start")));
unsigned int  module_stop(SceSize args, void *argp) __attribute__((alias("_stop")));

unsigned int MenuEnableKey  = PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN;
unsigned int PatchEnableKey = PSP_CTRL_NOTE;

//plugin globals
void *vram;
unsigned int running = 1;
unsigned int t_addr[3];

//---------------------------MENU--------------------------
typedef struct menutype{
	unsigned int depth;
	unsigned int tab;
	unsigned int drawn;
}menutype;

menutype menu;

//-------------------------MODULES-------------------------
#define MOD_DISPLAY_LINES 27
#define MOD_LIST_MAX 256
typedef struct modtype{
	SceUID list[MOD_LIST_MAX];
	unsigned int select;
	unsigned int scroll;
	int count;
	SceUID selected_modid;
}modtype;

modtype modules;

int moduleLoadStart(const char *path, int flags, int type){
	
	SceUID mpid = 2;
	if (type == 0){
		mpid = 1;
	}
	
	SceKernelLMOption option;
	memset(&option, 0, sizeof(option));
	option.size = sizeof(option);
	option.mpidtext = mpid;
	option.mpiddata = mpid;
	option.position = 0;
	option.access = 1;

	SceUID loadResult = sceKernelLoadModule(path,flags,(type) ? &option : NULL);
	if (loadResult & 0x80000000){ return loadResult;}

	int status;
	SceUID startResult = sceKernelStartModule(loadResult, 0, NULL, &status, NULL);
	if (loadResult != startResult){ return startResult;}

	return 0;

}

int moduleStopUnload(SceUID uid, int flags, int type){
	
	SceUID mpid = 2;
	if (type == 0){
		mpid = 1;
	}
	
	SceKernelSMOption option;
	memset(&option, 0, sizeof(option));
	option.size = sizeof(option);
	option.mpidstack = 0;
	option.stacksize = 0x1000;
	option.priority = 11;
	option.attribute = 4001;

	/**
	 * Stop a running module.
	 *
	 * @param modid   - The UID of the module to stop.
	 * @param argsize - The length of the arguments pointed to by argp.
	 * @param argp    - Pointer to arguments to pass to the module's module_stop() routine.
	 * @param status  - Return value of the module's module_stop() routine.
	 * @param option  - Pointer to an optional ::SceKernelSMOption structure.
	 *
	 * @return ??? on success, otherwise one of ::PspKernelErrorCodes.
	 * int sceKernelStopModule(SceUID modid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option);
	 */

	if(sceKernelDeleteUID(uid)){
		return -1;
	}
	
	int status;
	//SceUID stopResult1 = sceKernelStopUnloadSelfModule(0, NULL,  &status, NULL);
	//if (stopResult1 & 0x80000000){ return stopResult1;}
	
	SceUID stopResult2 = sceKernelStopModule(uid,0,NULL,NULL,NULL);
	if (stopResult2 & 0x80000000){ return stopResult2;}

	SceUID unloadResult = sceKernelUnloadModule(uid);
	if (unloadResult & 0x80000000){ return unloadResult;}

	return 0;

}

//-------------------------STUBS----------------------------
#define STUB_DISPLAY_LINES 19
typedef struct stubtype{
	unsigned int select;
	unsigned int scroll;
	unsigned int count;
	unsigned int addr_select;
}stubtype;

stubtype stubs;

//-------------------------THREADS--------------------------
#define THREAD_DISPLAY_LINES 27
#define MAX_THREAD 128
typedef struct threadtype{
	unsigned int select;
	unsigned int scroll;
	unsigned int edit;
	char paused[MAX_THREAD]; //used to keep track of pausing independent threads
	SceUID selected_thid;
}threadtype;

threadtype threads;

SceKernelThreadInfo* fetchThreadInfo(){

	static SceKernelThreadInfo status[MAX_THREAD];

	int thread_count_now;
	SceUID thread_buf_now[MAX_THREAD];
	sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
	//SceKernelThreadInfo *status = malloc(sizeof(SceKernelThreadInfo) * MAX_THREAD);
	
	int x;
	for(x = 0; x < thread_count_now; x++){
		SceUID tmp_thid = thread_buf_now[x];
		memset(&status[x], 0, sizeof(SceKernelThreadInfo));
		status[x].size = sizeof(SceKernelThreadInfo);
		int ret = sceKernelReferThreadStatus(tmp_thid, &status[x]); 
	}
	
	return status;
}

//---------------------SANiK'S DECODER-----------------------
#define DECODE_DISPLAY_LINES 26
typedef struct decodetype{
	unsigned int options;
	unsigned int address;
	unsigned int start;
	unsigned int stop;
	unsigned int c,y,x;
	unsigned int store_y;
	unsigned int optsel[4];
	unsigned int storeaddress;
	char mempart;
}decodetype;

decodetype decoder;

void SetAddressBoundaries(){
	
	/* could be useful lol */
	/**
	 * #include <pspsysmem_kernel.h>
	typedef struct _PspSysmemPartitionInfo
	{
		SceSize size;
		unsigned int startaddr;
		unsigned int memsize;
		unsigned int attr;
	} PspSysmemPartitionInfo;

	 * Query the parition information
	 *
	 * @param pid  - The partition id
	 * @param info - Pointer to the ::PspSysmemPartitionInfo structure
	 *
	 * @return 0 on success.
	 * 
	 * int sceKernelQueryMemoryPartitionInfo(int pid, PspSysmemPartitionInfo *info);
		PspSysmemPartitionInfo PatitionInfo;
		sceKernelQueryMemoryPartitionInfo(decoder.address, &PatitionInfo);
		decoder.start=PatitionInfo.startaddr;
		decoder.stop=PatitionInfo.startaddr+PatitionInfo.memsize;
	 */
				
	
	//set userspace boundaries (24mb)
	if((decoder.address >= 0x08800000) && (decoder.address < 0x0A000000)){
		decoder.start=0x08800000;
		decoder.stop=0x0A000000;
		decoder.mempart = 0;
	}
	
	//set kernel addressing boundaries (4mb)
	else if((decoder.address >= 0x88000000) && (decoder.address < 0x88400000)){
		decoder.start=0x88000000;
		decoder.stop=0x88400000;
		decoder.mempart = 1;
	}
	
	//set vmem addressing boundaries (4mb)
	else if((decoder.address >= 0x04000000) && (decoder.address < 0x04200000)){
		decoder.start=0x04000000;
		decoder.stop=0x04200000;
		decoder.mempart = 2;
	}

}

//-----------------------BASIC PATCH--------------------------
/* quick hacky patching */
#define MAX_PATCHES 64
typedef struct patch{
	unsigned int addr;
	unsigned int data;
}patch;

#define PATCH_DISPLAY_LINES 25
typedef struct patchtype{
	unsigned int enable;
	unsigned int count;
	unsigned int select;
	unsigned int scroll;
	patch list[MAX_PATCHES];
}patchtype;

patchtype patches;

int patchThread(){
	
	//while the thread is running
	while(running){
		//if we have enabled patching
		if(patches.enable){
			//loop through the stored addresses and insert the data to memory
			int x;
			for(x=0;x<patches.count;x++){
				//boundary checking
				if(patches.list[x].addr != 0){
					//assignment here
					*((unsigned int*)(patches.list[x].addr)) = patches.list[x].data;
				}
			}
		}
		//delay the thread at a speed of 30hz so we dont hog up the cpu
		sceKernelDelayThread(T_SECOND/30);
	}
	
	//exit clean
	return 0;
}

int loadPatches(){
	//open the dat
	signed fd=sceIoOpen("ms0:/patch.dat", PSP_O_RDONLY, 0777);
	if(fd > 0){
		//read it and close it
		unsigned int file_size = sceIoLseek(fd, 0, SEEK_END); sceIoLseek(fd, 0, SEEK_SET);
		unsigned int len = (file_size/sizeof(patch));
		unsigned int bytes_read = sceIoRead(fd, patches.list, len);
		sceIoClose(fd);
		return 1;
	}
	return 0;
}

int savePatches(){
	//open the dat
	signed fd=sceIoOpen("ms0:/patch.dat", PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if(fd > 0){
		//write it and close it
		unsigned int bytes_read = sceIoWrite(fd, patches.list, sizeof(patch)*256);
		sceIoClose(fd);
		return 1;
	}
	return 0;
}

int addPatch(unsigned int addr, unsigned int data){
	
	//check boundaries
	if(addr == 0) return 1;
	if(patches.count >= MAX_PATCHES) return 2;
	
	//assignment
	patches.list[patches.count].addr = addr;
	patches.list[patches.count].data = data;
	
	//mutation
	patches.count++;
	
	return 0;
}

//create the list of nid hashes for the existing prototypes
void hashNidListPrototypes(){
	//loop through the nids
	unsigned int y;
	for(y=0;y<NID_LIST_SIZE-1;y++){
		//hash the prototype name to get the nid
		unsigned char hash[20];
		SHA1((const char*) nid_list[y].label, strlen(nid_list[y].label), hash);
		//assign it to the structured array
		nid_list[y].data = *(unsigned int*) hash;
	}
}

void menuUpdate(){
	
	pspDebugScreenInitEx(vram, 0, 0);
	
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenSetBackColor(BLACK);
	
	pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n NidPR - 2025 "); pspDebugScreenSetTextColor(WHITE);
	char vbuf[16]; pspDebugScreenSetTextColor(YELLOW); sprintf(vbuf, "v%d.%d ", MODULE_MAJOR, MODULE_MINOR); pspDebugScreenPuts(vbuf); 
	pspDebugScreenSetTextColor((menu.tab == 0)? MAGENTA : WHITE); pspDebugScreenPuts("[MODULES] ");
	pspDebugScreenSetTextColor((menu.tab == 1)? MAGENTA : WHITE); pspDebugScreenPuts("[THREADS] "); 
	pspDebugScreenSetTextColor((menu.tab == 2)? MAGENTA : WHITE); pspDebugScreenPuts("[BROWSER] "); 
	pspDebugScreenSetTextColor((menu.tab == 3)? MAGENTA : WHITE); pspDebugScreenPuts("[PATCH] "); 
	//pspDebugScreenSetTextColor((menu.tab == 4)? MAGENTA : WHITE); pspDebugScreenPuts("[MEMPART] "); 
	pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("\r\n-------------------------------------------------------------------\r\n");
	pspDebugScreenSetTextColor(WHITE);

	if(menu.tab == 0){ //module	
		sceKernelGetModuleIdList(modules.list, MOD_LIST_MAX, &modules.count);
		if(menu.depth == 0){
			
			char buffer[256];
			
			//sprintf(buffer, "mod count: %d\r\n", modules.count);
			//pspDebugScreenPuts(buffer);
			
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" UID       ATTR  VER    NAME\r\n");
			
			unsigned int x;
			for(x=0;x<MOD_DISPLAY_LINES;x++){ //limit line count
				//find the module by UID
				if(modules.list[x+modules.scroll] != 0){
					SceModule* modP = sceKernelFindModuleByUID(modules.list[x+modules.scroll]); //replace x with select
					unsigned char id = *(unsigned char*)(&modP->unknown2);
					if(id == 0) break; //check the module id in the list
					if (modP != 0){
						if(modP->ent_top != 0){
							if(modP->modname != 0){
								if(modules.select == x) pspDebugScreenSetTextColor(MAGENTA);
								else pspDebugScreenSetTextColor(WHITE);
								sprintf(buffer, " %08X", 	(unsigned int)modP->modid); pspDebugScreenPuts(buffer);
								sprintf(buffer, "  %04X", 	modP->attribute); pspDebugScreenPuts(buffer);
								sprintf(buffer, "  %02d.%02d", 	modP->version[1],modP->version[0]); pspDebugScreenPuts(buffer);
								sprintf(buffer, "  %s", 	modP->modname); pspDebugScreenPuts(buffer);
								//sprintf(buffer, "  %d ", 	modP->stubs.size); pspDebugScreenPuts(buffer);
								pspDebugScreenPuts("\r\n");
								modules.selected_modid=modP->modid;
							}
						}
					}
				}
			}
		
			//footer
			pspDebugScreenSetXY(0,31);
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetTextColor(GRAY);
			pspDebugScreenPuts(" (X) Stubs (Start) Info (O) Close Menu");
		
		}
		else if (menu.depth == 1){ //stub viewer with cross
			char buffer[256];
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" MEMBER       DATA\r\n");
			pspDebugScreenSetTextColor(WHITE);
			//find a module by its UID
			SceModule* modP = sceKernelFindModuleByUID(modules.list[modules.select+modules.scroll]);
			if (modP != NULL){
				if(modP->ent_top != NULL){
					if(modP->modname != NULL){
						//retreive the modules information
						SceLibraryEntryTable* entP = (SceLibraryEntryTable*)modP->ent_top;
						while ((unsigned int)entP < ((unsigned int)modP->ent_top + modP->ent_size)){
							if (entP->libname != NULL){
								sprintf(buffer, " module name: %s \r\n", 	modP->modname); pspDebugScreenPuts(buffer);
								//display the module information
								sprintf(buffer, " lib name:    %s \r\n", 		entP->libname); 								 pspDebugScreenPuts(buffer);
								//data is spurious here, they all show up as 0x0011 ...wtf
								//sprintf(buffer, "  version:    %04X (they cant all be 11...)\r\n", 	    *(unsigned short*)entP->version);              pspDebugScreenPuts(buffer);
								sprintf(buffer, " attribute:   %04X \r\n", 		entP->attribute); 			 					 pspDebugScreenPuts(buffer);
								sprintf(buffer, " len:         %d \r\n", 		entP->len); 									 pspDebugScreenPuts(buffer);
								sprintf(buffer, " vstubcount:  %d \r\n", 		entP->vstubcount); 								 pspDebugScreenPuts(buffer);
								sprintf(buffer, " stubcount:   %d \r\n", 		entP->stubcount); 								 pspDebugScreenPuts(buffer);
								pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(0xFFFFFFFF);
								pspDebugScreenSetTextColor(YELLOW); sprintf(buffer, " ADDR        NID       FUNCTION\r\n"); pspDebugScreenPuts(buffer); pspDebugScreenSetTextColor(0xFFFFFFFF);
								// loop through the nids found in the module
								unsigned int i;
								stubs.count = entP->stubcount + entP->vstubcount;
								unsigned int* nidtable = (unsigned int*)entP->entrytable;
								for (i = 0; i < STUB_DISPLAY_LINES; i++){
									if(i < stubs.count){
										stubs.addr_select=(unsigned int)&nidtable[stubs.select+stubs.scroll];
										if(stubs.select == i) pspDebugScreenSetTextColor(MAGENTA);
										else pspDebugScreenSetTextColor(WHITE);
										
										//unsigned int procAddr = nidtable[count+i];
										sprintf(buffer, " 0x%08X  %08X", &nidtable[i+stubs.scroll], nidtable[i+stubs.scroll]); pspDebugScreenPuts(buffer);
										//loop through the nids matching prototypes to nids
										unsigned int y;
										for(y=0;y<NID_LIST_SIZE-1;y++){
											if(nidtable[i+stubs.scroll] == nid_list[y].data){
												sprintf(buffer, "  %s", nid_list[y].label); pspDebugScreenSetTextColor(MAGENTA); pspDebugScreenPuts(buffer); pspDebugScreenSetTextColor(WHITE);
												//pspDebugScreenPuts("Match!");
											}
										}
										pspDebugScreenSetTextColor(WHITE);
										pspDebugScreenPuts("\r\n");
									}
								}
								break;
							}
							entP++;
						}
					}
				}
			}
			//footer
			pspDebugScreenSetXY(0,31);
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetTextColor(GRAY);
			pspDebugScreenPuts(" (X) Hop-to decoder.r (/\\) Search Prototype (O) Back");
		}
		else if (menu.depth == 3){ //this one is accurate now.
			
			char buffer[256];
			
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" MEMBER               DATA\r\n");
			pspDebugScreenSetTextColor(WHITE);
			
			/*
				//real sony
				typedef struct SceKernelModuleInfo {
					// version 0
					SceSize				size;
					char				nsegment;
					char				reserved[3];
					int					segmentaddr[PSP_KERNEL_MAX_MODULE_SEGMENT];
					int					segmentsize[PSP_KERNEL_MAX_MODULE_SEGMENT];
					unsigned int		entry_addr;
					unsigned int		gp_value;
					unsigned int		text_addr;
					unsigned int		text_size;
					unsigned int		data_size;
					unsigned int		bss_size;
					
					//----------unused----------------
					// version 1
					unsigned short		modattribute;
					unsigned char		modversion[2];	// minor, magor, etc...
														// minor 0 - major 1
					char				modname[PSP_KERNEL_MODULE_NAME_LEN];
					char				terminal;
					
				} SceKernelModuleInfo;
			*/
			
			SceKernelModuleInfo info;
			sceKernelQueryModuleInfo(modules.list[modules.select+modules.scroll], &info);
			
			// version 0
			//sprintf(buffer, " info.size           %d\r\n", (unsigned int)info.size);                pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.nsegment       %d\r\n", info.nsegment);        pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.reserved       %02hX, %0h2X, %02hX\r\n", (unsigned char)info.reserved[0], (unsigned char)info.reserved[1], (unsigned char)info.reserved[2]); pspDebugScreenPuts(buffer);
			
			int i = 0;
			for(i=0;i<info.nsegment;i++){
				sprintf(buffer, " info.segmentaddr[%d] %08X\r\n info.segmentsize[%d] %08X\r\n", i, info.segmentaddr[i], i, info.segmentsize[i]);
				pspDebugScreenPuts(buffer);
			}
			
			sprintf(buffer, " info.entry_addr     %08X\r\n", info.entry_addr);    pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.gp_value       %08X\r\n", info.gp_value);      pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.text_addr      %08X\r\n", info.text_addr);     pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.text_size      %08X\r\n", info.text_size);     pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.data_size      %08X\r\n", info.data_size);     pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.bss_size       %08X\r\n", info.bss_size);      pspDebugScreenPuts(buffer);
			
			/*
			// version 1
			sprintf(buffer, " info.modattribute  %04X\r\n", (unsigned short)info.modattribute);  pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.modversion    %d, %d\r\n", (unsigned char)info.modversion[0], (unsigned char)info.modversion[1]); pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.modname       %s\r\n",   (char*)info.modname);       pspDebugScreenPuts(buffer);
			sprintf(buffer, " info.terminal      %02hX",    (unsigned char)info.terminal);      pspDebugScreenPuts(buffer);
			*/
			
			t_addr[0]=(unsigned int)info.gp_value;
			t_addr[1]=(unsigned int)info.entry_addr;
			t_addr[2]=(unsigned int)info.text_addr;
			
		}
		else if (menu.depth == 2){ //full module info square
			
			char buffer[256];

			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" MEMBER       DATA\r\n");
			pspDebugScreenSetTextColor(WHITE);

			/*	//from pspsdk all fubar on 5.00 m33...
				typedef struct SceModule {
					struct SceModule	*next;
					unsigned short		attribute;
					unsigned char		version[2];
					char				modname[27];
					char				terminal;
					unsigned int		unknown1;
					unsigned int		unknown2;
					SceUID				modid;
					unsigned int		unknown3[4];
					void *				ent_top;
					unsigned int		ent_size;
					void *				stub_top;
					unsigned int		stub_size;
					unsigned int		unknown4[4];
					unsigned int		entry_addr;
					unsigned int		gp_value;
					unsigned int		text_addr;
					unsigned int		text_size;
					unsigned int		data_size;
					unsigned int		bss_size;
					unsigned int		nsegment;
					unsigned int		segmentaddr[4];
					unsigned int		segmentsize[4];
				} __attribute__((packed)) SceModule;
			*/
			
			typedef struct SceModuleUltros {
				struct SceModuleUltros	*next;
				unsigned short		attribute;
				unsigned char		version[2];
				char				modname[27];
				char				terminal; 	 //always 0
				unsigned int		level; 		 //seems like there are diff runlevels for plugins (maybe this is openlevel?)
				unsigned short		listid; 	 //the order in which it was plugged
				unsigned short		unk0; 	     //always 0001
				SceUID				uid; 		 //module uid
				 
				unsigned int		unk1; 		 //(runs 0 until sceInit, then turns to -1 for every other module)
				SceUID				otheruid; 	 //another uid just off a cunt hair from the main uid
				unsigned int		unk2; 		 // 0 or 1 (0 if loaded pre sceInit, 1 after sceInit, 2 from umd, and 2827303 from net mods, weird)
				unsigned int		unk3; 		 // 0 or 1 in unison with the above
				
				void *				ent_top;     //proper ent top
				unsigned int		ent_size;    //proper ent size
				void *				stub_top;    //proper stub top
				unsigned int		stub_size;   //proper stub size
				unsigned int		entry_addr1; //-1 on anything up to sceinit then is the same as entry addr 2
				unsigned int		unknown4[4]; //is it stack allocation? heap? its weird values it seems, not uid, not addressing
				unsigned int		entry_addr2; //proper entry
				unsigned int		gp_value;	 //proper gp
				unsigned int		text_addr;   //proper text addr
				unsigned int		text_size;   //proper text size
				unsigned int		data_size;   //proper data size
				unsigned int		bss_size;    //proper bss_size
				
				unsigned int		nsegment;    //proper num segments
				unsigned int		segmentaddr[4]; //proper segment addresses
				unsigned int		segmentsize[4]; //proper segment sizes
			} __attribute__((packed)) SceModuleUltros;
			
			//find a module by its UID
			SceModuleUltros* modP = (void*)sceKernelFindModuleByUID(modules.list[modules.select+modules.scroll]);
			if (modP != NULL){
				if(modP->ent_top != NULL){
					if(modP->modname != NULL){
						
						//module info
						sprintf(buffer, " next_mod:    %08X\r\n",				modP->next); 									pspDebugScreenPuts(buffer);
						sprintf(buffer, " attribute:   %04X \r\n", 	    		modP->attribute); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " version:     %02d.%02d \r\n", 		modP->version[1], modP->version[0]); 			pspDebugScreenPuts(buffer);
						sprintf(buffer, " modname:     %s \r\n", 				modP->modname); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " terminal:    %02hX\r\n",				modP->terminal); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " level:       %04X\r\n",				modP->level); 								pspDebugScreenPuts(buffer); //module position in list short right side
						sprintf(buffer, " listid:      %d\r\n",				    modP->listid); 									pspDebugScreenPuts(buffer);
						sprintf(buffer, " unk0:        %d\r\n",			    	modP->unk0); 	   								pspDebugScreenPuts(buffer);
						sprintf(buffer, " uid:         %08X \r\n", 	     	   (unsigned int)modP->uid); 						pspDebugScreenPuts(buffer);
						sprintf(buffer, " unk1:        %d \r\n", 	     	    modP->unk1); 									pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " otheruid:    %08X\r\n",				modP->otheruid); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " unk2:        %d\r\n",					modP->unk2); 									pspDebugScreenPuts(buffer);
						sprintf(buffer, " unk3:        %d\r\n",					modP->unk3); 									pspDebugScreenPuts(buffer);;
						//ent
						sprintf(buffer, " ent_top:     %08X\r\n",				modP->ent_top); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " ent_size:    %08X\r\n",				modP->ent_size); 								pspDebugScreenPuts(buffer);
						//stub
						sprintf(buffer, " stub_top:    %08X\r\n",				modP->stub_top); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " stub_size:   %08X\r\n",				modP->stub_size); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " entry1:      %08X\r\n",			    modP->entry_addr1); 							pspDebugScreenPuts(buffer);
						
						//segment stuff
						sprintf(buffer, " chunks:      %08X %08X %08X %08X\r\n", modP->unknown4[0],
																			     modP->unknown4[1],
																			     modP->unknown4[2],
																				 modP->unknown4[3]);
																																pspDebugScreenPuts(buffer);
						sprintf(buffer, " entry2:      %08X\r\n",			    modP->entry_addr2); 							pspDebugScreenPuts(buffer);
						
						//more
						sprintf(buffer, " gp:          %08X\r\n",				modP->gp_value); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " text_addr:   %08X text_size:  %08X\r\n",	modP->text_addr); 							pspDebugScreenPuts(buffer);
						sprintf(buffer, " data_size:   %08X\r\n",				modP->data_size); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " bss_size:    %08X\r\n",				modP->bss_size); 								pspDebugScreenPuts(buffer);
						sprintf(buffer, " nsegment:    %08X\r\n",				modP->nsegment); 								pspDebugScreenPuts(buffer);
						
						int i;
						for(i=0;i<modP->nsegment;i++){
							sprintf(buffer, " segmentaddr  %08X segmentsize %08X\r\n",		modP->segmentaddr[i], modP->segmentsize[i]); pspDebugScreenPuts(buffer);
						}
						
						//t_addr[0]=(unsigned int)modP->entry_addr;
						//t_addr[1]=(unsigned int)modP->gp_value;
						//t_addr[2]=(unsigned int)modP->text_addr;
						
					}
				}
			}
			//footer
			pspDebugScreenSetXY(0,31);
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetTextColor(GRAY);
			pspDebugScreenPuts(" (O) Back");
		
		}
	}
	else if(menu.tab == 1){ //thread
		if(menu.depth == 0){
			
			char buffer[256];
			
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" UID                                   NAME  ENTRY       PRIORITY\r\n");
			//update the thread info
			SceKernelThreadInfo *threadInfo = (SceKernelThreadInfo*)fetchThreadInfo();
			if(threadInfo != NULL){
				
				int thread_count_now;
				SceUID thread_buf_now[MAX_THREAD];
				sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
				
				//loop it
				int x;
				for(x = 0; x < THREAD_DISPLAY_LINES; x++){ //fix t his add scroll
					
					if(x > thread_count_now-1) break;
					
					//if selected
					if(threads.select == x){ 
						threads.selected_thid=thread_buf_now[x+threads.scroll];
						t_addr[1]=(unsigned int)threadInfo[x+threads.scroll].entry;
						pspDebugScreenSetTextColor(MAGENTA); 
					}
					//not selected
					else{ 
						pspDebugScreenSetTextColor(WHITE);  
					}
					
					//denote paused thread with line
					if(threads.paused[x+threads.scroll]){ 
						
						//if selected
						if(threads.select == x){ 
							pspDebugScreenSetBackColor(MAGENTA);
						}
						//not selected
						else{ 
							pspDebugScreenSetBackColor(CYAN);
						}
						
						//set text color to black regardless of which bg color was set
						pspDebugScreenSetTextColor(BLACK);
					}
					
					//print it out
					if(thread_buf_now[x+threads.scroll] != 0){
						sprintf(buffer, " %08X",      thread_buf_now[x+threads.scroll]);		          pspDebugScreenPuts(buffer);
						sprintf(buffer, "  %32s",     threadInfo[x+threads.scroll].name);			  pspDebugScreenPuts(buffer);
						sprintf(buffer, "  0x%08X",   threadInfo[x+threads.scroll].entry); 		      pspDebugScreenPuts(buffer);
						//sprintf(buffer, "  %08X",   threadInfo[x+threads.scroll].stack);		      pspDebugScreenPuts(buffer);
						sprintf(buffer, "  %d",       threadInfo[x+threads.scroll].currentPriority);   pspDebugScreenPuts(buffer);
						pspDebugScreenSetBackColor(BLACK);
						pspDebugScreenPuts("\r\n");
					}
					
				}
			}
			//footer
			pspDebugScreenSetXY(0,31);
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetTextColor(GRAY);
			pspDebugScreenPuts(" (X) Pause / Resume Thread (/\\) Kill Thread (O) Close Menu");
		}
		else if(menu.depth == 1){
			
			char buffer[256];
			
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" MEMBER             DATA\r\n");
			pspDebugScreenSetTextColor(WHITE);
			
			int thread_count_now;
			SceUID thread_buf_now[MAX_THREAD];
			sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
			
			int x;
			for(x = 0; x < thread_count_now; x++){
				if(x == threads.select+threads.scroll){
					//int thid;
					SceKernelThreadInfo status;
					//thid = sceKernelGetThreadId();
					memset(&status, 0, sizeof(SceKernelThreadInfo));
					
					status.size = sizeof(SceKernelThreadInfo);
					int ret = sceKernelReferThreadStatus(thread_buf_now[x], &status);
					//sprintf(buffer, "sceKernelReferThreadStatus Retval: %08X\n", ret); pspDebugScreenPuts(buffer);
					if(ret == 0){
						
						//sprintf(buffer, " size               %d\r\n",   (unsigned int) status.size);			   	
						//pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " name               %s\r\n",   status.name);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " attr               %08X\r\n",   status.attr);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " status             %d\r\n",   status.status);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " entry              %08X\r\n",   (unsigned int)status.entry);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " stack              %X\r\n",   (unsigned int)status.stack);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " stackSize          %d\r\n",   status.stackSize);			
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " gpReg              %08X\r\n",   (unsigned int)status.gpReg);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " initPriority       %d\r\n",   status.initPriority);	    
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " currentPriority    %d\r\n",   status.currentPriority);	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " waitType           %d\r\n",   status.waitType);			
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " waitId             %08X\r\n",   (unsigned int)status.waitId);			   	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " wakeupCount        %d\r\n",   status.wakeupCount);		
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " exitStatus         %d\r\n",   status.exitStatus);			
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " runClocks.low      %d\r\n",   status.runClocks.low);			
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " runClocks.hi       %d\r\n",   status.runClocks.hi);			
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " intrPreemptCount   %d\r\n",   status.intrPreemptCount);	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " threadPreemptCount %d\r\n",   status.threadPreemptCount);	
						pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " releaseCount       %d\r\n",   status.releaseCount);		
						pspDebugScreenPuts(buffer);
					
					
						t_addr[1]=(unsigned int)status.entry;
						t_addr[2]=(unsigned int)status.stack;
						t_addr[0]=(unsigned int)status.gpReg;
					}
				}
			}
		
			//footer
			pspDebugScreenSetXY(0,31);
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetTextColor(GRAY);
			pspDebugScreenPuts(" (O) Back");
		
		}
	}
	else if(menu.tab == 2){ //browser
		
		char buffer[256];
		
		//DRAW decoder.R
		pspDebugScreenSetBackColor(BLACK);
		if(decoder.options == 0){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       OPCODE   ARGS\r\n");
		}
		else if(decoder.options == 1){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       ASCII DECIMAL     FLOAT\r\n");
		}
		else if(decoder.options == 2){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       NID\r\n");
		}
		
		//Write out the RAM
		unsigned int i = 0;
		for(i=0; i<DECODE_DISPLAY_LINES; i++){
		
			//Apply the row color
			if(i == decoder.y){
				pspDebugScreenSetTextColor(MAGENTA);
			}
			else{
				pspDebugScreenSetTextColor(WHITE); 
			}
		  
			//Print out the address
			sprintf(buffer, " 0x%08lX  ", (decoder.address+(i*4)));
			pspDebugScreenPuts(buffer);
			
			//Print out the dword of memory
			sprintf(buffer, "%08lX  ", *((unsigned int*)(decoder.address+(i*4))));
			pspDebugScreenPuts(buffer);
			
			if(decoder.options == 0){
				//Print out the opcode
				mipsDecode(*((unsigned int*)(decoder.address+(i*4))));
			}
			else if(decoder.options == 1){
				//Print out the ASCII
				int x;
				for(x=0;x<4;x++){
					buffer[x]=*((unsigned char*)(((unsigned int)decoder.address+(i*4))+x)); if((buffer[x]<=0x20) || (buffer[x]==0xFF)) buffer[x]='.';
				}
				buffer[x]=0;
				pspDebugScreenPuts(buffer);					
				
				//Print out the decimal
				sprintf(buffer, "  %010lu  ", *((unsigned int*)(decoder.address+(i*4))));
				pspDebugScreenPuts(buffer);	
			  
				//Print out the float
				f_cvt(decoder.address+(i*4), buffer, sizeof(buffer), 6, MODE_GENERIC);
				pspDebugScreenPuts(buffer);
				
			}
			else if(decoder.options == 2){
				//loop through the list of nids
				unsigned int x;
				for(x = 0; x<NID_LIST_SIZE-1;x++){
					//get value of pointed data
					unsigned int val = *((unsigned int*)(decoder.address+(i*4)));
					//compare
					if(val == nid_list[x].data){
						//print
						sprintf(buffer, "%s  ", nid_list[x].label);
						pspDebugScreenPuts(buffer);
					}
				}
			}
			
			//Skip a line, draw the pointer =)
			if(i == decoder.y){
			
				//Skip the initial line
				pspDebugScreenPuts("\n");;
			
				//Skip the desired amount?
				pspDebugScreenPuts("   ");
				
				//Skip Address
				if(decoder.c != 0){
					pspDebugScreenPuts("          ");
					//Skip Hex
				}
			
				//Skip the minimalist amount
				unsigned char tempCounter=decoder.x;
				while(tempCounter){
					pspDebugScreenPuts(" "); 
					tempCounter--;
				}
			
				//Draw the symbol (Finally!!)
				pspDebugScreenSetTextColor((decoder.optsel[3])? YELLOW : CYAN);
				pspDebugScreenPuts("^");
			}
	  
			//Goto the next cheat down under
			pspDebugScreenPuts("\r\n");
		
		}
		
		//footer
		pspDebugScreenSetXY(0,31);
		pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetTextColor(GRAY);
		pspDebugScreenPuts(" (X) Edit ([]) Shift Up/Down (/\\) Add To Patch (O) Close Menu");

	}
	else if(menu.tab == 3){ //patches
		
		char buffer[256];
		sprintf(buffer," Patch Count: %d\r\n", patches.count); pspDebugScreenPuts(buffer);
		
		pspDebugScreenSetTextColor((patches.enable)? MAGENTA : CYAN); 
		pspDebugScreenPuts((patches.enable)? " Patcher Enabled\r\n" : " Patcher Disabled\r\n");
		
		pspDebugScreenSetTextColor(YELLOW); 
		if(decoder.options == 0){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       OPCODE   ARGS\r\n");
		}
		else if(decoder.options == 1){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       ASCII DECIMAL     FLOAT\r\n");
		}
		else if(decoder.options == 2){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       NID\r\n");
		}
		
		int i;
		for(i=0;i<PATCH_DISPLAY_LINES;i++){
			if(patches.list[i+patches.scroll].addr != 0){
				if(patches.select == i) pspDebugScreenSetTextColor(MAGENTA);
				else pspDebugScreenSetTextColor(WHITE);
				sprintf(buffer, " 0x%08X  %08X  ", patches.list[i+patches.scroll].addr, patches.list[i+patches.scroll].data);
				pspDebugScreenPuts(buffer);
				
				if(decoder.options == 0){
					//Print out the opcode
					mipsDecode(patches.list[i+patches.scroll].data);
				}
				else if(decoder.options == 1){
					
					//Print out the ASCII
					int x;
					for(x=0;x<4;x++){
						buffer[x]=*((unsigned char*)(((unsigned int)&patches.list[i+patches.scroll].data)+x)); if((buffer[x]<=0x20) || (buffer[x]==0xFF)) buffer[x]='.';
					}
					buffer[x]=0;
					pspDebugScreenPuts(buffer);
					
					//Print out the decimal
					sprintf(buffer, "  %010lu  ", patches.list[i+patches.scroll].data);
					pspDebugScreenPuts(buffer);
				  
					//Print out the float
					f_cvt((unsigned int)&patches.list[i+patches.scroll].data, buffer, sizeof(buffer), 6, MODE_GENERIC);
					pspDebugScreenPuts(buffer);
					
				}
				else if(decoder.options == 2){
					//loop through the list of nids
					unsigned int x;
					for(x = 0; x<NID_LIST_SIZE-1;x++){
						//compare
						if(patches.list[i+patches.scroll].data == nid_list[x].data){
							//print
							sprintf(buffer, "%s  ", nid_list[x].label);
							pspDebugScreenPuts(buffer);
						}
					}
				}
				
				pspDebugScreenPuts("\r\n");
				
			}
		}
		
		//footer
		pspDebugScreenSetXY(0,31);
		pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetTextColor(GRAY);
		pspDebugScreenPuts(" (O) Close Menu");
		
	}
	/* //just inserted this, need to look into this stuff more
	else if(menu.tab == 4){ //mem
		char buffer[256];

		PspSysmemPartitionInfo info;
		sceKernelQueryMemoryPartitionInfo(((SceUID)0), &info);
		sprintf(buffer, " free %d size %d startaddr %08X memsize %d attr %08X\r\n", info.size, info.startaddr, info.memsize, info.attr);
		pspDebugScreenPuts(buffer);
		
		sceKernelQueryMemoryPartitionInfo(((SceUID)1), &info);
		sprintf(buffer, " free %d size %d startaddr %08X memsize %d attr %08X\r\n", info.size, info.startaddr, info.memsize, info.attr);
		pspDebugScreenPuts(buffer);
		
		sceKernelQueryMemoryPartitionInfo(((SceUID)2), &info);
		sprintf(buffer, " free %d size %d startaddr %08X memsize %d attr %08X\r\n",  info.size, info.startaddr, info.memsize, info.attr);
		pspDebugScreenPuts(buffer);

		int i;
		for(i=1;i<=6;i++){
			unsigned int free = sceKernelPartitionTotalFreeMemSize(((SceUID)i));
			sprintf(buffer, " mempart %d free %d\r\n", i, free);
			pspDebugScreenPuts(buffer);
		}
		
		//footer
		pspDebugScreenSetXY(0,31);
		pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetTextColor(GRAY);
		pspDebugScreenPuts(" (O) Close Menu");
		
	}
	*/
	if(menu.tab == 2){
		if(decoder.mempart == 0){
			pspDebugScreenSetTextColor(MAGENTA); pspDebugScreenPuts("\r\n USER MEMORY ");
		}
		if(decoder.mempart == 1){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts("\r\n KERNEL MEMORY ");
		}
		if(decoder.mempart == 2){
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("\r\n VISUAL MEMORY ");
		}
	}

}

void menuControls(){
	
	menuUpdate();
	
	int real_module_count = 0;
	
	char kbbuffer[PSP_DEBUG_KB_MAXLEN];
	bzero(kbbuffer, PSP_DEBUG_KB_MAXLEN);
	
	SceCtrlData input;
	//Setup the controller
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);		
	
	//Loop for input
	while(1){
		
	    sceCtrlPeekBufferPositive(&input, 1);
		
		//Has the HOME button screwed up the VRAM blocks?
	    void *a_address;
	    int a_bufferWidth, a_pixelFormat, *a_sync;
	    
	   	sceDisplayGetFrameBufferInternal(0, &a_address, &a_bufferWidth, &a_pixelFormat, a_sync);
	
		//if a_address is 0 disable menu
		if(a_address == 0){
			//close nidPR menu
			menu.drawn=0;
			return;
		}
	
		//left over for debugging an old static list of nids / labels
		//check the list out see where it errors?
		/*
		if(input.Buttons & PSP_CTRL_TRIANGLE){
			
			char buffer[256];
			unsigned int y;
			for(y=0;y<NID_LIST_SIZE-1;y++){
				sceCtrlPeekBufferPositive(&input, 1);
				if(nid_list[y].label != NULL){
					
					//sprintf(buffer, "NAME %s \r\n", nid_list[y].label);
					//pspDebugScreenPuts(buffer);
					
					//hash the prototype name to get the nid
					unsigned char hash[20];
					SHA1((const unsigned char*) nid_list[y].label, strlen(nid_list[y].label), hash);
					
					
					//print the hash as a nid
					pspDebugScreenPuts("HASH ");
					int i;
					for(i=3;i>=0;i--){
						sprintf(buffer, "%02hX", hash[i]);
						pspDebugScreenPuts(buffer); 
					}
					pspDebugScreenPuts("\r\n");
					
					unsigned int hash_int = *(unsigned int*) hash;
					//sprintf(buffer, "HASH INT %08X \r\n", hash_int);
					//pspDebugScreenPuts(buffer);
					
					if(hash_int != nid_list[y].data){
						sprintf(buffer, "Found bad nid! \r\n");
						pspDebugScreenPuts(buffer);
						sprintf(buffer, "NAME %s \r\n", nid_list[y].label);
						pspDebugScreenPuts(buffer);
						sprintf(buffer, "HASH FROM SHA1 %08X \r\n", hash_int);
						pspDebugScreenPuts(buffer);
						sprintf(buffer, "DATA IN LIST %08X \r\n", nid_list[y].data);
						pspDebugScreenPuts(buffer);
					}
					
				}
				
				//print the data from the existing list
				if(nid_list[y].data != 0){
					sprintf(buffer, "DATA %08X \r\n", nid_list[y].data);
					pspDebugScreenPuts(buffer);
				}
				
				
				if(input.Buttons & PSP_CTRL_SELECT){
					break;
				}
			}
			
			sprintf(buffer, "SIZE OF LIST %d \r\n", y);
			pspDebugScreenPuts(buffer);
			
			sceKernelDelayThread(T_SECOND/5);
		}
		*/
		
		if(input.Buttons & PSP_CTRL_LTRIGGER){
			if(menu.tab > 0) menu.tab--;
			else menu.tab = 3;
			stubs.select = 0;
			stubs.scroll = 0;
			menu.depth = 0;
			menuUpdate();
			sceKernelDelayThread(T_SECOND/5);
		}
		if(input.Buttons & PSP_CTRL_RTRIGGER){
			if(menu.tab < 3) menu.tab++;
			else menu.tab = 0;
			stubs.select = 0;
			stubs.scroll = 0;
			menu.depth = 0;
			menuUpdate();
			sceKernelDelayThread(T_SECOND/5);
		}
		//modules
		if(menu.tab == 0){
			if(menu.depth == 0){
			
				if(input.Buttons & PSP_CTRL_UP){
					sceKernelGetModuleIdList(modules.list, MOD_LIST_MAX, &modules.count);
					unsigned int x;
					for(x=0;x<modules.count;x++){
						//find the module by UID
						if(modules.list[x+modules.scroll] != 0){
							SceModule* modP = sceKernelFindModuleByUID(modules.list[x+modules.scroll]); //replace x with select
							unsigned char id = *(unsigned char*)(&modP->unknown2);
							if(id > real_module_count) real_module_count=id;
						}
					}
					if(modules.select > 0){
						modules.select--;
					}
					else if(modules.select == 0){
						if(modules.scroll > 0){
							modules.scroll--; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_DOWN){
					unsigned int x;
					for(x=0;x<modules.count;x++){
						//find the module by UID
						if(modules.list[x+modules.scroll] != 0){
							SceModule* modP = sceKernelFindModuleByUID(modules.list[x+modules.scroll]); //replace x with select
							unsigned char id = *(unsigned char*)(&modP->unknown2);
							if(id > real_module_count) real_module_count=id;
						}
					}
					sceKernelGetModuleIdList(modules.list, MOD_LIST_MAX, &modules.count);
					if((modules.select < real_module_count-1) && (modules.select < MOD_DISPLAY_LINES-1)){
						modules.select++;
					}
					else if(modules.select == MOD_DISPLAY_LINES-1){ 
						if(modules.scroll < real_module_count-MOD_DISPLAY_LINES-1){
							modules.scroll++; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
			
				if(input.Buttons & PSP_CTRL_LEFT){
					unsigned int x;
					for(x=0;x<modules.count;x++){
						//find the module by UID
						if(modules.list[x+modules.scroll] != 0){
							SceModule* modP = sceKernelFindModuleByUID(modules.list[x+modules.scroll]); //replace x with select
							unsigned char id = *(unsigned char*)(&modP->unknown2);
							if(id > real_module_count) real_module_count=id;
						}
					}
					sceKernelGetModuleIdList(modules.list, MOD_LIST_MAX, &modules.count);
					if(modules.select > 0){
						modules.select--;
					}
					else if(modules.select == 0){
						if(modules.scroll > 0){
							modules.scroll--; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if(input.Buttons & PSP_CTRL_RIGHT){
					unsigned int x;
					for(x=0;x<modules.count;x++){
						//find the module by UID
						if(modules.list[x+modules.scroll] != 0){
							SceModule* modP = sceKernelFindModuleByUID(modules.list[x+modules.scroll]); //replace x with select
							unsigned char id = *(unsigned char*)(&modP->unknown2);
							if(id > real_module_count) real_module_count=id;
						}
					}
					sceKernelGetModuleIdList(modules.list, MOD_LIST_MAX, &modules.count);
					if((modules.select < real_module_count-1) && (modules.select < MOD_DISPLAY_LINES-1)){
						modules.select++;
					}
					else if(modules.select == MOD_DISPLAY_LINES-1){ 
						if(modules.scroll < real_module_count-MOD_DISPLAY_LINES-1){
							modules.scroll++; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}

				if (input.Buttons & PSP_CTRL_CROSS){
					menu.depth = 1;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if (input.Buttons & PSP_CTRL_START){
					menu.depth = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					int stop_unload_status = moduleStopUnload(modules.selected_modid, 0, 0);
					menuUpdate();
					pspDebugScreenSetXY(1,1);
					char buf[64];
					sprintf(buf,"stop_unload_status: %08X", stop_unload_status);
					pspDebugScreenPuts(buf);
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					menu.depth = 3;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stubs.select = 0;
					stubs.scroll = 0;
					modules.select = 0;
					modules.scroll = 0;
					threads.select = 0;
					threads.scroll = 0;
					menu.depth = 0;
					sceKernelDelayThread(T_SECOND/2);
					menu.drawn=0;
					return;
				}
				
			}
			else if(menu.depth == 1){
									
				if(input.Buttons & PSP_CTRL_UP){
					if(stubs.select > 0){
						stubs.select--;
					}
					else if(stubs.select == 0){
						if(stubs.scroll > 0){
							stubs.scroll--; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_DOWN){
					if((stubs.select < stubs.count-1) && (stubs.select < STUB_DISPLAY_LINES-1)){
						stubs.select++;
					}
					else if(stubs.select == STUB_DISPLAY_LINES-1){ 
						if(stubs.scroll < stubs.count-STUB_DISPLAY_LINES){
							stubs.scroll++; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_LEFT){
					if(stubs.select > 0){
						stubs.select--;
					}
					else if(stubs.select == 0){
						if(stubs.scroll > 0){
							stubs.scroll--; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if(input.Buttons & PSP_CTRL_RIGHT){
					if((stubs.select < stubs.count-1) && (stubs.select < STUB_DISPLAY_LINES-1)){
						stubs.select++;
					}
					else if(stubs.select == STUB_DISPLAY_LINES-1){ 
						if(stubs.scroll < stubs.count-STUB_DISPLAY_LINES){
							stubs.scroll++; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
							
				if(input.Buttons & PSP_CTRL_CROSS){
					stubs.select = 0;
					stubs.scroll = 0;
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=stubs.addr_select;
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stubs.select = 0;
					stubs.scroll = 0;
					menu.depth = 0;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					
					//this is ugly as sin but its late, im tired and i'm starting to care less, i just wanna crack nids ffs.
					pspDebugKbInit(kbbuffer);
					menuUpdate();
					
					//hash the keyboard string
					unsigned char hash[20];
					SHA1((const unsigned char*) kbbuffer, strlen(kbbuffer), hash);
					unsigned int hash_val = *(unsigned int*) hash;
					
					//find a module by its UID
					SceModule* modP = sceKernelFindModuleByUID(modules.list[modules.select+modules.scroll]);
					if (modP != NULL){
						if(modP->ent_top != NULL){
							if(modP->modname != NULL){
								//retreive the modules information
								SceLibraryEntryTable* entP = (SceLibraryEntryTable*)modP->ent_top;
								while ((unsigned int)entP < ((unsigned int)modP->ent_top + modP->ent_size)){
									if (entP->libname != NULL){
										// loop through the nids found in the module
										unsigned int i;
										stubs.count = entP->stubcount + entP->vstubcount;
										unsigned int* nidtable = (unsigned int*)entP->entrytable;
										for (i = 0; i < stubs.count; i++){
											if(nidtable[i] == hash_val){
												pspDebugScreenSetTextColor(YELLOW);
												char buffer[128];
												sprintf(buffer, "\r\n YAR! CAUGHT ME A MARLIN! \"%s\" %08X", kbbuffer, hash_val); pspDebugScreenPuts(buffer);
											}

										}
										break;
									}
									entP++;
								}
							}
						}
					}
				
					sceKernelDelayThread(T_SECOND/2);
				}

			}
			else if(menu.depth == 2){
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					//decoder.address=t_addr[0];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=t_addr[2];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CROSS){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=t_addr[1];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					menu.depth = 0;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
			}
			else if(menu.depth == 3){
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					menu.depth = 0;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
			}
		}
		//threads
		else if(menu.tab == 1){
			if(menu.depth == 0){
				
				int thread_count_now;
				SceUID thread_buf_now[MAX_THREAD];
				
				if(input.Buttons & PSP_CTRL_CROSS){
					//pause resume thread
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					if(!threads.paused[threads.select+threads.scroll]){
						sceKernelSuspendThread(thread_buf_now[threads.select+threads.scroll]);
					}
					else{
						sceKernelResumeThread(thread_buf_now[threads.select+threads.scroll]);
					}
					//flip the marker
					threads.paused[threads.select+threads.scroll]++;
					if(threads.paused[threads.select+threads.scroll] > 1){
						threads.paused[threads.select+threads.scroll] = 0;
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					sceKernelTerminateDeleteThread(thread_buf_now[threads.select+threads.scroll]);
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_UP){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					if(threads.select > 0){
						threads.select--;
					}
					else if(threads.select == 0){
						if(threads.scroll > 0){
							threads.scroll--; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_DOWN){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					if((threads.select < thread_count_now-1) && (threads.select < THREAD_DISPLAY_LINES-1)){
						threads.select++;
					}
					else if(threads.select == THREAD_DISPLAY_LINES-1){ 
						if(threads.scroll < thread_count_now-THREAD_DISPLAY_LINES){
							threads.scroll++; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_LEFT){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					if(threads.select > 0){
						threads.select--;
					}
					else if(threads.select == 0){
						if(threads.scroll > 0){
							threads.scroll--; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if(input.Buttons & PSP_CTRL_RIGHT){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					if((threads.select < thread_count_now-1) && (threads.select < THREAD_DISPLAY_LINES-1)){
						threads.select++;
					}
					else if(threads.select == THREAD_DISPLAY_LINES-1){ 
						if(threads.scroll < thread_count_now-THREAD_DISPLAY_LINES){
							threads.scroll++; 
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				//select and open info menu
				if(input.Buttons & PSP_CTRL_START){
					menu.depth = 1;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					sceKernelDeleteUID(modules.selected_modid);
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SELECT){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=t_addr[1];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stubs.select = 0;
					stubs.scroll = 0;
					modules.select = 0;
					modules.scroll = 0;
					threads.select = 0;
					threads.scroll = 0;
					menu.depth = 0;
					sceKernelDelayThread(T_SECOND/2);
					menu.drawn=0;
					return;
				}
				
			}
			else if(menu.depth == 1){
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=t_addr[0];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=t_addr[2];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CROSS){
					decoder.x=0;
					decoder.y=0;
					decoder.c=0;
					decoder.address=t_addr[1];
					SetAddressBoundaries();
					menu.tab = 2;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stubs.select = 0;
					stubs.scroll = 0;
					menu.depth = 0;
					menuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
			}
		}
		//browser
		else if(menu.tab == 2){

			if(input.Buttons & PSP_CTRL_TRIANGLE){
				
				addPatch(decoder.address+(decoder.y*0x04), *(unsigned int*)(decoder.address+(decoder.y*0x04)));
				//addPatch(0x08800000, 0xFFFFFFFF);
				
				menuUpdate();
				sceKernelDelayThread(T_SECOND/5);
			}

			if(input.Buttons & PSP_CTRL_SELECT){
				//go to kernel memory
				if(decoder.mempart == 0){
					decoder.mempart = 1;
					decoder.address=0x88000000;
					SetAddressBoundaries();
				}
				//go to userspace memory
				else if(decoder.mempart == 1){
					decoder.mempart = 2;
					decoder.address=0x04000000;
					SetAddressBoundaries();
				}
				//go to userspace memory
				else if(decoder.mempart == 2){
					decoder.mempart = 0;
					decoder.address=0x08800000;
					SetAddressBoundaries();
				}
				
				menuUpdate();
				sceKernelDelayThread(T_SECOND/5);
			}

			if(input.Buttons & PSP_CTRL_CIRCLE){
				stubs.select = 0;
				stubs.scroll = 0;
				modules.select = 0;
				modules.scroll = 0;
				threads.select = 0;
				threads.scroll = 0;
				menu.depth = 0;
				sceKernelDelayThread(T_SECOND/2);
				menu.drawn=0;
				return;
			}
			//flip the modes
			if(input.Buttons & PSP_CTRL_START){
				decoder.options++;
				if(decoder.options > 2){
					decoder.options=0;
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/2);
			}
			//follow useful pointers, full range is 0x08000000 - 0x0C000000, psp game memory is 0x08800000 - 0x09FFFFFF
			if((input.Buttons & PSP_CTRL_SQUARE) && (input.Buttons & PSP_CTRL_RIGHT)){
				if((*((unsigned int*)(decoder.address+(decoder.y*DECODE_DISPLAY_LINES))) >= 0x08800000) && 
				   (*((unsigned int*)(decoder.address+(decoder.y*DECODE_DISPLAY_LINES)))  < 0x0A000000))
				{
					decoder.store_y 		= decoder.y;
					decoder.storeaddress = decoder.address;
					decoder.address 		= *((unsigned int*)(decoder.address+(decoder.y*0x04)));
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/2);
			}
			//return to the pointer if you've followed one off into the memory abyss
			if((input.Buttons & PSP_CTRL_SQUARE) && (input.Buttons & PSP_CTRL_LEFT)){
				decoder.address = decoder.storeaddress;
				decoder.y = decoder.store_y;
				//decoder.address -= decoder.address+(decoder.y*DECODE_DISPLAY_LINES);
				menuUpdate();
				sceKernelDelayThread(T_SECOND/2);
			}					
			
			if(input.Buttons & PSP_CTRL_CROSS){
				decoder.optsel[3]=!decoder.optsel[3];
				menuUpdate();
				sceKernelDelayThread(T_SECOND/5);
			}
			
			if(input.Buttons & PSP_CTRL_LEFT){
				decoder.x--;
				switch(decoder.c){
					case 0: if((signed)decoder.x == -1) { decoder.x=0; } break;
					case 1: if((signed)decoder.x == -1) { decoder.x=7; decoder.c--; } break;
					case 2: if((signed)decoder.x == -1) { decoder.x=7; decoder.c--; } break;
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/10);
			}
			else if(input.Buttons & PSP_CTRL_RIGHT){
				decoder.x++;
				switch(decoder.c){
					case 0: if(decoder.x > 7) { decoder.x=0; decoder.c++; } break;
					case 1: if(decoder.x > 7) { decoder.x=7; } break;
					case 2: if(decoder.x > 7) { decoder.x=7; } break;
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/10);
			}

			if(decoder.optsel[3]){
				if(input.Buttons & PSP_CTRL_UP){
					switch(decoder.c){
						case 0:
							if(decoder.x==7){
								decoder.address+=4;
							}
							else{
								decoder.address+=(1 << (4*(7-decoder.x)));
							}
							if(decoder.address < decoder.start){
								decoder.address=decoder.start;
							}
							if(decoder.address > decoder.stop-DECODE_DISPLAY_LINES*0x04){
								decoder.address=decoder.stop-DECODE_DISPLAY_LINES*0x04;
							}
						break;
						case 1:
							*((unsigned int*)(decoder.address+(decoder.y*4)))+=(1 << (4*(7-decoder.x)));
							sceKernelDcacheWritebackInvalidateRange(((unsigned int*)(decoder.address+(decoder.y*4))),4);
							sceKernelIcacheInvalidateRange(((unsigned int*)(decoder.address+(decoder.y*4))),4);
						break;
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				else if(input.Buttons & PSP_CTRL_DOWN){
					switch(decoder.c){
						case 0:
							if(decoder.x==7){
								decoder.address-=4;
							}
							else{
								decoder.address-=(1 << (4*(7-decoder.x)));
							}
							if(decoder.address < decoder.start){
								decoder.address=decoder.start;
							}
							if(decoder.address > decoder.stop-DECODE_DISPLAY_LINES*0x04){
								decoder.address=decoder.stop-DECODE_DISPLAY_LINES*0x04;
							}
						break;
						case 1:
							*((unsigned int*)(decoder.address+(decoder.y*4)))-=(1 << (4*(7-decoder.x)));
							sceKernelDcacheWritebackInvalidateRange(((unsigned int*)(decoder.address+(decoder.y*4))),4);
							sceKernelIcacheInvalidateRange(((unsigned int*)(decoder.address+(decoder.y*4))),4);
						break;
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
			}
			else if(input.Buttons & PSP_CTRL_SQUARE){
				if(input.Buttons & PSP_CTRL_UP){
					if(decoder.address > decoder.start){
						decoder.address-=4;
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				else if(input.Buttons & PSP_CTRL_DOWN){
					if(decoder.address < decoder.stop-DECODE_DISPLAY_LINES*0x04){
						decoder.address+=4;
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
			}
			else{
				if(input.Buttons & PSP_CTRL_UP){
					if(decoder.y > 0){
						decoder.y--;
					}
					else if(decoder.y == 0){
						if(decoder.address > decoder.start){
							decoder.address-=0x04;
						}
					}
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				else if(input.Buttons & PSP_CTRL_DOWN){
					if(decoder.y < DECODE_DISPLAY_LINES-1){
						decoder.y++;
					}
					else if(decoder.y == DECODE_DISPLAY_LINES-1){
						if(decoder.address < (decoder.stop - DECODE_DISPLAY_LINES*0x04)){
							decoder.address+=0x04;
						}
					}	
					menuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}

			}

		}
		//patches
		else if(menu.tab == 3){
		
			if(input.Buttons & PSP_CTRL_START){
				if(decoder.options==0){
					decoder.options=1;
				}
				else if(decoder.options==1){
					decoder.options=2;
				}
				else if(decoder.options==2){
					decoder.options=0;
				}
				menuUpdate();
				sceKernelDelayThread(150000);
			}
		
			if(input.Buttons & PSP_CTRL_UP){
				if(patches.select > 0){
					patches.select--;
				}
				else if(patches.select == 0){
					if(patches.scroll > 0){
						patches.scroll--; 
					}
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/10);
			}
			
			if(input.Buttons & PSP_CTRL_DOWN){
				if((patches.select < patches.count-1) && (patches.select < PATCH_DISPLAY_LINES-1)){
					patches.select++;
				}
				else if(patches.select == PATCH_DISPLAY_LINES-1){ 
					if(patches.scroll < patches.count-PATCH_DISPLAY_LINES){
						patches.scroll++; 
					}
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/10);
			}
		
			if(input.Buttons & PSP_CTRL_LEFT){
				if(patches.select > 0){
					patches.select--;
				}
				else if(patches.select == 0){
					if(patches.scroll > 0){
						patches.scroll--; 
					}
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/120);
			}
			
			if(input.Buttons & PSP_CTRL_RIGHT){
				if((patches.select < patches.count-1) && (patches.select < PATCH_DISPLAY_LINES-1)){
					patches.select++;
				}
				else if(patches.select == PATCH_DISPLAY_LINES-1){ 
					if(patches.scroll < patches.count-PATCH_DISPLAY_LINES){
						patches.scroll++; 
					}
				}
				menuUpdate();
				sceKernelDelayThread(T_SECOND/120);
			}
		
			if(input.Buttons & PSP_CTRL_SELECT){
				if(!patches.enable) patches.enable = 1;
				else patches.enable = 0;
				menuUpdate();
				sceKernelDelayThread(T_SECOND/5);
			}
		
			if(input.Buttons & PSP_CTRL_CIRCLE){
				stubs.select = 0;
				stubs.scroll = 0;
				modules.select = 0;
				modules.scroll = 0;
				threads.select = 0;
				threads.scroll = 0;
				menu.depth = 0;
				sceKernelDelayThread(T_SECOND/2);
				menu.drawn=0;
				return;
			}
		
		}
	
		sceKernelDelayThread(T_SECOND/60);
		
	}
	
}

void buttonCallback(int curr, int last, void *arg){  
	if(vram==NULL) return;
	if(((curr & MenuEnableKey) == MenuEnableKey) && (!menu.drawn)){
		menu.drawn=1;
	}
}

int mainThread(){
	
	//Wait for the kernel to boot
	sceKernelDelayThread(100000);
	while(!sceKernelFindModuleByName("sceKernelLibrary"))
	sceKernelDelayThread(100000);
	sceKernelDelayThread(100000);
	sceKernelDelayThread(100000);
	sceKernelDelayThread(100000);
   
	//set initial browsing boundaries to userspace
	decoder.options = 0;
	decoder.address = decoder.start = 0x08800000;
	decoder.stop = 0x0A000000;
   
	//plug a module?, get eyes on it?
    //moduleLoadStart("ms0:/seplugins/a_plugin_to_snoop.prx", 0, 0);
   
	hashNidListPrototypes();
  
  	//Set the VRAM to null, use the current screen
	pspDebugScreenInitEx((unsigned int*)(0x44000000), 0, 0);
	vram=NULL;
  
	//Setup the controller
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  
	//Register the button callbacks 
	sceCtrlRegisterButtonCallback(2, PatchEnableKey | MenuEnableKey, buttonCallback, NULL);

	while(running){
		
		if(vram == NULL){
			
			//Has the HOME button been pressed?
			void *a_address;
			int a_bufferWidth, a_pixelFormat, *a_sync;
			sceDisplayGetFrameBufferInternal(0, &a_address, &a_bufferWidth, &a_pixelFormat, a_sync);
			if(a_address){ vram=(void*)(0xA0000000 | (unsigned int)a_address); }
			else{
				sceDisplayGetMode(&a_pixelFormat, &a_bufferWidth, &a_bufferWidth);
				pspDebugScreenSetColorMode(a_pixelFormat);
				pspDebugScreenSetXY(0, 0);
				pspDebugScreenSetTextColor(0xFFFFFFFF);
				pspDebugScreenPuts("nidPR");
			}

			sceKernelDelayThread(1500);
			continue;
		}
	    
	   	//Handle menu
	    if(menu.drawn){
		
			//Stop the game from receiving input (USER mode input block)
			sceCtrlSetButtonMasks(0xFFFF, 1);  // Mask lower 16bits
			sceCtrlSetButtonMasks(0x10000, 2); // Always return HOME key
	      
			//Setup a custom VRAM
			sceDisplaySetFrameBufferInternal(0, vram, 512, 0, 1);
			pspDebugScreenInitEx(vram, 0, 0);
	    
			//Draw menu
			menuControls();
	    
			//Return the standard VRAM
			sceDisplaySetFrameBufferInternal(0, 0, 512, 0, 1);
	      
			//Allow the game to receive input
			sceCtrlSetButtonMasks(0x10000, 0); // Unset HOME key
			sceCtrlSetButtonMasks(0xFFFF, 0);  // Unset mask
			
	    }
	    
	    sceKernelDelayThread(T_SECOND/30);
	    
	}

	return 0;
	
}

unsigned int _start(SceSize args, void *argp){
	
	patchThid = sceKernelCreateThread("nidPRpatchThread", patchThread, 0x11, 0x2000, 0, 0);
	if(patchThid >= 0) sceKernelStartThread(patchThid, 0, NULL);

	mainThid = sceKernelCreateThread("nidPRmainThread", &mainThread, 0x18, 0x2000, 0, NULL);
	if(mainThid >= 0) sceKernelStartThread(mainThid, 0, NULL);
	
	return 0;
	
}

unsigned int _stop(SceSize args, void *argp){
	
	running = 0;
	
	sceKernelTerminateThread(patchThid);
 	sceKernelTerminateThread(mainThid);
 	
	return 0;
	
}

