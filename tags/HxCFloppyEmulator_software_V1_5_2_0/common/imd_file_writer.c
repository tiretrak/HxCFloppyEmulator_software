/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-Fran�ois DEL NERO
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
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "version.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "imd_file_writer.h"
#include "plugins/common/crc.h"
#include "plugins/imd_loader/imd_format.h"

#include "sector_extractor.h"



extern unsigned char  size_to_code(unsigned long size);
/*{

	switch(size)
	{
		case 128:
			return 0;
		break;
		case 256:
			return 1;
		break;
		case 512:
			return 2;
		break;
		case 1024:
			return 3;
		break;
		case 2048:
			return 4;
		break;
		case 4096:
			return 5;
		break;
		case 8192:
			return 6;
		break;
		case 16384:
			return 7;
		break;
		default:
			return 0;
		break;
	}
}*/

int write_IMD_file(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{	
	int i,j,k,l,nbsector;
	FILE * imdfile;
	char * log_str;
	char   tmp_str[256];
	char rec_mode;

	unsigned char sector_numbering_map[256];
	unsigned char cylinder_numbering_map[256];
	unsigned char side_numbering_map[256];

	int track_cnt;
	int sectorlistoffset,trackinfooffset;
	sect_track track;
	imd_trackheader imd_th;

	struct tm * ts;
	time_t currenttime;

//	struct DateTime reptime;


	floppycontext->hxc_printf(MSG_INFO_1,"Write IMD file %s...",filename);

	log_str=0;
	imdfile=fopen(filename,"wb");
	if(imdfile)
	{

		currenttime=time (NULL);
		ts=localtime(&currenttime);

		fprintf(imdfile,"IMD 1.17: %.2d/%.2d/%.4d %.2d:%.2d:%.2d\r\n",ts->tm_mday,ts->tm_mon,ts->tm_year+1900,ts->tm_hour,ts->tm_min,ts->tm_sec);
		fprintf(imdfile,"File generated by the HxC Floppy Emulator software v%s\r\n",STR_FILE_VERSION2);
		fprintf(imdfile,"%c",0x1A);

		memset(sector_numbering_map,0,0x100);
		memset(cylinder_numbering_map,0,0x100);
		memset(side_numbering_map,0,0x100);

		track_cnt=0;

		for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
		{
			for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
			{
				sprintf(tmp_str,"track:%.2d:%d file offset:0x%.6x, sectors: ",j,i,ftell(imdfile));

				log_str=0;
				log_str=realloc(log_str,strlen(tmp_str)+1);
				memset(log_str,0,strlen(tmp_str)+1);
				strcat(log_str,tmp_str);

				rec_mode=0;
				memset(&track,0,sizeof(sect_track));
				track.side=i;
				track.track=j;
				rec_mode=2;
				nbsector=analysis_and_extract_sector_MFM(floppycontext,floppy->tracks[j]->sides[i],&track);
				if(!nbsector)
				{
					nbsector=analysis_and_extract_sector_FM(floppycontext,floppy->tracks[j]->sides[i],&track);
					rec_mode=1;
					if(!nbsector)
					{
						rec_mode=0;
						nbsector=analysis_and_extract_sector_AMIGAMFM(floppycontext,floppy->tracks[j]->sides[i],&track);
					}
				}


				memset(&imd_th,0,sizeof(imd_trackheader));

				imd_th.physical_head=i;


				l=0;
				while((l<track.number_of_sector) && track.sectorlist[l]->side_id==track.side)
				{
					l++;
				}
				if(l!=track.number_of_sector)
				{
					imd_th.physical_head=imd_th.physical_head | 0x40;
				}
					

				l=0;
				while((l<track.number_of_sector) && track.sectorlist[l]->track_id==track.track)
				{
					l++;
				}
				if(l!=track.number_of_sector)
				{
					imd_th.physical_head=imd_th.physical_head | 0x80;
				}
					
				imd_th.physical_cylinder=j;
				imd_th.number_of_sector=track.number_of_sector;
				
				imd_th.track_mode_code=rec_mode;

				switch(floppy->tracks[j]->sides[i]->bitrate)
				{
					case 250000:
						imd_th.track_mode_code=2;
						break;
					case 300000:
						imd_th.track_mode_code=1;
						break;
					case 500000:
						imd_th.track_mode_code=0;
						break;
					default:
						imd_th.track_mode_code=2;
						break;

				}

				if(rec_mode==2)
				{
					imd_th.track_mode_code=imd_th.track_mode_code+3;
				}

				if(track.number_of_sector)
				{
					imd_th.sector_size_code=size_to_code(track.sectorlist[0]->sectorsize);
				}

				trackinfooffset=ftell(imdfile);
				fwrite(&imd_th,sizeof(imd_trackheader),1,imdfile);
				sectorlistoffset=ftell(imdfile);

				for(k=0;k<track.number_of_sector;k++)
				{
					sector_numbering_map[k]=track.sectorlist[k]->sector_id;
					cylinder_numbering_map[k]=track.sectorlist[k]->track_id;
					side_numbering_map[k]=track.sectorlist[k]->side_id;
				}

				fwrite(sector_numbering_map,imd_th.number_of_sector,1,imdfile);
				if(imd_th.physical_head & 0x80)fwrite(cylinder_numbering_map,imd_th.number_of_sector,1,imdfile);
				if(imd_th.physical_head & 0x40)fwrite(side_numbering_map,imd_th.number_of_sector,1,imdfile);

				if(track.number_of_sector)
				{
			
					k=0;
					do
					{


						l=0;
						while((l<track.sectorlist[k]->sectorsize) && track.sectorlist[k]->buffer[l]==track.sectorlist[k]->buffer[0])
						{
							l++;
						}

						if(l!=track.sectorlist[k]->sectorsize)
						{
							fputs("\1",imdfile);
							fwrite(track.sectorlist[k]->buffer,track.sectorlist[k]->sectorsize,1,imdfile);
						}
						else
						{
							fputs("\2",imdfile);
							fwrite(track.sectorlist[k]->buffer,1,1,imdfile);
						}

						sprintf(tmp_str,"%d ",track.sectorlist[k]->sector_id);
						log_str=realloc(log_str,strlen(log_str)+strlen(tmp_str)+1);
						strcat(log_str,tmp_str);
						k++;
			
					}while(k<track.number_of_sector);
	

					k=0;
					do
					{
						free(track.sectorlist[k]->buffer);
						free(track.sectorlist[k]);
						k++;
					}while(k<track.number_of_sector);

			
					log_str=realloc(log_str,strlen(log_str)+strlen(tmp_str)+1);
					strcat(log_str,tmp_str);

				}

				track_cnt++;

				floppycontext->hxc_printf(MSG_INFO_1,log_str);
				free(log_str);
			
			}
		}	


		fclose(imdfile);
	}
	
	return 0;
}