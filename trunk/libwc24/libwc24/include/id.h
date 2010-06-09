//id.h
//Copyright (C) 2008 by raven
//This is Free Software released under the GNU/GPL License.

#ifndef _ID_H_
#define _ID_H_

s32 identify_SU();
s32 identify_menu();
s32 identify_title(char *title_hi, char *title_lo);
s32 launch_title(u64 titleID);
u64 build_id(char *title_hi, char *title_lo);

#endif
