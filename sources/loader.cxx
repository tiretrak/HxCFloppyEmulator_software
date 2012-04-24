/*
//
// Copyright (C) 2006-2012 Jean-Fran�ois DEL NERO
//
// This file is part of HxCFloppyEmulator.
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : loader.cxx
// Contains: image loader functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
}
#include "main.h"
#include "loader.h"

extern s_gui_context * guicontext;

extern track_type track_type_list[];

extern char * basename (const char *name);

int load_floppy(FLOPPY * floppydisk)
{
	int ret;
	int oldifmode;

	hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);
	guicontext->loadedfloppy=0;
	guicontext->loadedfloppy=floppydisk;

	ret=HXCFE_NOERROR;
	guicontext->loadstatus=ret;

	if(ret!=HXCFE_NOERROR || !guicontext->loadedfloppy)
	{
		guicontext->loadedfloppy=0;
		guicontext->bufferfilename[0]=0;
	}
	else
	{
		oldifmode=libusbhxcfe_getInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe);
		libusbhxcfe_setInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy->floppyiftype,guicontext->loadedfloppy->double_step,0);
		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);

		if(!guicontext->autoselectmode)
		{	// keep the old interface mode
			//hwif->interface_mode=oldifmode;
		};

		sprintf(guicontext->bufferfilename,"Floppy Dump");
	}

	return ret;
}

int load_floppy_image(char *filename)
{
	int ret;
	int i;
	int oldifmode;
	int loaderid;

	hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);
	guicontext->loadedfloppy=0;

	ret=-1;

	loaderid=hxcfe_autoSelectLoader(guicontext->hxcfe,filename,0);
		
	if(loaderid>=0)
		guicontext->loadedfloppy=hxcfe_floppyLoad(guicontext->hxcfe,filename,loaderid,&ret);
		
	guicontext->loadstatus=ret;

	if(ret!=HXCFE_NOERROR || !guicontext->loadedfloppy)
	{
		guicontext->loadedfloppy=0;
		guicontext->bufferfilename[0]=0;
	}
	else
	{	
//		oldifmode=hwif->interface_mode;

		libusbhxcfe_setInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy->floppyiftype,guicontext->loadedfloppy->double_step,0);
		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
		if(!guicontext->autoselectmode)
		{	// keep the old interface mode
			//hwif->interface_mode=oldifmode;
		};
			
		if(filename)
		{
			i=strlen(filename);
			while(i!=0 && filename[i]!='\\')
			{
				i--;
			}
			if(filename[i]=='\\') i++;
			sprintf(guicontext->bufferfilename,"%s",&filename[i]);
		}
		else
		{
			sprintf(guicontext->bufferfilename,"Empty Floppy");
		}
	}

	return ret;
}

int loadrawfile(HXCFLOPPYEMULATOR* floppycontext,cfgrawfile * rfc,char * file)
{
	FBuilder* fb;
	FILE * f;
	unsigned int i,j,k,nbside;
	int ret;
	int oldifmode;
	unsigned char * trackbuffer;

	f=0;
	if(file)
		f=fopen(file,"r+b");

	if(f || file==NULL)
	{
		if(guicontext->loadedfloppy)
			hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);

		nbside=(rfc->sidecfg)&2?2:1;

		if(f)
		{
			trackbuffer=(unsigned char *)malloc((128<<rfc->sectorsize)*rfc->sectorpertrack);
			memset(trackbuffer,rfc->fillvalue,(128<<rfc->sectorsize)*rfc->sectorpertrack);
		}

		sprintf(guicontext->bufferfilename,"");
		guicontext->loadedfloppy=0;
		fb=hxcfe_initFloppy(floppycontext,rfc->numberoftrack,nbside);
		if(fb)
		{
			hxcfe_setTrackInterleave(fb,rfc->interleave);	
			hxcfe_setSectorFill(fb,rfc->fillvalue);

			if(rfc->autogap3)
				hxcfe_setSectorGap3(fb,255);
			else
				hxcfe_setSectorGap3(fb,(unsigned char)rfc->gap3);

			hxcfe_setTrackBitrate(fb,rfc->bitrate);

			for(i=0;i<rfc->numberoftrack;i++)
			{
				for(j=0;j<nbside;j++)
				{
					// prepare a new track.
					hxcfe_pushTrack(fb,rfc->rpm,i,j,track_type_list[rfc->tracktype].tracktype);
					
					// Set the skew
					if(rfc->sideskew)
					{
						hxcfe_setTrackSkew(fb,(unsigned char)(((i<<1)|(j&1))*rfc->skew) );
					}
					else
					{
						hxcfe_setTrackSkew(fb,(unsigned char)i*rfc->skew);
					}


					if(f)
					{
						fread(trackbuffer,(128<<rfc->sectorsize)*rfc->sectorpertrack,1,f);
					}

					// add all sectors
					for(k=0;k<rfc->sectorpertrack;k++)
					{
						if(f)
							hxcfe_addSector(fb,rfc->firstidsector+k,j,i,&trackbuffer[k*(128<<rfc->sectorsize)],128<<rfc->sectorsize);
						else
							hxcfe_addSector(fb,rfc->firstidsector+k,j,i,0,128<<rfc->sectorsize);
					}

					// generate the track
					hxcfe_popTrack(fb);
				}
			}

			guicontext->loadedfloppy=hxcfe_getFloppy(fb);
		}

		/*t=hxcfe_getfloppysize(floppycontext,thefloppydisk,0);

		test=malloc(512*18);

		ss=hxcfe_init_sectorsearch(floppycontext,thefloppydisk);
		t=hxcfe_readsectordata(ss,0,0,1,5,512,test);

		hxcfe_deinit_sectorsearch(ss);*/


		if(guicontext->loadedfloppy)
		{
			ret=HXCFE_NOERROR;

			guicontext->loadstatus=ret;

			//oldifmode=hwif->interface_mode;
			libusbhxcfe_setInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy->floppyiftype,guicontext->loadedfloppy->double_step,0);
			libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
			if(!guicontext->autoselectmode)
			{	// keep the old interface mode
				//hwif->interface_mode=oldifmode;
			};

			if(f)
				sprintf(guicontext->bufferfilename,basename(file));
			else
				sprintf(guicontext->bufferfilename,"Empty Floppy");
		}

		if(f)
		{
			free(trackbuffer);
			fclose(f);
		}
	}	


	return ret;
}
