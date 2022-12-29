/*
 *      STVN Engine
 *      (c) 2022 Toyoyo
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <sys/types.h>
#include <stdio.h>
#include <mint/osbind.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <vt52.h>
#include <osbind.h>
#include <fcntl.h>
#include <zlib.h>
#include <mint/linea.h>
// Line reading routine, better than I would have done anyway
#include "line.h"

// SNDH routines, including our gcc function call convention wrapper
#include "sndh.h"

// Things I kept from the original YM player
#define locate(x, y) printf("\033Y%c%c", (char)(32 + y), (char)(32 + x))
#define write_byte(value, address) (*address) = (__uint8_t)value
#define read_byte(address) (*address)

#define SaveMacro() ({\
  next=readKeyBoardStatus();\
  while(next!=10 && next!=11 && next!=12 && next!=13 && next!=2) {\
    next=readKeyBoardStatus();\
  }\
  if(next!=2) {\
    snprintf(savefile, 14, "data/sav%d.sav", next-9);\
    int fd=open(savefile, O_WRONLY | O_CREAT);\
    memcpy(videoram, background, 25600);\
    if(fd > 0) {\
      dprintf(fd, "%06d%d%d%d%d%d%d%d%d%d%d", savepointer,\
        choicedata[0],\
        choicedata[1],\
        choicedata[2],\
        choicedata[3],\
        choicedata[4],\
        choicedata[5],\
        choicedata[6],\
        choicedata[7],\
        choicedata[8],\
        choicedata[9]);\
      close(fd);\
    }\
  }\
})

//      write(fd, (char*)videoram, 25600);\

// I definitely use this too much to not actually define it properly
#define LoadBackground() ({\
  int pfd=open(picture, 0);\
  if(pfd > 0) {\
    gzFile gzf=gzdopen(pfd, "rb");\
    gzseek(gzf, 2, SEEK_CUR);\
    gzread(gzf, &bgpalette, 32);\
    gzread(gzf, background, 25600);\
    gzclose(gzf);\
    close(pfd);\
    memcpy(oldpicture, picture, 18);\
    memcpy(videoram, background, 25600);\
  }\
})

typedef struct {
  int x;
  int y;
  char file[18];
} sprite;

sprite currentsprites[256];
sprite previoussprites[256];
void backup_spritearray() {
  int i;
  for(i=0; i<255; i++) {
    previoussprites[i].x = currentsprites[i].x;
    previoussprites[i].y = currentsprites[i].y;
    if(strlen(currentsprites[i].file) > 0) {
      memcpy(previoussprites[i].file, currentsprites[i].file, 18);
    } else {
      memset(previoussprites[i].file, 0, 18);
    }
  }
}
void reset_cursprites() {
  int i;
  for(i=0; i<255; i++) {
    currentsprites[i].x = 0;
    currentsprites[i].y = 0;
    memset(currentsprites[i].file, 0, 18);
  }
}
void reset_prevsprites() {
  int i;
  for(i=0; i<255; i++) {
    previoussprites[i].x = 0;
    previoussprites[i].y = 0;
    memset(previoussprites[i].file, 0, 18);
  }
}
int compare_sprites() {
  int i;
  for(i=0; i<255; i++) {
    if(currentsprites[i].x != previoussprites[i].x) return 1;
    if(currentsprites[i].y != previoussprites[i].y) return 1;
    if(strncmp(currentsprites[i].file, previoussprites[i].file, 18) != 0) return 1;
  }
  return 0;
}

#define RedrawBorder() ({\
  X1=0; Y1=320; X2=640; Y2=320;\
  linea3();\
  X1=0; Y1=399; X2=640; Y2=399;\
  linea3();\
  X1=0; Y1=320; X2=0; Y2=399;\
  linea3();\
  X1=639; Y1=320; X2=639; Y2=399;\
  linea3();\
})

// non-blocking keyboard reading routine & handling
// Space -> read next script line
// 'q' -> jump to cleanup section & quit
// - Later -
// 'l' -> load state
// 's' -> save state
// Basically line bumber + 10 boolean registers
__uint32_t key;
int readKeyBoardStatus() {
  if (Cconis() != 0L) {
    key = Crawcin();
    if ((char)key == ' ') {
      return 1;
    }
    if ((char)key == 'q') {
      return 2;
    }
    if ((char)key == 's') {
      return 3;
    }
    if ((char)key == 'l') {
      return 4;
    }
    if ((char)key == 'b') {
      return 5;
    }
    if ((char)key == 'h') {
      return 6;
    }
    if ((char)key == '1') {
      return 10;
    }
    if ((char)key == '2') {
      return 11;
    }
    if ((char)key == '3') {
      return 12;
    }
    if ((char)key == '4') {
      return 13;
    }
  }
}

void DispDialogBorder() {
  X1=272; Y1=128; X2=392; Y2=128;
  linea3();
  X1=272; Y1=224; X2=392; Y2=224;
  linea3();
  X1=272; Y1=128; X2=272; Y2=224;
  linea3();
  X1=392; Y1=128; X2=392; Y2=224;
  linea3();
}

void DispLoading() {
  locate(35,8);
  printf("- Loading -");
  locate(35,9);
  if(access("data/sav1.sav", F_OK) == 0) {
    printf("1: USED    ");
  } else {
    printf("1: EMPTY   ");
  }
  locate(35,10);
  if(access("data/sav2.sav", F_OK) == 0) {
    printf("2: USED    ");
  } else {
    printf("2: EMPTY   ");
  }
  locate(35,11);
  if(access("data/sav3.sav", F_OK) == 0) {
    printf("3: USED    ");
  } else {
    printf("3: EMPTY   ");
  }
  locate(35,12);
  if(access("data/sav4.sav", F_OK) == 0) {
    printf("4: USED    ");
  } else {
    printf("4: EMPTY   ");
  }
  locate(35,13);
  printf("[q] : quit ");
  fflush(stdout);
  X1=280; Y1=128; X2=368; Y2=128;
  linea3();
  X1=280; Y1=224; X2=368; Y2=224;
  linea3();
  X1=280; Y1=128; X2=280; Y2=224;
  linea3();
  X1=368; Y1=128; X2=368; Y2=224;
  linea3();
}
void DispSaving() {
  locate(35,8);
  printf("- Saving -");
  locate(35,9);
  if(access("data/sav1.sav", F_OK) == 0) {
    printf("1: USED   ");
  } else {
    printf("1: EMPTY  ");
  }
  locate(35,10);
  if(access("data/sav2.sav", F_OK) == 0) {
    printf("2: USED   ");
  } else {
    printf("2: EMPTY  ");
  }
  locate(35,11);
  if(access("data/sav3.sav", F_OK) == 0) {
    printf("3: USED   ");
  } else {
    printf("3: EMPTY  ");
  }
  locate(35,12);
  if(access("data/sav4.sav", F_OK) == 0) {
    printf("4: USED   ");
  } else {
    printf("4: EMPTY  ");
  }
  locate(35,13);
  printf("[q] : quit");
  fflush(stdout);
  X1=280; Y1=128; X2=360; Y2=128;
  linea3();
  X1=280; Y1=224; X2=360; Y2=224;
  linea3();
  X1=280; Y1=128; X2=280; Y2=224;
  linea3();
  X1=360; Y1=128; X2=360; Y2=224;
  linea3();
}
void DispHelp() {
  locate(34,8);
  printf("-    Usage    -");
  locate(34,9);
  printf("[q] Quit       ");
  locate(34,10);
  printf("[b] Back       ");
  locate(34,11);
  printf("[l] Load save  ");
  locate(34,12);
  printf("[s] Save state ");
  locate(34,13);
  printf("[ ] Advance    ");
  fflush(stdout);
  PTSIN[0] = 272; PTSIN[1] = 128; PTSIN[0] = 392; PTSIN[1] = 128;
  X1=272; Y1=128; X2=392; Y2=128;
  linea3();
  X1=272; Y1=224; X2=392; Y2=224;
  linea3();
  X1=272; Y1=128; X2=272; Y2=224;
  linea3();
  X1=392; Y1=128; X2=392; Y2=224;
  linea3();
}

// Main function which will run in supervisor mode
void run() {
  char *background;
  char textarea[6400] = {0};
  char bgpalette[32] = {0};
  FILE *script;
  FILE *config;
  long lineNumber = 0;
  long prevLineNumber = 0;
  char* line;
  char picture[18] = {0};
  char oldpicture[18] = {0};
  char musicfile[18] = {0};
  char oldmusicfile[18] = {0};
  int charlines=0;
  void* tuneptr;
  gzFile sndfile;
  int isplaying=0;
  int willplaying=0;
  int next;
  char *choicedata;
  char jumplabel[5] = {0};
  char savefile[14] = {0};
  long savepointer = 0;
  long save_linenb = 0;
  int skipnextprev=0;
  int loadsave=0;

  // Sprite crap
  char spritefile[18] = {0};
  int posx=0;
  int posy=0;
  int isbackfunc=0;
  char linex[4] = {0};
  char liney[4] = {0};

  reset_cursprites();
  reset_prevsprites();

  // to track sprites during load/back
  int spritecount=0;

  // Defaults
  long sndhbuffersize=20000;
  char scriptfile[13] = "stvn.vns";

  // Get video RAM address
  char *videoram=Logbase();

  // Our choice registers
  choicedata=malloc(11);
  if(choicedata == NULL) return;
  memset(choicedata, 0, 11);

  // And screen buffers
  // The displayable area is 640x320, so 25600 bytes,
  background=malloc(25600);
  if(background == NULL) return;

  // Disable mouse
  lineaa();

  // Some VT52 setup
  printf(CLEAR_HOME);
  printf(WRAP_OFF);

  //Line-A Setup
  // - Initialize internal structure
  // - Write mode: Replace
  // - Draw last pixel of a line
  // - Bitplane 0 color = black
  linea0();
  WMODE=1;
  LSTLIN=1;
  COLBIT0=1;

  // Let's parse the config file
  if(access("stvn.ini", F_OK) == 0) {
    config=fopen("stvn.ini", "r");
    line = get_line(config);
    while(line) {
      if(strlen(line) > 0) {
        if(*line == 'S') {
          int filelength=strlen(line)-1;
          if(filelength > 12) filelength=12;
          memset(scriptfile, 0, 18);
          snprintf(scriptfile, 6, "DATA/");
          memcpy(scriptfile+5, line+1, filelength);
        }
        if(*line == 'B') {
          sndhbuffersize=atoi(line+1);
        }
      }
      line = get_line(config);
    }
    fclose(config);
  } else {
    printf("STVN.INI not found, using defaults:.\n");
    printf("Script file: STVN.VNS\n");
    printf("Maximum SNDH Size: 20000 bytes\n");
    printf("Press any key to continue...\n");
    fflush(stdout);
    Crawcin();
  }

  // Alloc some buffer space for the SNDH file
  // 20k seems like a good enough default, but definitely not enough for DMA ones.
  // This is the size of the biggest SNDH file in the project
  tuneptr = malloc(sndhbuffersize);

  // Disable Keyboard clicks
  __uint8_t originalKeyClick = read_byte((__uint8_t *)0x00000484);
  write_byte(0b11111110 & originalKeyClick, (__uint8_t *)0x484);

  // Our script
  script=fopen(scriptfile, "r");
  if(script == NULL) {
    printf("Opening %s failed.\n", scriptfile);
    printf("Press any key to quit...\n");
    fflush(stdout);
    Crawcin();
    free(tuneptr);
    linea9();
    return;
  }

  // main loop
  while (1) {
    // Read the next line
    // If there isn't, let's quit
    line = get_line(script);
    if(line == NULL) goto endprog;
    lineNumber = lineNumber + 1;

parseline:
    // Empty lines are simply ignored.
    // Feel free to make a readable script
    if(strlen(line) > 0) {
      // Our format is trivial.
      // First char is the command (T, P, etc..)
      // Then its arguments, starting at line+1
      // If the first character isn't a known operand, we'll do nothing.
      // So there's no "comment" operand, just use anything that's not a known operand.

      // 'W' : Wait for input.
      // This is where most interaction will happen.
      // Like saving/loading, waiting for 'space', quitting, etc...
      if(*line == 'W') {
        next=readKeyBoardStatus();
        while(next!=1) {
          // Our quit signal from 'q'
          if(next == 2) goto endprog;

          // Save
          if(next == 3) {
            memcpy(background, videoram, 25600);
            DispSaving();
            SaveMacro();
            memcpy(videoram, background, 25600);
          }

          if(next == 5) {
            save_linenb = prevLineNumber;
            goto seektoline;
          }

          if(next == 6) {
            memcpy(background, videoram, 25600);
            DispHelp();
            while(next!=2) {
                next=readKeyBoardStatus();
            }
            memcpy(videoram, background, 25600);
          }

          // Load
          if(next == 4) {
            loadsave=1;
            memcpy(background, videoram, 25600);
            DispLoading();
            while(next!=10 && next!=11 && next!=12 && next!=13 && next!=2) {
                next=readKeyBoardStatus();
            }
            memcpy(videoram, background, 25600);

            lblloadsave:
            // Effective loading.
            if(next!=2) {
              snprintf(savefile, 14, "data/sav%d.sav", next-9);
              if(access(savefile, F_OK) == 0) {
                int fd=open(savefile, O_RDONLY);
                char savestate[17] = {0};
                char save_line[7] = {0};
                char save_register[2] = {0};
                char save_register_val=0;
                int forceredraw=0;
                save_linenb = 0;
                read(fd, &savestate, 16);

                // First, get the target line number
                memcpy(save_line, savestate, 6);
                save_linenb=atoi(save_line);

                // Now we restore the choices register
                int i;
                for(i=0; i<10; i++) {
                  memcpy(save_register, savestate+6+i, 1);
                  save_register_val = atoi(save_register);
                  choicedata[i] = save_register_val;
                }

                seektoline:
                //Now, we're going to parse the entire file...
                rewind(script);
                lineNumber=0;
                savepointer=0;
                prevLineNumber=0;
                willplaying=0;
                spritecount=0;

                if(loadsave == 0) {
                  backup_spritearray();
                  reset_cursprites();
                }

                while(lineNumber < save_linenb) {
                  line = get_line(script);
                  if(line == NULL) goto endprog;
                  lineNumber = lineNumber + 1;

                  // So we'll get the last background picture to draw
                  // As this is supposed to refresh the screen, this invalidate previous sprites
                  if(*line == 'I') {
                    int filelen=strlen(line);
                    if(filelen > 12) filelen=12;
                    memset(picture, 0, 18);
                    snprintf(picture, 6, "DATA/");
                    memcpy(picture+5, line+1, filelen);
                    reset_cursprites();
                    spritecount=0;
                  }

                  // This redraw the background, so invalidates previous sprites too
                  if(*line == 'R') {
                    reset_cursprites();
                    spritecount=0;
                  }

                  // I didn't want to implement this
                  if(*line == 'A') {
                    if(strlen(line) < 8) goto endspriteload;
                    int filelen=strlen(line)-7;
                    if(filelen > 12) filelen=12;
                    memset(currentsprites[spritecount].file, 0, 18);
//                    snprintf(currentsprites[spritecount].file, 6, "DATA/");
                    memcpy(currentsprites[spritecount].file, line+7, filelen);
                    memset(linex, 0, 4);
                    memset(liney, 0, 4);
                    memcpy(linex, line+1, 3);
                    memcpy(liney, line+4, 3);
                    currentsprites[spritecount].x=atoi(linex);
                    currentsprites[spritecount].y=atoi(liney);
                    spritecount++;
                    endspriteload:
                    ;
                  }

                  // And the lastmusicfile
                  if(*line == 'P') {
                    int filelen=strlen(line);
                    if(filelen > 12) filelen=12;
                    memset(musicfile, 0, 18);
                    snprintf(musicfile, 6, "DATA/");
                    memcpy(musicfile+5, line+1, 12);
                    willplaying=1;
                  }
                  if(*line == 'S') {
                    prevLineNumber = savepointer;
                    savepointer=lineNumber;
                  }

                  // We also beed to replay the 'B' Lines
                  if(*line == 'B') {
                    if(strlen(line) == 6) {
                      char lineregister[2] = {0};
                      memcpy(lineregister, line+1, 1);
                      char selectedregister=atoi(lineregister);
                      if(choicedata[selectedregister] == 1) {
                        memcpy(jumplabel, line+2, 4);
                        while(1) {
                          line = get_line(script);
                          if(line == NULL) goto endprog;
                          lineNumber = lineNumber + 1;
                          if(strlen(line) >= 4) {
                            if(strncmp(jumplabel, line, 4) == 0) {
                              goto btestend;
                            }
                          }
                        }
                        btestend:
                        ;
                      }
                    }
                  }
                }

                // And probably 'J' Lines too.
                if(*line == 'J') {
                  if(strlen(line) >= 5) {
                    memcpy(jumplabel, line+1, 4);
                    while(1) {
                      line = get_line(script);
                      if(line == NULL) goto endprog;
                      lineNumber = lineNumber + 1;
                      if(strlen(line) >= 4) {
                        if(strncmp(jumplabel, line, 4) == 0) goto jend;
                      }
                    }
                  }
                  jend:
                  ;
                }

                // We should have everything we need, now let's restorethings

                // Let's explain things:
                // "background" is the last picture encoutered in the backlog
                // If we're loading a save, the're no way we don't neet to load it
                // If we're not, we're rolling back in-game.
                // In this case, reload on two cases:
                // * If the background changed, obviously
                // * If there's a difference in sprites to display
                if(loadsave == 0) {
                  if(strcmp(picture, oldpicture) != 0) {
                    LoadBackground();
                  } else {
                    if(compare_sprites() != 0) {
                      LoadBackground();
                    }
                  }
                } else {
                  LoadBackground();
                }

                charlines=0;

                // Then the music
                if(willplaying == 1) {
                  if(strncmp(musicfile, oldmusicfile, 12) != 0) {
                    if(isplaying == 1) {
                      SNDH_StopTune();
                      isplaying=0;
                    }

                    SNDHTune mytune;
                    memcpy(oldmusicfile, musicfile, 12);

                    sndfile=gzopen(musicfile,"rb");
                    if(sndfile != NULL) {
                      gzread(sndfile, tuneptr, sndhbuffersize);
                      gzclose(sndfile);
                      SNDH_GetTuneInfo(tuneptr,&mytune);
                      SNDH_PlayTune(&mytune,0);
                      isplaying=1;
                    } else {
                      isplaying=0;
                    }
                  }
                }
                close(fd);

                //Now to display required sprites;
                if(compare_sprites() != 0 || loadsave == 1) {
                  int spritecounter=1;
                  if(spritecount > 0) {
                    for(spritecounter=1; spritecounter <= spritecount; spritecounter++) {
                      snprintf(spritefile, 6, "DATA/");
                      memcpy(spritefile+5, currentsprites[spritecounter-1].file, 12);
                      posx=currentsprites[spritecounter-1].x;
                      posy=currentsprites[spritecounter-1].y;
                      isbackfunc=1;

                      goto displaysprite;
                      spritedrawn:
                      ;
                    }
                  }
                }
                // We're done, and the next action will be to display to current line.
                // Which sould be a 'S' since they're the only ones advancing the save pointer.
                loadsave=0;
                skipnextprev=1;
                break;
              }
            }
            // In case we didn't load anything, restore the scene
            memcpy(videoram, background, 25600);
          }
          next=readKeyBoardStatus();
        }
      }

      // 'I' : Change background image
      if(*line == 'I') {
        int filelen=strlen(line)-1;
        if(filelen > 12) filelen=12;
        memset(picture, 0, 18);
        snprintf(picture, 6, "DATA/");
        memcpy(picture+5, line+1, filelen);

        // In case the 'new' picture is the previous one
        // Don't waste cycles for nothing
        if(strncmp(picture, oldpicture, 18) != 0) {
          LoadBackground();
        }
        reset_cursprites();
        spritecount=0;
        charlines=0;
      }

      if(*line == 'R') {
        memcpy(videoram, background, 25600);
        reset_cursprites();
        spritecount=0;
      }
      // 'S' : Sayer change
      // Obviously, this will reset character lines to 0
      // This is used as the save pointer and will also reset the text box
      if(*line == 'S') {
        charlines=0;
        memcpy(videoram+25600, textarea, 6400);
        RedrawBorder();

        // Display the character name at a fixed place
        locate(0,20);
        printf("%s", line+1);
        fflush(stdout);
        if(skipnextprev == 1) {
          skipnextprev=0;
        } else {
          prevLineNumber=savepointer;
        }
        savepointer=lineNumber;
      }

      //'T' : Text line
      // We disabled line wrapping so make sure a line isn't more then 79 characters!
      if(*line == 'T') {
        // Skip already written lines
        // charlines being reset to 0 after each 'S' lines
        locate(0,21+charlines);
        printf(" %s", line+1);
        fflush(stdout);
        charlines++;
      }

      // 'P' : Change music
      // The SNDH routine MUST be stopped ONLY if there's already one playing.
      if(*line == 'P') {
        int filelen=strlen(line);
        if(filelen > 12) filelen=12;
        memset(musicfile, 0, 18);
        snprintf(musicfile, 6, "DATA/");
        memcpy(musicfile+5, line+1, filelen);
        if(strncmp(musicfile, oldmusicfile, 12) != 0) {
          SNDHTune mytune;
          memcpy(oldmusicfile, musicfile, 12);

          if(isplaying == 1) SNDH_StopTune();
          isplaying=0;
          sndfile=gzopen(musicfile,"rb");

          if(sndfile != NULL) {
            gzread(sndfile, tuneptr, sndhbuffersize);
            gzclose(sndfile);
            SNDH_GetTuneInfo(tuneptr,&mytune);
            SNDH_PlayTune(&mytune,0);
            isplaying=1;
          } else {
            isplaying=0;
          }
        }
      }

      // 'J' : Jump to label
      // Mostly useful for 'B'+'J' combos, but 'B' and 'C' lines will jump here.
      if(*line == 'J') {
        if(strlen(line) >= 5) {
          memcpy(jumplabel, line+1, 4);
          jumptolabel:
          while(1) {
            line = get_line(script);
            if(line == NULL) goto endprog;
            lineNumber = lineNumber + 1;
            if(strlen(line) >= 4) {
              if(strncmp(jumplabel, line, 4) == 0) {
                break;
              }
            }
          }
        }
      }

      // 'B' : Jump to label if register is set
      if(*line == 'B') {
        if(strlen(line) == 6) {
          char lineregister[2] = {0};
          memcpy(lineregister, line+1, 1);
          char selectedregister=atoi(lineregister);
          if(choicedata[selectedregister] == 1) {
            memcpy(jumplabel, line+2, 4);
            goto jumptolabel;
          }
        }
      }

      // 'C': Offer a choice and store it in a register
      // We also want the same kind of interaction we have with 'W' lines
      // Users *will* want to save before making a choice
      if(*line == 'C') {
        if(strlen(line) == 2) {
          char lineregister[2] = {0};
          memcpy(lineregister, line+1, 1);
          char selectedregister=atoi(lineregister);

          next=readKeyBoardStatus();
          while(next!=10 && next!=11) {
            next=readKeyBoardStatus();
            if(next == 2) goto endprog;
            if(next == 3) {
              memcpy(background, videoram, 25600);
              DispSaving();
              SaveMacro();
              next=0;
              memcpy(videoram, background, 25600);
            }
            if(next == 4) {
              memcpy(background, videoram, 25600);
              DispLoading();
              while(next!=10 && next!=11 && next!=12 && next!=13 && next!=2) {
                next=readKeyBoardStatus();
              }
              snprintf(savefile, 14, "data/sav%d.sav", next-9);
              if(access(savefile, F_OK) == 0) goto lblloadsave;
              next=0;
              memcpy(videoram, background, 25600);
            }

            if(next == 5) {
                save_linenb = prevLineNumber;
                goto seektoline;
            }

            if(next == 6) {
              memcpy(background, videoram, 25600);
              DispHelp();
              while(next!=2) {
                  next=readKeyBoardStatus();
              }
              memcpy(videoram, background, 25600);
            }

          }
          choicedata[selectedregister] = next - 10;
        }
      }

      if(*line == 'D') {
        if(strlen(line+1) < 6) {
          usleep(atoi(line+1)*1000);
        }
      }

      if(*line == 'A') {
        posx=0;
        posy=0;
        memset(linex, 0, 4);
        memset(liney, 0, 4);
        gzFile sprite;

        // Minimum A + xxx + yyy + filename
        if(strlen(line) < 8) goto endsprite;

        int filelen=strlen(line)-7;
        if(filelen > 12) filelen=12;
        memset(spritefile, 0, 18);
        snprintf(spritefile, 6, "DATA/");
        memcpy(spritefile+5, line+7, filelen);
        memcpy(linex, line+1, 3);
        memcpy(liney, line+4, 3);
        posx=atoi(linex);
        posy=atoi(liney);
        currentsprites[spritecount].x=atoi(linex);
        currentsprites[spritecount].y=atoi(liney);
        memcpy(currentsprites[spritecount].file, line+7, filelen);
        spritecount++;

        displaysprite:
        sprite=gzopen(spritefile,"rb");
        if(sprite != NULL) {
          char* spritedata;
          char spritebuf[1];
          int x=0;
          int y=0;
          int ppos;
          int pctsize=0;
          int pctread=0;
          int pctpos=0;
          char *tmp;
          char* pctmem;
          pctmem=malloc(1024);
          if(pctmem == NULL) goto endsprite;


          // Read at most bytes, if more needed, allocate 1024 more.
          // A full-screen sprite would use  640*400 = 256000 bytes, I don't want to use that much.
          // And the format allows for more.
          pctread=gzread(sprite, pctmem, 1024);
          pctsize=pctsize+pctread;
          while(pctread != 0) {
            tmp=realloc(pctmem, 1024+pctsize);
            if(tmp == NULL) {
              gzclose(sprite);
              goto abortdraw;
            }
            pctmem=tmp;
            pctread=gzread(sprite, pctmem+pctsize, 1024);
            pctsize=pctsize+pctread;
          }
          gzclose(sprite);

          // Now for the grawing routine
          for(pctpos=0; pctpos < pctsize; pctpos++) {

            //Horizontal line change!
            if(pctmem[pctpos] == 10) {
              if(posy+y <= 400) {
                y++;
                x=0;
              } else {
                goto abortdraw;
              }
            }

            // Transparency, don't draw anything
            if(pctmem[pctpos] == ' ') {
              if(x+posx < 639) x++;
            }

           // Actual writing.
           // Thanks, grad'
           if(pctmem[pctpos] == '0' || pctmem[pctpos] == '1') {
              if(x+posx < 639 && posy+y <= 400) {
                ppos = (y + posy) * 640 + x + posx;
                if(pctmem[pctpos] == '1') videoram[ppos / 8] |= 1 << 7 - (ppos % 8);
                if(pctmem[pctpos] == '0') videoram[ppos / 8] &= ~(1 << 7 - (ppos % 8));
                x++;
              };
            }
          }
          abortdraw:
          free(pctmem);
        }
        endsprite:
        if(isbackfunc==1) {
          isbackfunc=0;
          goto spritedrawn;
        }
        ;
      }
    }

    // Redraw our text-area border
    RedrawBorder();
    // Just wait a bit... until the next vertical sync.
    Vsync();
  }

  endprog:
  fclose(script);

  // Maybe there wasn't any sound ever? :'(
  if(isplaying == 1) SNDH_StopTune();

  // Restore our beloved(?) keyboard clicks
  write_byte(originalKeyClick, (__uint8_t *)0x484);

  // I Hope I didn't forget anything. This isn't win3 though.
  free(line);
  free(tuneptr);
  free(choicedata);
  free(background);

  // Finally, restore the mouse cursor
  linea9();
}

int main(int argc, char *argv[]) {
  // Disable console cursor
  Cursconf(0,0);

  // Execute our program in supervisor mode
  Supexec(&run);
}
