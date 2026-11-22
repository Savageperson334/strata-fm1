/* SPDX-License-Identifier: MPL-2.0 */
/* FM-1 panel for the independent PALIMPSEST instrument. */
#include "palimpsest.h"
#include "fastmath.h"
#include <string.h>
static int mode, octave, recording, running, slots[PM_VOICES], replace_at;
static uint32_t old_keys, old_btn, frames;
static float gain_now;
static float audio_l[APP_BLOCK],audio_r[APP_BLOCK];
static char message[24];
static uint32_t message_until;
static const int enc[7]={EN_K1,EN_K2,EN_K3,EN_K4,EN_SELECT,EN_ALGO,EN_PRESET};
static const char *const labels[PM_PARAMS]={
 "X","Y","COMPLEX","TONE","SPREAD","MOVE","EXCITE",
 "POSITION","SIZE","DENSITY","SPRAY","PITCH","REVERSE","FEEDBACK",
 "MIX","FADE","SCAN","WINDOW","STEREO","DAMP","LEVEL"};
static const char *const rec_names[]={"LIVE","FREEZE","OVERDUB","RESAMPLE"};
static const char *const knobs[]={"K1","K2","K3","K4","SELECT","ALGO","PRESET"};
static void number(int v,char *s){
    char t[12];int n=0;unsigned u;
    if(v<0){*s++='-';u=(unsigned)(-v);}else u=(unsigned)v;
    do{t[n++]=(char)('0'+u%10u);u/=10u;}while(u);
    while(n)*s++=t[--n];*s=0;
}
void app_say(const char *a,const char *b){
    uint32_t i=0;for(;a&&a[i]&&i<23;i++)message[i]=a[i];
    if(b&&*b&&i<22){message[i++]=' ';while(*b&&i<23)message[i++]=*b++;}
    message[i]=0;message_until=plat_ms()+1200u;
}
static int voices(void){int n=0;for(int j=0;j<PM_VOICES;j++)n+=slots[j]>=0;return n;}
static void all_off(void){for(int j=0;j<PM_VOICES;j++){slots[j]=-1;pm_note(j,-1);}}
static void latch(int note){
    int empty=-1;
    for(int j=0;j<PM_VOICES;j++){
        if(slots[j]==note){slots[j]=-1;pm_note(j,-1);return;}
        if(slots[j]<0&&empty<0)empty=j;
    }
    if(empty<0)empty=(replace_at++)%PM_VOICES;
    slots[empty]=note;pm_note(empty,note);
}
void app_init(void){
    pm_init();mode=0;octave=-1;recording=PM_LIVE;running=0;replace_at=0;
    old_keys=old_btn=frames=message_until=0;gain_now=0;
    all_off();
}
void app_panic(void){pm_panic();all_off();running=0;}
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12){
    /* Audio frame sizes are bounded by APP_BLOCK, but handle arbitrary n defensively. */
    float gain=(float)master_q12*(8300000.f/4096.f);
    while(n){
        uint32_t k=n>APP_BLOCK?APP_BLOCK:n;
        pm_render(audio_l,audio_r,k);
        for(uint32_t i=0;i<k;i++){
            gain_now+=(gain-gain_now)*.004f;
            *lr++=(int32_t)(audio_l[i]*gain_now);*lr++=(int32_t)(audio_r[i]*gain_now);
        }
        n-=k;
    }
}
void app_input_only(void){
    uint32_t keys=plat_keys(),btn=plat_buttons(),newb=btn&~old_btn,newk=keys&~old_keys;
    if(newb&(1u<<B_FX)){mode=0;app_say("MORPH","");}
    if(newb&(1u<<B_EDIT)){mode=1;app_say("SILT","");}
    if(newb&(1u<<B_REC)){recording=(recording+1)&3;pm_record(recording);app_say(rec_names[recording],"");}
    if(newb&(1u<<B_PLAY)){running=!running;pm_grains(running);app_say("GRAINS",running?"ON":"OFF");}
    if(newb&(1u<<B_HOME)){all_off();running=0;pm_grains(0);app_say("VOICES","OFF");}
    if(newb&(1u<<B_SAVE))app_say("RAM ONLY","0.1");
    if((newb&(1u<<B_OCTDN))&&octave>-3)octave--;
    if((newb&(1u<<B_OCTUP))&&octave<2)octave++;
    for(int k=0;k<NKEYS;k++)if(newk&(1u<<k)){
        if(!mode)latch(53+12*octave+k);
        else{
            pm_set(S_POS,(float)k/(NKEYS-1));
            pm_burst();
        }
    }
    int page=(btn&(1u<<B_SEL))?14:mode*7;
    for(int j=0;j<7;j++){
        int d=plat_enc(enc[j]);if(!d)continue;
        float v=pm_get(page+j)+(float)d/64.f;
        pm_set(page+j,fm_clampf(v,0,1));
    }
    old_keys=keys;old_btn=btn;
    uint32_t lit=(1u<<(mode?B_EDIT:B_FX))|(running?LED_PLAY_GREEN:0)|
        (recording!=PM_LIVE?(1u<<B_REC):0)|(btn&(1u<<B_SEL));
    uint32_t kl=0;
    if(!mode)for(int j=0;j<PM_VOICES;j++){
        int k=slots[j]-(53+12*octave);
        if(k>=0&&k<NKEYS)kl|=1u<<k;
    }
    else kl=1u<<(int)(pm_get(S_POS)*(NKEYS-1)+.5f);
    plat_lights(lit,kl,(1u<<B_FX)|(1u<<B_EDIT)|(1u<<B_REC)|(1u<<B_PLAY)|(1u<<B_HOME)|(1u<<B_SEL),0,1);
}
static void draw(void){
    char t[12];float lo[64],hi[64];
    cv_begin(240,120,C_BLACK);
    cv_text(4,3,&FONT_B,mode?"SILT":"MORPH",C_HI);
    cv_text(142,8,&FONT_XS,rec_names[recording],C_AMB);
    if(message_until&&(int32_t)(message_until-plat_ms())>0)
        cv_text(4,25,&FONT_XS,message,C_WHITE);
    else cv_text(4,25,&FONT_XS,"PALIMPSEST  0.1",C_GRAY);
    if(!mode){
        int x=24+(int)(pm_morph_x()*192.f),y=102-(int)(pm_morph_y()*58.f);
        cv_rect(24,44,192,1,C_LINE);cv_rect(24,102,192,1,C_LINE);
        cv_rect(24,44,1,58,C_LINE);cv_rect(215,44,1,58,C_LINE);
        cv_line(24,73,215,73,C_LINE);cv_line(120,44,120,102,C_LINE);
        cv_text(28,45,&FONT_XS,"FM",C_GRAY);cv_text(169,45,&FONT_XS,"MODAL",C_GRAY);
        cv_text(28,87,&FONT_XS,"ADD",C_GRAY);cv_text(170,87,&FONT_XS,"FOLD",C_GRAY);
        cv_rect(x-3,y-3,7,7,C_WHITE);
        number(voices(),t);cv_text(4,107,&FONT_XS,t,C_HI);cv_text(20,107,&FONT_XS,"VOICES",C_GRAY);
    }else{
        pm_memory(lo,hi,64);
        cv_line(4,73,235,73,C_LINE);
        for(int j=0;j<64;j++){
            int x=4+j*231/63;
            cv_line(x,73-(int)(hi[j]*30),x,73-(int)(lo[j]*30),C_HI);
        }
        int x=4+(int)(pm_get(S_POS)*231);
        cv_rect(x,43,1,55,C_WHITE);
        cv_rect(4,100,232,3,C_LINE);cv_rect(4,100,(int)(232*pm_fill()),3,C_AMB);
        number(pm_active_grains(),t);cv_text(4,107,&FONT_XS,t,C_HI);
        cv_text(24,107,&FONT_XS,"GRAINS",C_GRAY);
        cv_text(151,107,&FONT_XS,running?"RUNNING":"STOPPED",C_GRAY);
    }
    cv_blit(0,0);
    cv_begin(240,120,C_BLACK);
    int page=(old_btn&(1u<<B_SEL))?14:mode*7;
    for(int j=0;j<7;j++){
        int x=(j&3)*60,y=(j/4)*60;float v=pm_get(page+j);
        cv_text(x+3,y+4,&FONT_XS,labels[page+j],C_GRAY);
        number((int)(v*100+.5f),t);cv_text(x+3,y+20,&FONT_B,t,C_HI);
        cv_text(x+3,y+40,&FONT_XS,knobs[j],C_DIM);
        cv_rect(x+3,y+53,54,3,C_LINE);cv_rect(x+3,y+53,(int)(v*54),3,C_AMB);
        if(j&3)cv_rect(x,y+3,1,54,C_LINE);
    }
    cv_text(183,65,&FONT_XS,"FX/EDIT",C_GRAY);
    cv_text(183,81,&FONT_XS,"SEL MORE",C_GRAY);
    cv_text(183,97,&FONT_XS,"REC MODE",C_GRAY);
    cv_blit(0,120);
}
void app_ui_init(void){lcd_fill(0,0,240,240,C_BLACK);draw();}
void app_frame(void){app_input_only();if((++frames&3u)==0)draw();}
int app_expect(const char *what,int *got){
    if(!strcmp(what,"mode"))*got=mode;
    else if(!strcmp(what,"voices"))*got=voices();
    else if(!strcmp(what,"recording"))*got=recording;
    else if(!strcmp(what,"grains"))*got=running;
    else if(!strcmp(what,"filled"))*got=(int)(pm_fill()*100);
    else return 0;
    return 1;
}
