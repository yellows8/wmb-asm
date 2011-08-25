/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#define DLLMAIN//This define is for wmb_asm.h, see that header for details
#include "..\dll\include\wmb_asm.h"

#ifndef NDS
#define APPNAME "wmb_asm.exe"
#endif
#ifdef NDS
#define APPNAME "wmb_asm.nds"
#endif

#define APPVERSION "v2.0b r3"//The string displayed for the version, when this program is ran without any parameters.

int ReadDump(int argc, char *argv[]);
int ReadCaptureLoop(char *cap, int argc, char *argv[], bool checkrsa, char *outdir, bool run, char *copydir, bool use_copydir);
void InitConsoleStuff();//Used for initializing on consoles.(NDS and such)

bool Stop=1;//If true at the end, display "Press any key to continue"(PC only)
bool ShowTime=1;//If true at the end, display how long the assembly took, in seconds.
bool Debug=0;//If true, write out a debug log.
bool *Has_AVS;

#ifdef NDS//There is no module loading available for homebrew on NDS, so fake module loading.
//Trick the code into thinking loading was successful. Actually, there's code in the source directory, which are named
//the same as the filenames in the module code, however, these include the code from the module.
//Yes, it's not a very good scheme, but doing it any other way would mean maintaining duplicate
//code. That would be a pain to maintain. One change in code in one file would mean copying the changed
//code/file to the other file.
bool LoadDLL(const char *filename, char *error_buffer = NULL)
{
return 1;
}
bool CloseDLL(char *error_buffer = NULL)
{
return 1;
}
void* LoadFunctionDLL(const char *func_name, char *error_buffer = NULL)
{
return NULL;
}
bool LoadAsmDLL(const char *filename ,char *error_buffer = NULL)
{
return 1;
}
bool CloseAsmDLL(char *error_buffer = NULL)
{
return 1;
}
#endif

int main(int argc, char *argv[])
{
		  #ifdef NDS//The DS can use argc/argv, however, there isn't any loaders/homebrew cards
		  //available that use it. So fake values for this must be used, since this program
		  //uses argc/argv.
          char *fixed_argv[2];
		  fixed_argv[0] = (char*)malloc(256);
		  fixed_argv[1] = (char*)malloc(256);
		  strcpy(fixed_argv[0],"/wmb_asm.nds");
          strcpy(fixed_argv[1],"/capture.cap");//captures directory in the root of the homebew card filesystem.

		  InitConsoleStuff();//Initialize DS-specific things. Such as the console, FAT/filesystem, and other things.
		  argc=2; argv = fixed_argv;
		  #endif

          if(argc==1)
          {
                     printf("%s %s by yellowstar built on %s, at %s.\n",APPNAME,APPVERSION,__DATE__,__TIME__);
                     printf("Usage:\n");
                     printf("wmb_asm.exe <options> [List of captures and directories]\n");
                     printf("Options:\n");
                     printf("-rsa The program will check the RSA-signature, with this option\n");
                     printf("-nds_capdir Assembled binaries will be written to the captures' directory(default)\n");
                     printf("-nds_curdir Assembled binaries will be written to the program's working/current directory\n");
                     printf("-nds_dirDir Assembled binaries will be written to Dir.\n");
                     printf("-run Opens the output once done as if you double-clicked it in Windows Explorer.\n");
                     printf("-stop Displays the \"Press any key to continue...\" when done.(Default)\n");
                     printf("-nostop Similar to the previous option, except that text isn't displayed, if the captures were assembled successfully.\n");
                     printf("-copy_dirDir Similar to -nds_dirDir, except the output is copied to Dir after the assembly.\n");
                     printf("-showtime Display how long the the whole assembly process took, at the end, in seconds.(Default)\n");
                     printf("-notime Similar to the previous option, except the time elapsed isn't displayed.\n");
                     printf("-debug Write to a debug log file. Intended for debugging, and fixing binaries not assembled correctly.\n");
                     printf("-nodebug Don't write to a debug log file.(Default)\n");
                     printf("Example: wmb_asm.exe -rsa -nds_dirbinaries capture1.cap capture2.cap captures more_captures...\n");
                     printf("With that, the RSA-siganture would be checked if ndsrsa.exe is in the current directory,\noutput would be written to the binaries directory,\ncapture1.cap, capture2.cap would be used,\nand captures and more_captures directories would be scanned for captures.\n");

                     system("PAUSE");
          }
          else
          {
                printf("%s\n",APPNAME);
                time_t start, end;
                start=time(NULL);

                char *error_buffer = (char*)malloc(256);

                    //Try to use the assembly module in the dll directory first, then try the working directory.
                    if(!LoadAsmDLL("dll/wmb_asm", error_buffer))//LoadAsmDLL takes care of the extension
                    {
                        if(!LoadAsmDLL("wmb_asm", error_buffer))
                        {
                            //If those fail, report the error, then quit.
                            printf("Error: %s\n",error_buffer);
                            system("PAUSE");
                            return 0;
                        }
                    }


                            Has_AVS = (bool*)malloc(sizeof(bool));
                            *Has_AVS = 0;

                            if(!ReadDump(argc,argv))
                            {
                                    if(!CloseAsmDLL(error_buffer))
                                    {
                                        printf("Error: %s\n",error_buffer);
                                        system("PAUSE");
                                    }
                                    else
                                    {
                                        system("PAUSE");
                                    }
                                free(error_buffer);
                                free(Has_AVS);
                                return 1;
                            }
                            else
                            {
                                end=time(NULL);

                                if(ShowTime)
                                    printf("Assembly took %d seconds.\n",((int)end-(int)start));

                                if(Stop)
                                    system("PAUSE");
                            }

                if(!CloseAsmDLL(error_buffer))
                {
                    printf("Error: %s\n",error_buffer);
                    system("PAUSE");
                }
            free(error_buffer);
            free(Has_AVS);

          }

    return 0;
}

//Called when assembly of a demo was successful
void AsmSuccessCallback()
{
    printf("Success!\n");
}

void HandleOptions(int i, char *argv[], bool *checkrsa, char *outdir, bool *use_capdir, bool *run, char *copydir, bool *use_copydir)
{
            if(strcmp(argv[i],"-rsa")==0)*checkrsa=1;//If the option -rsa is detected in the parameters, check the RSA-signature after assembly of each demo.
            if(strcmp(argv[i],"-nds_capdir")==0)*use_capdir=1;//If the option -nds_capdir is detected in the parameters, write the output/assembled demo to the same directory as the capture.
            if(strncmp(argv[i],"-nds_dir",8)==0)//If option -nds_dirDir is detected in the parametes, write the output to directory Dir.
            {
                *use_capdir=0;//Don't write output to the captures' directories.
                strncpy(outdir,&argv[i][8],strlen(argv[i])-8);//Copy in the target directory contained in the option.
                //If the directory doesn't already have a forward or back-slash, add a forward slash.
                if(outdir[strlen(outdir)]!='/' && outdir[strlen(outdir)]!='\\')
                {
                    int len=strlen(outdir);
                    outdir[len]='/';
                    outdir[len+1]=0;
                }

                    DIR *dir = opendir(outdir);
                    if(dir!=NULL)//Check if the directory exists.
                    {
                        closedir(dir);
                    }
                    else
                    {
                        //If the directory doesn't exist, attempt to create it.
                        #ifdef WIN32
                        if(mkdir(outdir)!=0)
                        #endif
                        #ifndef WIN32
                        if(mkdir(outdir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0)//Create with read/write/search permissions.
                        #endif
                        {
                            printf("Failed to create directory %s\n",outdir);
                            system("PAUSE");
                            #ifndef NDS
                            exit(1);
                            #endif
                        }
                    }
            }

            if(strncmp(argv[i],"-copy_dir",9)==0)//Similar to the -nds_dirDir option. Except this time,
            //the output is copied to this directory. So it would be written to one directory first, then copy it to Dir directory.
            {
                *use_capdir=0;//Don't write output to the captures' directories.
                *use_copydir=1;
                strncpy(copydir,&argv[i][9],strlen(argv[i])-9);//Copy in the directory the output will be copied to, which is contained in the option.
                if(copydir[strlen(copydir)]!='/' && outdir[strlen(copydir)]!='\\')
                {
                    //If the directory doesn't already have a forward or back-slash, add a forward slash.
                    int len=strlen(copydir);
                    copydir[len]='/';
                    copydir[len+1]=0;
                }

                    DIR *dir = opendir(copydir);
                    if(dir!=NULL)//Check if the directory exists.
                    {
                        closedir(dir);
                    }
                    else
                    {
                        //If the directory doesn't exist, attempt to create it.
                        #ifdef WIN32
                        if(mkdir(copydir)!=0)
                        #endif
                        #ifndef WIN32
                        if(mkdir(copydir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0)//Create with read/write/search permissions.
                        #endif
						{
                            printf("Failed to create directory %s\n",copydir);
                            system("PAUSE");
                            #ifndef NDS
                            exit(1);
                            #endif
                        }
                    }
            }

            if(strcmp(argv[i],"-nds_curdir")==0){strcpy(outdir,"");*use_capdir=0;}//If option -nds_curdir is detected, write the output to the working directory.

            if(strcmp(argv[i],"-run")==0)*run=1;//If the option -run is detected in the parameters, execute the output as if it was double-clicked in Windows Explorer.(That is, double-clicking the file. Windows only. )
            if(strcmp(argv[i],"-stop")==0)Stop=1;//If the option -stop is detected in the parameters, display "Press any key to continue..." at the end.(default)
            if(strcmp(argv[i],"-nostop")==0)Stop=0;//If the option -nostop is detected in the parameters, don't display "Press any key to continue..." at the end, if assembly was successful.
            if(strcmp(argv[i],"-showtime")==0)ShowTime=1;//If the option -showtime is detected in the parameters, display how long assembly took in seconds, at the end.(default)
            if(strcmp(argv[i],"-notime")==0)ShowTime=0;//If the option -notime is detected in the parameters, don't display how long assembly took.
            if(strcmp(argv[i],"-debug")==0)Debug=1;//If the option -debug is detected, write out a debug log.
            if(strcmp(argv[i],"-nodebug")==0)Debug=0;//Similar to the previous option, except don't write a debug log.(Default)
}

int ReadDump(int argc, char *argv[])
{
    bool checkrsa=0;//Do we check the RSA-signature after assembling each demo?
    char *outdir = (char*)malloc(256);//Where the output will be written too.
    memset(outdir,0,256);
    char *copydir = (char*)malloc(256);
    memset(copydir,0,256);
    bool use_copydir=0;//Do we copy the output to copydir?
    bool use_capdir=1;//Do we write the output to the captures' directories?
    bool run = 0;//Do we execute the output after assembly, as if double-clicked in Windows Explorer?(Windows only)
    Debug = 0;
    FILE_LIST *files_list = (FILE_LIST*)malloc(sizeof(FILE_LIST));//List that will contain the filenames of the captures
    FILE_LIST *cur_file;
    memset(files_list,0,sizeof(FILE_LIST));

    cur_file=files_list;
    for(int i=1; i<=argc-1; i++)//Go through all of the parameters, excluding the first one which contains the application's filename, in search of options, captures, and directories containing captures.
    {
        if(*argv[i]=='-')
        {
            //If the first character in the parameter, assume it's an option, and process that option.
            HandleOptions(i,argv,&checkrsa,outdir,&use_capdir,&run,copydir,&use_copydir);
            continue;
        }
        else
        {

            if(ScanDirectory(files_list,argv[i],(char*)".cap")==NULL)
            {
                //ScanDirectory will put the filename contained in argv[i], in files_list, if it's a file.
                //But if it's a directory, it will scan the whole directory for captures. Then if it finds a directory
                //during this scanning, it will repeat, and scan that directory for captures. And so on if it finds more directories in that directory.
                printf("Failed to open file or directory %s\n",argv[i]);
            }
        }
    }

    InitAsm(&AsmSuccessCallback, Debug);//Initialize the assembler

	   while(cur_file!=NULL)//Go through all the files ScanDirectory found.
	   {
	       if(cur_file->filename==NULL)break;//The last item in the list is empty, meaning filename is NULL too.(The last file is in the item before this)

	           if(use_capdir)//If we're using the captures directory for the outputs' directories, process the output directory name for each capture.
	           {

                    int pos=strlen(cur_file->filename);//Get the length of the capture
                    bool found=1;

                        while(pos<=0 || found)
                        {
                            if(pos==0){found=0;break;}//If we reached the start of the filename, write the output to the working directory.



                                if(cur_file->filename[pos]=='/' || cur_file->filename[pos]=='\\')//We found the slash for the directory. Stop, and increment the pos var.
                                {
                                    pos++;

                                    found=0;
                                    break;
                                }

                            pos--;
                        }
                    strncpy(outdir,(const char*)cur_file->filename,(size_t)pos);//Copy in the capture's directory name into the output directory name
               }

	       //Read and assemble the capture.
           if(ReadCaptureLoop(cur_file->filename,argc,argv,checkrsa,outdir,run,copydir,use_copydir))
           {
           //return 0;


           int error_code=0;
	       char *str = CaptureAsmReset(&error_code);

                if(str!=NULL)
                {
                        printf("ERROR: %s\n",str);//If we made it past the beacon assembly stage, report the error message.
                }

        }

           cur_file=cur_file->next;//Begin processing the next capture
      }

    free(outdir);//Free the output directory and copy to directory
    free(copydir);
    FreeDirectory(files_list);//Free the file list
    ExitAsm();//Deinitialize the assembler

	return 1;
}

int ReadCaptureLoop(char *cap, int argc, char *argv[], bool checkrsa, char *outdir, bool run, char *copydir, bool use_copydir)
{
    if(cap==NULL)return -1;//If the pointer to the capture's filename is NULL, abort.(Abort reading only this capture)

    pcap_t *fp=NULL;//Standard libpcap capture reading code. However, this program isn't using libpcap. It's using my own code for this, but with the exact same interface as libpcap.(The capture reading code is contained in the assembler module)
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_pkthdr *header;
	const u_char *pkt_data;
	int res;

    /* Open the capture file */
	if ((fp = pcap_open_offline(cap,			// name of the file
						 errbuf					// error buffer
						 )) == NULL)
	{
		printf("\nUnable to open the file %s.\n", cap);
		return 0;//If we fail to open the file, report this, and abort.
	}

    printf("Reading capture file %s\n",cap);
    *Has_AVS = CapHasAVS();

	/* Retrieve the packets from the file */
	while((res = pcap_next_ex(fp, &header, &pkt_data)) >= 0)
	{

        //Send the packet to the assembler to process & assemble.
        if(!HandlePacket(header,(u_char*)pkt_data, header->caplen,argv,fp,checkrsa,outdir,run,copydir,use_copydir,Has_AVS))return 0;

    }

	if (res == -1)
	{
		printf("Error reading the packets: %s\n", pcap_geterr(fp));
	}

	pcap_close(fp);
	//free(fp);//Gdb hangs on this call...(And on the free call in pcap_close in my capture reading code)

	return 1;
}
