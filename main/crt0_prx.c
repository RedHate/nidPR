/**
 * nidPR - 2025
 * github.com/redhate
 * 
 * big shout out to all the people who have remnant code in this project
 * there's a little tyranid, a little sanik, a little noeffex, and a lot
 * more! to infinity and beyond!
 * 
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
//standard libs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//extras
#include "headers/screenshot.h"
#include "float/float.h"
#include "mips/mips.h"
#include "sha1/sha1.h"
#include "headers/nidlist.h"


//Defines
PSP_MODULE_INFO("nidPR", 0x3007, 1, 2); //0x3007
PSP_MAIN_THREAD_ATTR(0); //0 for kernel mode too
PSP_HEAP_SIZE_KB(-1024);

//thread id
SceUID mainThid, searchThid, patchThid;

//vram pointer
void *vram;

unsigned int running = 1;
unsigned int menudepth = 0;
unsigned int menutab   = 0;
unsigned int menudrawn = 0;

#define MOD_DISPLAY_LINES 28
#define MOD_LIST_MAX 256
SceUID mod_list[MOD_LIST_MAX];
unsigned int mod_select = 0;
unsigned int mod_scroll = 0;
unsigned int mod_count = 0;
SceUID selected_modid = 0;

#define STUB_DISPLAY_LINES 20
unsigned int stub_select = 0;
unsigned int stub_scroll = 0;
unsigned int stub_count = 0;
unsigned int stub_addr_select = 0;

#define THREAD_DISPLAY_LINES 29
#define MAX_THREAD 128
static int thread_count_start, 
		   thread_count_now;

unsigned int thread_select=0;
unsigned int thread_scroll=0;
unsigned int thread_edit=0;
SceUID selected_thid;

#define DECODE_DISPLAY_LINES 27
unsigned int flip1 = 0;
unsigned int DecodeOptions = 0;
unsigned int DecodeAddress=0x08800000;
unsigned int DecodeAddressStart=0x08800000;
unsigned int DecodeAddressStop=0x0A000000;
unsigned int DecodeC, DecodeY, DecodeX;
unsigned int DecodeY_Stored;
unsigned int DecodeOptSelect[4]={0,0,0,0};
unsigned int DecodeAddress_Stored;
unsigned int ButtonAgeX=0;
unsigned int ButtonAgeY=0;

char RamViewMode = 0;

unsigned int t_addr[3]={0x00000000,0x00000000,0x00000000};

SceUID thread_buf_start[MAX_THREAD], thread_buf_now[MAX_THREAD];

unsigned int MenuEnableKey  = PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN;

unsigned int module_start(SceSize args, void *argp) __attribute__((alias("_start")));
unsigned int  module_stop(SceSize args, void *argp) __attribute__((alias("_stop")));

SceUID PauseUID = -1;

char thread_paused[MAX_THREAD]; //used to keep track of pausing independent threads

SceKernelThreadInfo status[MAX_THREAD];

SceKernelThreadInfo* fetchThreadInfo(){

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

#define MAX_PATCHES 64

typedef struct patch{
	unsigned int addr;
	unsigned int data;
}patch;

#define PATCH_DISPLAY_LINES 25
char patchEnable = 0;
char patch_count = 0;
char patch_select = 0;
char patch_scroll = 0;
patch patch_list[MAX_PATCHES];

int patchThread(){
	
	//while the thread is running
	while(running){
		//if we have enabled patching
		if(patchEnable){
			//loop through the stored addresses and insert the data to memory
			int x;
			for(x=0;x<patch_count;x++){
				//boundary checking
				if(patch_list[x].addr != 0){
					//assignment here
					*((unsigned int*)(patch_list[x].addr)) = patch_list[x].data;
				}
			}
		}
		//delay the thread so we dont hog up the cpu
		sceKernelDelayThread(T_SECOND/30);
	}
	
	//exit clean
	return 0;
}

int load_patches(){
	signed fd=sceIoOpen("ms0:/patch.dat", PSP_O_RDONLY, 0777);
	if(fd > 0){
		unsigned int file_size = sceIoLseek(fd, 0, SEEK_END); sceIoLseek(fd, 0, SEEK_SET);
		unsigned int len = (file_size/sizeof(patch));
		unsigned int bytes_read = sceIoRead(fd, patch_list, len);
		sceIoClose(fd);
		return 1;
	}
	return 0;
}

int save_patches(){
	signed fd=sceIoOpen("ms0:/patch.dat", PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if(fd > 0){
		unsigned int bytes_read = sceIoWrite(fd, patch_list, sizeof(patch)*256);
		sceIoClose(fd);
		return 1;
	}
	return 0;
}

int add_to_patches(unsigned int addr, unsigned int data){
	
	if(patch_count >= 256) return 2;
	if(addr == 0) return 1;
	
	patch_list[patch_count].addr = addr;
	patch_list[patch_count].data = data;
	
	patch_count++;
	
	return 0;
}

void HashNidListPrototypes(){
	//loop through the nids matching prototypes to nids
	unsigned int y;
	for(y=0;y<NID_LIST_SIZE-1;y++){
		//hash the prototype name to get the nid
		unsigned char hash[20];
		SHA1((const char*) nid_list[y].label, strlen(nid_list[y].label), hash);
		nid_list[y].data = *(unsigned int*) hash;
	}
}

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
		sceKernelQueryMemoryPartitionInfo(DecodeAddress, &PatitionInfo);
		DecodeAddressStart=PatitionInfo.startaddr;
		DecodeAddressStop=PatitionInfo.startaddr+PatitionInfo.memsize;
	 */
				
	
	//set userspace boundaries (24mb)
	if((DecodeAddress >= 0x08800000) && (DecodeAddress < 0x0A000000)){
		DecodeAddressStart=0x08800000;
		DecodeAddressStop=0x0A000000;
		RamViewMode = 0;
	}
	
	//set kernel addressing boundaries (4mb)
	else if((DecodeAddress >= 0x88000000) && (DecodeAddress < 0x88400000)){
		DecodeAddressStart=0x88000000;
		DecodeAddressStop=0x88400000;
		RamViewMode = 1;
	}
	
	//set vmem addressing boundaries (4mb)
	else if((DecodeAddress >= 0x04000000) && (DecodeAddress < 0x04200000)){
		DecodeAddressStart=0x04000000;
		DecodeAddressStop=0x04200000;
		RamViewMode = 2;
	}

}

unsigned int moduleLoadStart(const char *path, int flags, int type){
	
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

unsigned int moduleStopUnload(SceUID uid, int flags, int type){
	
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


#include <pspsysmem.h>
#include <pspsysmem_kernel.h>
void MenuUpdate(){
	
	pspDebugScreenInitEx(vram, 0, 0);
	
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenSetBackColor(BLACK);
	
	pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n NidPR - 2025 "); pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetTextColor((menutab == 0)? MAGENTA : WHITE); pspDebugScreenPuts("[MODULES] ");
	pspDebugScreenSetTextColor((menutab == 1)? MAGENTA : WHITE); pspDebugScreenPuts("[THREADS] ");
	pspDebugScreenSetTextColor((menutab == 2)? MAGENTA : WHITE); pspDebugScreenPuts("[BROWSER] ");
	pspDebugScreenSetTextColor((menutab == 3)? MAGENTA : WHITE); pspDebugScreenPuts("[PATCHES] ");
	pspDebugScreenSetTextColor((menutab == 4)? MAGENTA : WHITE); pspDebugScreenPuts("[PARTITIONS] ");
	
	pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("\r\n-------------------------------------------------------------------\r\n");
	pspDebugScreenSetTextColor(WHITE);


	
	if(menutab == 0){ //module
		
		sceKernelGetModuleIdList(mod_list, MOD_LIST_MAX, &mod_count);
		if(menudepth == 0){
			
			char buffer[256];
			
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" UID       ATTR  VER    NAME\r\n");
			
			unsigned int x;
			for(x=0;x<MOD_DISPLAY_LINES;x++){ //limit line count
				//find the module by UID
				if(&mod_list[x+mod_scroll] != NULL){
					SceModule* modP = sceKernelFindModuleByUID(mod_list[x+mod_scroll]); //replace x with select 
					if (modP != NULL){
						if(modP->ent_top != NULL){
							if(modP->modname != NULL){
								if(mod_select == x) pspDebugScreenSetTextColor(MAGENTA);
								else pspDebugScreenSetTextColor(WHITE);
								sprintf(buffer, " %08X", 	(unsigned int)modP->modid); pspDebugScreenPuts(buffer);
								sprintf(buffer, "  %04X", 	modP->attribute); pspDebugScreenPuts(buffer);
								sprintf(buffer, "  %02d.%02d", 	modP->version[1],modP->version[0]); pspDebugScreenPuts(buffer);
								sprintf(buffer, "  %s", 	modP->modname); pspDebugScreenPuts(buffer);
								//sprintf(buffer, "  %d ", 	modP->stub_size); pspDebugScreenPuts(buffer);
								pspDebugScreenPuts("\r\n");
								selected_modid=modP->modid;
							}
						}
					}
				}
			}
		}
		else if (menudepth == 1){ //stub viewer with cross
			char buffer[256];
			//find a module by its UID
			SceModule* modP = sceKernelFindModuleByUID(mod_list[mod_select+mod_scroll]);
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
								stub_count = entP->stubcount + entP->vstubcount;
								unsigned int* nidtable = (unsigned int*)entP->entrytable;
								for (i = 0; i < STUB_DISPLAY_LINES; i++){
									if(i < stub_count){
										stub_addr_select=(unsigned int)&nidtable[stub_select+stub_scroll];
										if(stub_select == i) pspDebugScreenSetTextColor(MAGENTA);
										else pspDebugScreenSetTextColor(WHITE);
										
										//unsigned int procAddr = nidtable[count+i];
										sprintf(buffer, " 0x%08X  %08X", &nidtable[i+stub_scroll], nidtable[i+stub_scroll]); pspDebugScreenPuts(buffer);
										//loop through the nids matching prototypes to nids
										unsigned int y;
										for(y=0;y<NID_LIST_SIZE-1;y++){
											if(nidtable[i+stub_scroll] == nid_list[y].data){
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
		}
		else if (menudepth == 9999){ //module info with triangle (this is shitty, i think we can do this better with my next test below)...
			
			char buffer[256];
			
			SceKernelModuleInfo info;
			sceKernelQueryModuleInfo(mod_list[mod_select+mod_scroll], &info);
			
			int i;
			pspDebugScreenPuts(" name:       ");
			for(i=0;i<strlen(info.name);i++){
				if(info.name[i]==0x00) break;
				sprintf(buffer, "%c", info.name[i]); 
				pspDebugScreenPuts(buffer);
			}
			// sprintf(buffer, "name %s\r\n", info.name); 
			// pspDebugScreenPuts(buffer);
			pspDebugScreenPuts("\r\n");
			
			sprintf(buffer, " attribute:   %04X\r\n", info.attribute); 
			pspDebugScreenPuts(buffer);
			
			sprintf(buffer, " version:     %2d.%d\r\n", (unsigned char)info.version[0], (unsigned char)info.version[1]); 
			pspDebugScreenPuts(buffer);
			
			
			sprintf(buffer, " size:        %d\r\n", (unsigned int) info.size); 
			pspDebugScreenPuts(buffer);
			
			pspDebugScreenPuts(" reserved:    ");
			for(i=0;i<3;i++){
				sprintf(buffer, "%02hX ", (unsigned char)info.reserved[i]);
				pspDebugScreenPuts(buffer);
			}
			pspDebugScreenPuts("\r\n");
			
			sprintf(buffer, " entry_addr:  %08X\r\n", info.entry_addr); 
			pspDebugScreenPuts(buffer);
			sprintf(buffer, " gp_addr:     %08X\r\n", info.gp_value); 
			pspDebugScreenPuts(buffer);
			sprintf(buffer, " text_addr:   %08X\r\n", info.text_addr); 
			pspDebugScreenPuts(buffer);
			sprintf(buffer, " text_size:   %08X\r\n", info.text_size); 
			pspDebugScreenPuts(buffer);
			sprintf(buffer, " data_size:   %08X\r\n", info.data_size); 
			pspDebugScreenPuts(buffer);
			sprintf(buffer, " bss_size:    %08X\r\n", info.bss_size); 
			pspDebugScreenPuts(buffer);
			pspDebugScreenPuts("\r\n");
			
			sprintf(buffer, " segments: %d\r\n", (unsigned char) info.nsegment); 	
			pspDebugScreenPuts(buffer);
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" ADDR     SIZE\r\n");
			pspDebugScreenSetTextColor(WHITE);

			for(i=0;i<info.nsegment;i++){
				sprintf(buffer, " %08X %08X\r\n", (unsigned int)info.segmentaddr[i], (unsigned int)info.segmentsize[i]); 
				pspDebugScreenPuts(buffer);
			}
			pspDebugScreenPuts("\r\n");
			
			t_addr[0]=(unsigned int)info.gp_value;
			t_addr[1]=(unsigned int)info.entry_addr;
			t_addr[2]=(unsigned int)info.text_addr;
			
		}
		else if (menudepth == 2){ //full module info square
			
			char buffer[256];
			
			//find a module by its UID
			SceModule* modP = sceKernelFindModuleByUID(mod_list[mod_select+mod_scroll]);
			if (modP != NULL){
				if(modP->ent_top != NULL){
					if(modP->modname != NULL){
						//retreive the modules information
						sprintf(buffer, " modname:    %s \r\n", 		    modP->modname); 						pspDebugScreenPuts(buffer);
						sprintf(buffer, " uid:        %08X \r\n", 	    (unsigned int)modP->modid); 			pspDebugScreenPuts(buffer);
						sprintf(buffer, " attribute:  %04X \r\n", 	    modP->attribute); 						pspDebugScreenPuts(buffer);
						sprintf(buffer, " version:    %02d.%02d \r\n", 	modP->version[1], modP->version[0]); 	pspDebugScreenPuts(buffer);

						sprintf(buffer, " next module addr: %08X\r\n",	modP->next); 		pspDebugScreenPuts(buffer);
						sprintf(buffer, " terminal:   %02hX\r\n",		modP->terminal); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " run level:  %08X\r\n",			modP->unknown1); 	pspDebugScreenPuts(buffer); //module position in list short right side
						
						sprintf(buffer, " id in list: %d\r\n",			 *(unsigned char*)(&modP->unknown2)); 			pspDebugScreenPuts(buffer);
						sprintf(buffer, " unknown:    %04X\r\n",			*((unsigned char*)(&modP->unknown2)) >> 16); 	pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " unknown:    %08X %08X %08X %08X\r\n",		modP->unknown3[0],modP->unknown3[1],modP->unknown3[2],modP->unknown3[3]); pspDebugScreenPuts(buffer);
						sprintf(buffer, " unknown:    %08X %08X %08X %08X\r\n",		modP->unknown4[0],modP->unknown4[1],modP->unknown4[2],modP->unknown4[3]); pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " ent_top:    %08X\r\n",			modP->ent_top); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " ent_size:   %08X\r\n",			modP->ent_size); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " stub_top:   %08X\r\n",			modP->stub_top); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " stub_size:  %08X\r\n",			modP->stub_size); 	pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " entry_addr: %08X\r\n",			modP->entry_addr); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " gp_value:   %08X\r\n",			modP->gp_value); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " text_addr:  %08X\r\n",			modP->text_addr); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " text_size:  %08X\r\n",			modP->text_size); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " data_size:  %08X\r\n",			modP->data_size); 	pspDebugScreenPuts(buffer);
						sprintf(buffer, " bss_size:   %08X\r\n",			modP->bss_size); 	pspDebugScreenPuts(buffer);
						
						sprintf(buffer, " nsegment:   %d\r\n",			modP->nsegment); 	pspDebugScreenPuts(buffer);
						
						/*
						int i;
						for(i=0;i<modP->nsegment;i++){
							sprintf(buffer, "  %08X %08X\r\n",		modP->segmentaddr[i], modP->segmentsize[i]); pspDebugScreenPuts(buffer);
						
						}
						*/
					
						t_addr[0]=(unsigned int)modP->entry_addr;
						t_addr[1]=(unsigned int)modP->gp_value;
						t_addr[2]=(unsigned int)modP->text_addr;
						
					}
				}
			}
		
		
		}
	}
	else if(menutab == 1){ //thread
		if(menudepth == 0){
			
			char buffer[256];
			
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenPuts(" UID                                 NAME  ENTRY       PRIORITY\r\n");
			//update the thread info
			SceKernelThreadInfo *threadInfo = (SceKernelThreadInfo*)fetchThreadInfo();
			if(threadInfo != NULL){
				sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
				//loop it
				int x;
				for(x = 0; x < thread_count_now; x++){ //fix t his add scroll
					
					//if selected
					if(thread_select+thread_scroll == x){ 
						selected_thid=thread_buf_now[x];
						t_addr[1]=(unsigned int)threadInfo[x].entry;
						pspDebugScreenSetTextColor(MAGENTA); 
					}
					//not selected
					else{ 
						pspDebugScreenSetTextColor(WHITE);  
					}
					
					//denote paused thread with line
					if(thread_paused[x]){ 
						
						//if selected
						if(thread_select+thread_scroll == x){ 
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
					if(&thread_buf_now[x] != NULL){
						sprintf(buffer, " %08X",      thread_buf_now[x]);		      pspDebugScreenPuts(buffer);
						sprintf(buffer, "  %30s",     threadInfo[x].name);			  pspDebugScreenPuts(buffer);
						sprintf(buffer, "  0x%08X",   threadInfo[x].entry); 		  pspDebugScreenPuts(buffer);
						//sprintf(buffer, "  %08X",   threadInfo[x].stack);		      pspDebugScreenPuts(buffer);
						sprintf(buffer, "  %d",       threadInfo[x].currentPriority); pspDebugScreenPuts(buffer);
						pspDebugScreenSetBackColor(BLACK);
						pspDebugScreenPuts("\r\n");
					}
					
				}
			}
		}
		else if(menudepth == 1){
			
			char buffer[256];
			
			sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
			int x;
			for(x = 0; x < thread_count_now; x++){
				if(x == thread_select){
					//int thid;
					SceKernelThreadInfo status;
					//thid = sceKernelGetThreadId();
					memset(&status, 0, sizeof(SceKernelThreadInfo));
					
					status.size = sizeof(SceKernelThreadInfo);
					int ret = sceKernelReferThreadStatus(thread_buf_now[x], &status);
					//sprintf(buffer, "sceKernelReferThreadStatus Retval: %08X\n", ret); pspDebugScreenPuts(buffer);
					if(ret == 0){
						sprintf(buffer, " name:          %s\r\n",   status.name); pspDebugScreenPuts(buffer);
						sprintf(buffer, " uid:           %08X\r\n", thread_buf_now[x]); pspDebugScreenPuts(buffer);
						sprintf(buffer, " entry:         %08X\r\n",   status.entry); pspDebugScreenPuts(buffer);
						sprintf(buffer, " stack addr:    %08X\r\n",   status.stack); pspDebugScreenPuts(buffer);
						sprintf(buffer, " stack size:    %08X\r\n", status.stackSize); pspDebugScreenPuts(buffer);
						sprintf(buffer, " gpReg:         %08X\r\n",   status.gpReg); pspDebugScreenPuts(buffer);
						sprintf(buffer, " init priority: %d\r\n",   status.initPriority); pspDebugScreenPuts(buffer);
						sprintf(buffer, " cur priority:  %d\r\n",   status.currentPriority); pspDebugScreenPuts(buffer);
						t_addr[1]=(unsigned int)status.entry;
						t_addr[2]=(unsigned int)status.stack;
						t_addr[0]=(unsigned int)status.gpReg;
					}
				}
			}
		}
	}
	else if(menutab == 2){ //browser
		
		char buffer[256];
		
		//DRAW DecodeR
		pspDebugScreenSetBackColor(BLACK);
		if(DecodeOptions == 0){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       OPCODE   ARGS\r\n");
		}
		else if(DecodeOptions == 1){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       ASCII DECIMAL     FLOAT\r\n");
		}
		else if(DecodeOptions == 2){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       NID\r\n");
		}
		
		//Write out the RAM
		unsigned int i = 0;
		for(i=0; i<DECODE_DISPLAY_LINES; i++){
		
			//Apply the row color
			if(i == DecodeY){
				pspDebugScreenSetTextColor(MAGENTA);
			}
			else{
				pspDebugScreenSetTextColor(WHITE); 
			}
		  
			//Print out the address
			sprintf(buffer, " 0x%08lX  ", (DecodeAddress+(i*4)));
			pspDebugScreenPuts(buffer);
			
			//Print out the dword of memory
			sprintf(buffer, "%08lX  ", *((unsigned int*)(DecodeAddress+(i*4))));
			pspDebugScreenPuts(buffer);
			
			if(DecodeOptions == 0){
				//Print out the opcode
				mipsDecode(*((unsigned int*)(DecodeAddress+(i*4))));
			}
			else if(DecodeOptions == 1){
				//Print out the ASCII
				buffer[0]=*((unsigned char*)(((unsigned int)DecodeAddress+(i*4))+0)); if((buffer[0]<=0x20) || (buffer[0]==0xFF)) buffer[0]='.';
				buffer[1]=*((unsigned char*)(((unsigned int)DecodeAddress+(i*4))+1)); if((buffer[1]<=0x20) || (buffer[1]==0xFF)) buffer[1]='.';
				buffer[2]=*((unsigned char*)(((unsigned int)DecodeAddress+(i*4))+2)); if((buffer[2]<=0x20) || (buffer[2]==0xFF)) buffer[2]='.';
				buffer[3]=*((unsigned char*)(((unsigned int)DecodeAddress+(i*4))+3)); if((buffer[3]<=0x20) || (buffer[3]==0xFF)) buffer[3]='.';
				buffer[4]=0;
				pspDebugScreenPuts(buffer);							
				
				//Print out the decimal
				sprintf(buffer, "  %010lu  ", *((unsigned int*)(DecodeAddress+(i*4))));
				pspDebugScreenPuts(buffer);	
			  
				//Print out the float
				f_cvt(DecodeAddress+(i*4), buffer, sizeof(buffer), 6, MODE_GENERIC);
				pspDebugScreenPuts(buffer);
				
			}
			else if(DecodeOptions == 2){
				//loop through the list of nids
				unsigned int x;
				for(x = 0; x<NID_LIST_SIZE-1;x++){
					//get value of pointed data
					unsigned int val = *((unsigned int*)(DecodeAddress+(i*4)));
					//compare
					if(val == nid_list[x].data){
						//print
						sprintf(buffer, "%s  ", nid_list[x].label);
						pspDebugScreenPuts(buffer);
					}
				}
			}
			
			//Skip a line, draw the pointer =)
			if(i == DecodeY){
			
				//Skip the initial line
				pspDebugScreenPuts("\n");;
			
				//Skip the desired amount?
				pspDebugScreenPuts("   ");
				
				//Skip Address
				if(DecodeC != 0){
					pspDebugScreenPuts("          ");
					//Skip Hex
				}
			
				//Skip the minimalist amount
				unsigned char tempCounter=DecodeX;
				while(tempCounter){
					pspDebugScreenPuts(" "); 
					tempCounter--;
				}
			
				//Draw the symbol (Finally!!)
				pspDebugScreenSetTextColor((DecodeOptSelect[3])? YELLOW : CYAN);
				pspDebugScreenPuts("^");
			}
	  
			//Goto the next cheat down under
			pspDebugScreenPuts("\r\n");
		
		}
		
	}
	else if(menutab == 3){ //patches
		
		char buffer[256];
		sprintf(buffer, (patchEnable)? "  Patch Count: %d\r\n  Patcher Enabled\r\n\r\n" : "  Patch Count: %d\r\n  Patcher Disabled\r\n\r\n", patch_count); pspDebugScreenPuts(buffer);
		pspDebugScreenSetTextColor(YELLOW); 
		if(DecodeOptions == 0){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       OPCODE   ARGS\r\n");
		}
		else if(DecodeOptions == 1){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       ASCII DECIMAL     FLOAT\r\n");
		}
		else if(DecodeOptions == 2){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts(" ADDRESS     HEX       NID\r\n");
		}
		
		int i;
		for(i=0;i<PATCH_DISPLAY_LINES;i++){
			if(patch_list[i+patch_scroll].addr != 0){
				if(patch_select == i) pspDebugScreenSetTextColor(MAGENTA);
				else pspDebugScreenSetTextColor(WHITE);
				sprintf(buffer, " 0x%08X  %08X  ", patch_list[i+patch_scroll].addr, patch_list[i+patch_scroll].data);
				pspDebugScreenPuts(buffer);
				
				if(DecodeOptions == 0){
					//Print out the opcode
					mipsDecode(patch_list[i+patch_scroll].data);
				}
				else if(DecodeOptions == 1){
					
					//Print out the ASCII
					buffer[0]=*((unsigned char*)(((unsigned int)&patch_list[i+patch_scroll].data)+0)); if((buffer[0]<=0x20) || (buffer[0]==0xFF)) buffer[0]='.';
					buffer[1]=*((unsigned char*)(((unsigned int)&patch_list[i+patch_scroll].data)+1)); if((buffer[1]<=0x20) || (buffer[1]==0xFF)) buffer[1]='.';
					buffer[2]=*((unsigned char*)(((unsigned int)&patch_list[i+patch_scroll].data)+2)); if((buffer[2]<=0x20) || (buffer[2]==0xFF)) buffer[2]='.';
					buffer[3]=*((unsigned char*)(((unsigned int)&patch_list[i+patch_scroll].data)+3)); if((buffer[3]<=0x20) || (buffer[3]==0xFF)) buffer[3]='.';
					buffer[4]=0;
					pspDebugScreenPuts(buffer);
					
					//Print out the decimal
					sprintf(buffer, "  %010lu  ", patch_list[i+patch_scroll].data);
					pspDebugScreenPuts(buffer);
				  
					//Print out the float
					f_cvt((unsigned int)&patch_list[i+patch_scroll].data, buffer, sizeof(buffer), 6, MODE_GENERIC);
					pspDebugScreenPuts(buffer);
					
				}
				else if(DecodeOptions == 2){
					//loop through the list of nids
					unsigned int x;
					for(x = 0; x<NID_LIST_SIZE-1;x++){
						//compare
						if(patch_list[i+patch_scroll].data == nid_list[x].data){
							//print
							sprintf(buffer, "%s  ", nid_list[x].label);
							pspDebugScreenPuts(buffer);
						}
					}
				}
				
				pspDebugScreenPuts("\r\n");
				
			}
		}
		
	}
	else if(menutab == 4){
		char a_buffer[64];
/*		
		PspSysmemPartitionInfo info;
		sceKernelQueryMemoryPartitionInfo(0x08800000, &info);
		sprintf(a_buffer, "free %d size %d startaddr %08X memsize %d attr %08X\r\n", info.size, info.startaddr, info.memsize, info.attr);
		pspDebugScreenPuts(a_buffer);
		
		sceKernelQueryMemoryPartitionInfo(0x88000000, &info);
		sprintf(a_buffer, "free %d size %d startaddr %08X memsize %d attr %08X\r\n", info.size, info.startaddr, info.memsize, info.attr);
		pspDebugScreenPuts(a_buffer);
		
		sceKernelQueryMemoryPartitionInfo(0x40000000, &info);
		sprintf(a_buffer, "free %d size %d startaddr %08X memsize %d attr %08X\r\n",  info.size, info.startaddr, info.memsize, info.attr);
		pspDebugScreenPuts(a_buffer);
*/

		int i;
		for(i=1;i<=6;i++){
			unsigned int free = sceKernelPartitionTotalFreeMemSize(i);
			
			sprintf(a_buffer, "partid %d free %d\r\n", i, free);
			pspDebugScreenPuts(a_buffer);
		}
	}
	//footer
	pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("-------------------------------------------------------------------\r\n"); pspDebugScreenSetTextColor(WHITE);
	if(menutab == 2){
		if(RamViewMode == 0){
			pspDebugScreenSetTextColor(MAGENTA); pspDebugScreenPuts("  USER MEMORY ");
		}
		if(RamViewMode == 1){
			pspDebugScreenSetTextColor(YELLOW); pspDebugScreenPuts("  KERNEL MEMORY ");
		}
		if(RamViewMode == 2){
			pspDebugScreenSetTextColor(CYAN); pspDebugScreenPuts("  VISUAL MEMORY ");
		}
	}

}

#include "pspdebugkb/pspdebugkb.h"

void MenuControls(){
	
	MenuUpdate();
	
	SceCtrlData input;
	
	char kbbuffer[PSP_DEBUG_KB_MAXLEN];
	bzero(kbbuffer, PSP_DEBUG_KB_MAXLEN);
	
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
			menudrawn=0;
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
			if(menutab > 0) menutab--;
			else menutab = 4;
			stub_select = 0;
			stub_scroll = 0;
			menudepth = 0;
			MenuUpdate();
			sceKernelDelayThread(T_SECOND/5);
		}
		if(input.Buttons & PSP_CTRL_RTRIGGER){
			if(menutab < 4) menutab++;
			else menutab = 0;
			stub_select = 0;
			stub_scroll = 0;
			menudepth = 0;
			MenuUpdate();
			sceKernelDelayThread(T_SECOND/5);
		}
		//modules
		if(menutab == 0){
			if(menudepth == 0){
			
				if(input.Buttons & PSP_CTRL_UP){
					if(mod_select > 0){
						mod_select--;
					}
					else if(mod_select == 0){
						if(mod_scroll > 0){
							mod_scroll--; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_DOWN){
					if((mod_select < mod_count-1) && (mod_select < MOD_DISPLAY_LINES-1)){
						mod_select++;
					}
					else if(mod_select == MOD_DISPLAY_LINES-1){ 
						if(mod_scroll < mod_count-MOD_DISPLAY_LINES){
							mod_scroll++; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_LEFT){
					if(mod_select > 0){
						mod_select--;
					}
					else if(mod_select == 0){
						if(mod_scroll > 0){
							mod_scroll--; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if(input.Buttons & PSP_CTRL_RIGHT){
					if((mod_select < mod_count-1) && (mod_select < MOD_DISPLAY_LINES-1)){
						mod_select++;
					}
					else if(mod_select == MOD_DISPLAY_LINES-1){ 
						if(mod_scroll < mod_count-MOD_DISPLAY_LINES){
							mod_scroll++; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if (input.Buttons & PSP_CTRL_CROSS){
					menudepth = 1;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
					
				}
				
				if (input.Buttons & PSP_CTRL_START){
					menudepth = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					int stop_unload_status = moduleStopUnload(selected_modid, 0, 0);
					MenuUpdate();
					pspDebugScreenSetXY(1,1);
					char buf[64];
					sprintf(buf,"stop_unload_status: %08X", stop_unload_status);
					pspDebugScreenPuts(buf);
					sceKernelDelayThread(T_SECOND/2);
				}
				
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stub_select = 0;
					stub_scroll = 0;
					mod_select = 0;
					mod_scroll = 0;
					thread_select = 0;
					thread_scroll = 0;
					menudepth = 0;
					sceKernelDelayThread(T_SECOND/2);
					menudrawn=0;
					return;
				}
				
			}
			else if(menudepth == 1){
									
				if(input.Buttons & PSP_CTRL_UP){
					if(stub_select > 0){
						stub_select--;
					}
					else if(stub_select == 0){
						if(stub_scroll > 0){
							stub_scroll--; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_DOWN){
					if((stub_select < stub_count-1) && (stub_select < STUB_DISPLAY_LINES-1)){
						stub_select++;
					}
					else if(stub_select == STUB_DISPLAY_LINES-1){ 
						if(stub_scroll < stub_count-STUB_DISPLAY_LINES){
							stub_scroll++; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_LEFT){
					if(stub_select > 0){
						stub_select--;
					}
					else if(stub_select == 0){
						if(stub_scroll > 0){
							stub_scroll--; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if(input.Buttons & PSP_CTRL_RIGHT){
					if((stub_select < stub_count-1) && (stub_select < STUB_DISPLAY_LINES-1)){
						stub_select++;
					}
					else if(stub_select == STUB_DISPLAY_LINES-1){ 
						if(stub_scroll < stub_count-STUB_DISPLAY_LINES){
							stub_scroll++; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
							
				if(input.Buttons & PSP_CTRL_CROSS){
					stub_select = 0;
					stub_scroll = 0;
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=stub_addr_select;
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stub_select = 0;
					stub_scroll = 0;
					menudepth = 0;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					
					//this is ugly as sin but its late, im tired and i'm starting to care less, i just wanna crack nids ffs.
					pspDebugKbInit(kbbuffer);
					MenuUpdate();
					
					//hash the keyboard string
					unsigned char hash[20];
					SHA1((const unsigned char*) kbbuffer, strlen(kbbuffer), hash);
					unsigned int hash_val = *(unsigned int*) hash;
					
					//find a module by its UID
					SceModule* modP = sceKernelFindModuleByUID(mod_list[mod_select+mod_scroll]);
					if (modP != NULL){
						if(modP->ent_top != NULL){
							if(modP->modname != NULL){
								//retreive the modules information
								SceLibraryEntryTable* entP = (SceLibraryEntryTable*)modP->ent_top;
								while ((unsigned int)entP < ((unsigned int)modP->ent_top + modP->ent_size)){
									if (entP->libname != NULL){
										// loop through the nids found in the module
										unsigned int i;
										stub_count = entP->stubcount + entP->vstubcount;
										unsigned int* nidtable = (unsigned int*)entP->entrytable;
										for (i = 0; i < stub_count; i++){
											if(nidtable[i] == hash_val){
												pspDebugScreenSetTextColor(YELLOW);
												char buffer[128];
												sprintf(buffer, " YAR! CAUGHT ME A MARLIN! \"%s\" %08X", kbbuffer, hash_val); pspDebugScreenPuts(buffer);
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
				
					sceKernelDelayThread(T_SECOND/2);
				}

			}
			else if(menudepth == 2){
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					//DecodeAddress=t_addr[0];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=t_addr[2];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CROSS){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=t_addr[1];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					menudepth = 0;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
			}
		}
		//threads
		else if(menutab == 1){
			if(menudepth == 0){
				
				if(input.Buttons & PSP_CTRL_CROSS){
					//pause resume thread
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					if(!thread_paused[thread_select+thread_scroll]){
						sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
						sceKernelSuspendThread(thread_buf_now[thread_select+thread_scroll]);
					}
					else{
						sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
						sceKernelResumeThread(thread_buf_now[thread_select+thread_scroll]);
					}
					//flip the marker
					thread_paused[thread_select+thread_scroll]++;
					if(thread_paused[thread_select+thread_scroll] > 1){
						thread_paused[thread_select+thread_scroll] = 0;
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thread_buf_now, MAX_THREAD, &thread_count_now);
					sceKernelTerminateDeleteThread(thread_buf_now[thread_select+thread_scroll]);
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}

				//up down
				if(input.Buttons & PSP_CTRL_UP){
					if(thread_select > 0){
						thread_select--;
					}
					else if(thread_select == 0){
						if(thread_scroll > 0){
							thread_scroll--; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				if(input.Buttons & PSP_CTRL_DOWN){
					if((thread_select < thread_count_now-1) && (thread_select < THREAD_DISPLAY_LINES-1)){
						thread_select++;
					}
					else if(thread_select == THREAD_DISPLAY_LINES-1){ 
						if(thread_scroll < thread_count_now-THREAD_DISPLAY_LINES){
							thread_scroll++; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/10);
				}
				
				//left right
				if(input.Buttons & PSP_CTRL_LEFT){
					if(thread_select > 0){
						thread_select--;
					}
					else if(thread_select == 0){
						if(thread_scroll > 0){
							thread_scroll--; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				if(input.Buttons & PSP_CTRL_RIGHT){
					if((thread_select < thread_count_now-1) && (thread_select < THREAD_DISPLAY_LINES-1)){
						thread_select++;
					}
					else if(thread_select == THREAD_DISPLAY_LINES-1){ 
						if(thread_scroll < thread_count_now-THREAD_DISPLAY_LINES){
							thread_scroll++; 
						}
					}
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/120);
				}
				
				//select and open info menu
				if(input.Buttons & PSP_CTRL_START){
					menudepth = 1;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					sceKernelDeleteUID(selected_modid);
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SELECT){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=t_addr[1];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stub_select = 0;
					stub_scroll = 0;
					mod_select = 0;
					mod_scroll = 0;
					thread_select = 0;
					thread_scroll = 0;
					menudepth = 0;
					sceKernelDelayThread(T_SECOND/2);
					menudrawn=0;
					return;
				}
				
			}
			else if(menudepth == 1){
				
				if(input.Buttons & PSP_CTRL_TRIANGLE){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=t_addr[0];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_SQUARE){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=t_addr[2];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CROSS){
					DecodeX=0;
					DecodeY=0;
					DecodeC=0;
					DecodeAddress=t_addr[1];
					SetAddressBoundaries();
					menutab = 2;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}
				
				if(input.Buttons & PSP_CTRL_CIRCLE){
					stub_select = 0;
					stub_scroll = 0;
					menudepth = 0;
					MenuUpdate();
					sceKernelDelayThread(T_SECOND/2);
				}

			}
		}
		//browser
		else if(menutab == 2){

			if(input.Buttons & PSP_CTRL_SELECT){
				
				add_to_patches(DecodeAddress+(DecodeY*0x04), *(unsigned int*)(DecodeAddress+(DecodeY*0x04)));
				//add_to_patches(0x08800000, 0xFFFFFFFF);
				
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/5);
			}

			if(input.Buttons & PSP_CTRL_TRIANGLE){
				//go to kernel memory
				if(RamViewMode == 0){
					RamViewMode = 1;
					DecodeAddress=0x88000000;
					SetAddressBoundaries();
				}
				//go to userspace memory
				else if(RamViewMode == 1){
					RamViewMode = 2;
					DecodeAddress=0x04000000;
					SetAddressBoundaries();
				}
				//go to userspace memory
				else if(RamViewMode == 2){
					RamViewMode = 0;
					DecodeAddress=0x08800000;
					SetAddressBoundaries();
				}
				
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/2);
			}

			if(input.Buttons & PSP_CTRL_CIRCLE){
				stub_select = 0;
				stub_scroll = 0;
				mod_select = 0;
				mod_scroll = 0;
				thread_select = 0;
				thread_scroll = 0;
				menudepth = 0;
				sceKernelDelayThread(T_SECOND/2);
				menudrawn=0;
				return;
			}
			//INPUT DecodeR
			//flip the modes
			if(input.Buttons & PSP_CTRL_START){
				if(DecodeOptions==0){
					DecodeOptions=1;
				}
				else if(DecodeOptions==1){
					DecodeOptions=2;
				}
				else if(DecodeOptions==2){
					DecodeOptions=0;
				}
				MenuUpdate();
				sceKernelDelayThread(150000);
			}
			//follow useful pointers, full range is 0x08000000 - 0x0C000000, psp game memory is 0x08800000 - 0x09FFFFFF
			if((input.Buttons & PSP_CTRL_SQUARE) && (input.Buttons & PSP_CTRL_RIGHT)){
				if((*((unsigned int*)(DecodeAddress+(DecodeY*DECODE_DISPLAY_LINES))) >= 0x08800000) && 
				   (*((unsigned int*)(DecodeAddress+(DecodeY*DECODE_DISPLAY_LINES)))  < 0x0A000000))
				{
					DecodeY_Stored 		= DecodeY;
					DecodeAddress_Stored = DecodeAddress;
					DecodeAddress 		= *((unsigned int*)(DecodeAddress+(DecodeY*0x04)));
				}
				MenuUpdate();
				sceKernelDelayThread(150000);
			}
			//return to the pointer if you've followed one off into the memory abyss
			if((input.Buttons & PSP_CTRL_SQUARE) && (input.Buttons & PSP_CTRL_LEFT)){
				DecodeAddress = DecodeAddress_Stored;
				DecodeY = DecodeY_Stored;
				//DecodeAddress -= DecodeAddress+(DecodeY*DECODE_DISPLAY_LINES);
				MenuUpdate();
				sceKernelDelayThread(150000);
			}					
			if(input.Buttons & PSP_CTRL_CROSS){
				DecodeOptSelect[3]=!DecodeOptSelect[3];
				MenuUpdate();
				sceKernelDelayThread(150000);
			}
			if(input.Buttons & PSP_CTRL_LEFT){
				DecodeX--;
				switch(DecodeC){
					case 0: if((signed)DecodeX == -1) { DecodeX=0; } break;
					case 1: if((signed)DecodeX == -1) { DecodeX=7; DecodeC--; } break;
					case 2: if((signed)DecodeX == -1) { DecodeX=7; DecodeC--; } break;
				}
				MenuUpdate();
				if(ButtonAgeX < 12) ButtonAgeX++; sceKernelDelayThread(150000-(10000*ButtonAgeX));
			}
			else if(input.Buttons & PSP_CTRL_RIGHT){
				DecodeX++;
				switch(DecodeC){
					case 0: if(DecodeX > 7) { DecodeX=0; DecodeC++; } break;
					case 1: if(DecodeX > 7) { DecodeX=7; } break;
					case 2: if(DecodeX > 7) { DecodeX=7; } break;
				}
				MenuUpdate();
				if(ButtonAgeX < 12) ButtonAgeX++; sceKernelDelayThread(150000-(10000*ButtonAgeX));
			}
			else{
					ButtonAgeX=0;
			}	
			if(DecodeOptSelect[3]){
				if(input.Buttons & PSP_CTRL_UP){
					switch(DecodeC){
						case 0:
							if(DecodeX==7){
								DecodeAddress+=4;
							}
							else{
								DecodeAddress+=(1 << (4*(7-DecodeX)));
							}
							if(DecodeAddress < DecodeAddressStart){
								DecodeAddress=DecodeAddressStart;
							}
							if(DecodeAddress > DecodeAddressStop-DECODE_DISPLAY_LINES*0x04){
								DecodeAddress=DecodeAddressStop-DECODE_DISPLAY_LINES*0x04;
							}
						break;
						case 1:
							*((unsigned int*)(DecodeAddress+(DecodeY*4)))+=(1 << (4*(7-DecodeX)));
							sceKernelDcacheWritebackInvalidateRange(((unsigned int*)(DecodeAddress+(DecodeY*4))),4);
							sceKernelIcacheInvalidateRange(((unsigned int*)(DecodeAddress+(DecodeY*4))),4);
						break;
					}
					MenuUpdate();
					if(ButtonAgeY < 12) ButtonAgeY++; sceKernelDelayThread(150000-(10000*ButtonAgeY));
				}
				else if(input.Buttons & PSP_CTRL_DOWN){
					switch(DecodeC){
						case 0:
							if(DecodeX==7){
								DecodeAddress-=4;
							}
							else{
								DecodeAddress-=(1 << (4*(7-DecodeX)));
							}
							if(DecodeAddress < DecodeAddressStart){
								DecodeAddress=DecodeAddressStart;
							}
							if(DecodeAddress > DecodeAddressStop-DECODE_DISPLAY_LINES*0x04){
								DecodeAddress=DecodeAddressStop-DECODE_DISPLAY_LINES*0x04;
							}
						break;
						case 1:
							*((unsigned int*)(DecodeAddress+(DecodeY*4)))-=(1 << (4*(7-DecodeX)));
							sceKernelDcacheWritebackInvalidateRange(((unsigned int*)(DecodeAddress+(DecodeY*4))),4);
							sceKernelIcacheInvalidateRange(((unsigned int*)(DecodeAddress+(DecodeY*4))),4);
						break;
					}
					MenuUpdate();
					if(ButtonAgeY < 12) ButtonAgeY++; sceKernelDelayThread(150000-(10000*ButtonAgeY));
				}
				else{
					ButtonAgeY=0;
				}
			}
			else if(input.Buttons & PSP_CTRL_SQUARE){
				if(input.Buttons & PSP_CTRL_UP){
					if(DecodeAddress > DecodeAddressStart){
						DecodeAddress-=4;
					}
					MenuUpdate();
					if(ButtonAgeY < 12) ButtonAgeY++; sceKernelDelayThread(150000-(10000*ButtonAgeY));
				}
				else if(input.Buttons & PSP_CTRL_DOWN){
					if(DecodeAddress < DecodeAddressStop-DECODE_DISPLAY_LINES*0x04){
						DecodeAddress+=4;
					}
					MenuUpdate();
					if(ButtonAgeY < 12) ButtonAgeY++; sceKernelDelayThread(150000-(10000*ButtonAgeY));
				}
			}
			else{
				if(input.Buttons & PSP_CTRL_UP){
					if(DecodeY > 0){
						DecodeY--;
					}
					else if(DecodeY == 0){
						if(DecodeAddress > DecodeAddressStart){
							DecodeAddress-=0x04;
						}
					}
					MenuUpdate();
					if(ButtonAgeY < 12) ButtonAgeY++; sceKernelDelayThread(150000-(10000*ButtonAgeY));
				}
				else if(input.Buttons & PSP_CTRL_DOWN){
					if(DecodeY < DECODE_DISPLAY_LINES-1){
						DecodeY++;
					}
					else if(DecodeY == DECODE_DISPLAY_LINES-1){
						if(DecodeAddress < (DecodeAddressStop - DECODE_DISPLAY_LINES*0x04)){
							DecodeAddress+=0x04;
						}
					}	
					MenuUpdate();
					if(ButtonAgeY < 12) ButtonAgeY++; sceKernelDelayThread(150000-(10000*ButtonAgeY));
				}
				else{
					ButtonAgeY=0;
				}
			}

		}
		//patches
		else if(menutab == 3){
		
			if(input.Buttons & PSP_CTRL_START){
				if(DecodeOptions==0){
					DecodeOptions=1;
				}
				else if(DecodeOptions==1){
					DecodeOptions=2;
				}
				else if(DecodeOptions==2){
					DecodeOptions=0;
				}
				MenuUpdate();
				sceKernelDelayThread(150000);
			}
		
			if(input.Buttons & PSP_CTRL_UP){
				if(patch_select > 0){
					patch_select--;
				}
				else if(patch_select == 0){
					if(patch_scroll > 0){
						patch_scroll--; 
					}
				}
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/10);
			}
			
			if(input.Buttons & PSP_CTRL_DOWN){
				if((patch_select < patch_count-1) && (patch_select < PATCH_DISPLAY_LINES-1)){
					patch_select++;
				}
				else if(patch_select == PATCH_DISPLAY_LINES-1){ 
					if(patch_scroll < patch_count-PATCH_DISPLAY_LINES){
						patch_scroll++; 
					}
				}
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/10);
			}
		
			if(input.Buttons & PSP_CTRL_LEFT){
				if(patch_select > 0){
					patch_select--;
				}
				else if(patch_select == 0){
					if(patch_scroll > 0){
						patch_scroll--; 
					}
				}
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/120);
			}
			
			if(input.Buttons & PSP_CTRL_RIGHT){
				if((patch_select < patch_count-1) && (patch_select < PATCH_DISPLAY_LINES-1)){
					patch_select++;
				}
				else if(patch_select == PATCH_DISPLAY_LINES-1){ 
					if(patch_scroll < patch_count-PATCH_DISPLAY_LINES){
						patch_scroll++; 
					}
				}
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/120);
			}
		
			if(input.Buttons & PSP_CTRL_SELECT){
				if(!patchEnable) patchEnable = 1;
				else patchEnable = 0;
				MenuUpdate();
				sceKernelDelayThread(T_SECOND/5);
			}
		
			if(input.Buttons & PSP_CTRL_CIRCLE){
				stub_select = 0;
				stub_scroll = 0;
				mod_select = 0;
				mod_scroll = 0;
				thread_select = 0;
				thread_scroll = 0;
				menudepth = 0;
				sceKernelDelayThread(T_SECOND/2);
				menudrawn=0;
				return;
			}
		
		}
	
		sceKernelDelayThread(T_SECOND/60);
		
	}
	
}

void ButtonCallback(int curr, int last, void *arg){  
	if(vram==NULL) return;
	if(((curr & MenuEnableKey) == MenuEnableKey) && (!menudrawn)){
		menudrawn=1;
	}
}

int cheatpause = 0;

int mainThread(){
	
	//Wait for the kernel to boot
	sceKernelDelayThread(100000);
	while(!sceKernelFindModuleByName("sceKernelLibrary"))
	sceKernelDelayThread(100000);
	sceKernelDelayThread(100000);
	sceKernelDelayThread(100000);
	sceKernelDelayThread(100000);
   
    //moduleLoadStart("ms0:/seplugins/nitePR.prx", 0, 0);
   
	HashNidListPrototypes();
  
  	//Set the VRAM to null, use the current screen
	pspDebugScreenInitEx((unsigned int*)(0x44000000), 0, 0);
	vram=NULL;
  
	//Setup the controller
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  
	//Register the button callbacks 
	sceCtrlRegisterButtonCallback(1,  MenuEnableKey, ButtonCallback, NULL);

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
	    if(menudrawn){
		
			//Stop the game from receiving input (USER mode input block)
			sceCtrlSetButtonMasks(0xFFFF, 1);  // Mask lower 16bits
			sceCtrlSetButtonMasks(0x10000, 2); // Always return HOME key
	      
			//Setup a custom VRAM
			sceDisplaySetFrameBufferInternal(0, vram, 512, 0, 1);
			pspDebugScreenInitEx(vram, 0, 0);
	    
			//Draw menu
			if(cheatpause) sceKernelSuspendThread(mainThid);
			MenuControls();
			sceKernelResumeThread(mainThid);
	    
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

