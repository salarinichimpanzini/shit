#include <ctype.h>
#include "soft_keyboard.h"

namespace {
constexpr uint32_t BG = 0x05080A;
constexpr uint32_t KEY = 0x111B21;
constexpr uint32_t KEY_ALT = 0x15242B;
constexpr uint32_t PRESSED = 0x24414B;
constexpr uint32_t LINE = 0x25414B;
constexpr uint32_t CYAN = 0x52E4FF;
constexpr uint32_t WHITE = 0xF2F5F6;
constexpr uint32_t GREEN = 0x54F28C;
}

SoftKeyboard softKeyboard;

void SoftKeyboard::open(lv_obj_t *parent, lv_obj_t *textarea, Mode mode) {
    if (!parent || !textarea) return;
    close();
    target_ = textarea;
    mode_ = mode;
    upper_ = false;

    const int sw = lv_obj_get_width(parent);
    const int sh = lv_obj_get_height(parent);
    const int pw = min(sw - 8, 312);
    const int ph = min(sh - 60, 246);
    panel_ = lv_obj_create(parent);
    lv_obj_remove_style_all(panel_);
    lv_obj_set_size(panel_, pw, ph);
    lv_obj_set_pos(panel_, (sw-pw)/2, sh-ph);
    lv_obj_set_style_bg_color(panel_, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(panel_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel_, lv_color_hex(LINE), 0);
    lv_obj_set_style_border_width(panel_, 1, 0);
    lv_obj_set_style_pad_all(panel_, 5, 0);
    lv_obj_set_style_pad_row(panel_, 4, 0);
    lv_obj_set_style_pad_column(panel_, 2, 0);
    lv_obj_clear_flag(panel_, LV_OBJ_FLAG_SCROLLABLE);

    title_ = lv_label_create(panel_);
    lv_obj_set_width(title_, pw-10);
    lv_label_set_text(title_, mode_==ALPHA?"INPUT  /  ABC":mode_==NUMERIC?"INPUT  /  123":"INPUT  /  SYMBOLS");
    lv_obj_set_style_text_color(title_, lv_color_hex(CYAN), 0);
    lv_obj_set_pos(title_, 5, 3);
    lv_obj_set_height(title_, 18);

    rebuild();
    if (visibilityCallback_) visibilityCallback_(true);
}

void SoftKeyboard::close() {
    if (panel_) { lv_obj_del(panel_); panel_=nullptr; title_=nullptr; }
    target_=nullptr;
    if (visibilityCallback_) visibilityCallback_(false);
}

void SoftKeyboard::addKey(const char *label,int x,int y,int w,int h,uint8_t action){
    if(!panel_)return;
    lv_obj_t*b=lv_btn_create(panel_);lv_obj_set_size(b,w,h);lv_obj_set_pos(b,x,y);
    lv_obj_remove_style_all(b);
    lv_obj_set_style_bg_color(b,lv_color_hex(KEY),0);
    lv_obj_set_style_bg_color(b,lv_color_hex(PRESSED),LV_STATE_PRESSED);
    lv_obj_set_style_border_color(b,lv_color_hex(LINE),0);lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_radius(b,3,0);
    lv_obj_set_style_text_color(b,lv_color_hex(WHITE),0);
    lv_obj_t*l=lv_label_create(b);lv_label_set_text(l,label);lv_obj_center(l);
    lv_obj_add_event_cb(b,keyEvent,LV_EVENT_CLICKED,reinterpret_cast<void *>(static_cast<intptr_t>(action)));
}

void SoftKeyboard::rebuild(){
    if(!panel_)return;
    while(lv_obj_get_child_cnt(panel_)>1)lv_obj_del(lv_obj_get_child(panel_,1));
    const int keyH=35;
    const int unit=30;
    const int y1=28,y2=67,y3=106,y4=145,y5=184;
    if(mode_==ALPHA){
        const char*r1="qwertyuiop";const char*r2="asdfghjkl";const char*r3="zxcvbnm";
        for(int i=0;i<10;++i){char k[2]={upper_?(char)toupper(r1[i]):r1[i],0};addKey(k,4+i*unit,y1,28,keyH);}
        for(int i=0;i<9;++i){char k[2]={upper_?(char)toupper(r2[i]):r2[i],0};addKey(k,19+i*unit,y2,28,keyH);}
        for(int i=0;i<7;++i){char k[2]={upper_?(char)toupper(r3[i]):r3[i],0};addKey(k,4+i*unit,y3,28,keyH);}
        addKey("BS",216,y3,42,keyH);
        addKey("123",260,y3,43,keyH,2);
        addKey(upper_?"abc":"SHIFT",4,y4,64,keyH,3);
        addKey("SPACE",70,y4,112,keyH,4);
        addKey("SYM",184,y4,50,keyH,5);
        addKey("OK",236,y4,50,keyH,6);
        addKey("@",4,y5,50,keyH);addKey(".",56,y5,50,keyH);addKey("/",108,y5,50,keyH);addKey("-",160,y5,50,keyH);addKey("_",212,y5,50,keyH);addKey(":",264,y5,39,keyH);
    } else if(mode_==NUMERIC){
        const char*r1="1234567890";const char*r2="-/:;,.!?+";const char*r3="()[]{}<>";
        for(int i=0;i<10;++i){char k[2]={r1[i],0};addKey(k,4+i*unit,y1,28,keyH);}
        for(int i=0;i<9;++i){char k[2]={r2[i],0};addKey(k,19+i*unit,y2,28,keyH);}
        for(int i=0;i<8;++i){char k[2]={r3[i],0};addKey(k,4+i*unit,y3,28,keyH);}
        addKey("BS",246,y3,57,keyH);
        addKey("ABC",4,y4,62,keyH,1);addKey("SPACE",68,y4,112,keyH,4);addKey("SYM",182,y4,53,keyH,5);addKey("OK",237,y4,50,keyH,6);
        addKey("%",4,y5,50,keyH);addKey("=",56,y5,50,keyH);addKey("*",108,y5,50,keyH);addKey("#",160,y5,50,keyH);addKey("@",212,y5,50,keyH);addKey("$",264,y5,39,keyH);
    } else {
        const char*r1="@#$%^&*=";const char*r2="-+_|\\/:;";const char*r3="\"'`~[]{}";
        for(int i=0;i<8;++i){char k[2]={r1[i],0};addKey(k,4+i*unit,y1,28,keyH);}
        for(int i=0;i<8;++i){char k[2]={r2[i],0};addKey(k,4+i*unit,y2,28,keyH);}
        for(int i=0;i<8;++i){char k[2]={r3[i],0};addKey(k,4+i*unit,y3,28,keyH);}
        addKey("BS",246,y3,57,keyH);
        addKey("ABC",4,y4,62,keyH,1);addKey("SPACE",68,y4,112,keyH,4);addKey("123",182,y4,53,keyH,2);addKey("OK",237,y4,50,keyH,6);
        addKey("<",4,y5,50,keyH);addKey(">",56,y5,50,keyH);addKey("?",108,y5,50,keyH);addKey("!",160,y5,50,keyH);addKey("&",212,y5,50,keyH);addKey(";",264,y5,39,keyH);
    }
}

void SoftKeyboard::handleKey(const String &key){
    if(!target_)return;
    if(key=="BS"){lv_textarea_del_char(target_);return;}
    if(key=="OK"){close();return;}
    if(key=="SPACE"){lv_textarea_add_text(target_," ");return;}
    lv_textarea_add_text(target_,key.c_str());
}

void SoftKeyboard::keyEvent(lv_event_t*e){
    auto*self=&softKeyboard;lv_obj_t*btn=lv_event_get_target(e);const intptr_t action=reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
    if(action==1){self->mode_=ALPHA;self->rebuild();return;}if(action==2){self->mode_=self->mode_==ALPHA?NUMERIC:self->mode_==NUMERIC?ALPHA:NUMERIC;self->rebuild();return;}if(action==3){self->upper_=!self->upper_;self->rebuild();return;}if(action==4){self->handleKey("SPACE");return;}if(action==5){self->mode_=self->mode_==SYMBOLS?ALPHA:SYMBOLS;self->rebuild();return;}if(action==6){self->handleKey("OK");return;}
    if(lv_obj_get_child_cnt(btn)>0){const char*txt=lv_label_get_text(lv_obj_get_child(btn,0));if(txt)self->handleKey(String(txt));}
}
