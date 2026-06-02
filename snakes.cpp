// Snakes & Ladders — SFML 3 (MSYS2 mingw-w64-x86_64-sfml)
// Compile in MSYS2 MinGW64 terminal:
// g++ snakes.cpp -o game.exe -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
// Run: ./game.exe

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

// ── Constants ─────────────────────────────────────────────────
static const int WW=1100,WH=780,OX=30,OY=30,CS=70,NC=10;
static const int PX=OX+CS*NC+20, PW=WW-PX-20;

static const std::map<int,int> SNAKES ={{99,7},{95,75},{92,88},{89,68},{74,53},{64,60},{62,19},{49,11},{46,25},{16,6}};
static const std::map<int,int> LADDERS={{4,14},{9,31},{20,38},{28,84},{40,59},{51,67},{63,81},{71,91}};
static const sf::Color PC[4]={sf::Color(220,60,60),sf::Color(60,140,220),sf::Color(60,190,90),sf::Color(230,180,40)};
static const char* PN[4]={"Red","Blue","Green","Yellow"};

// ── Helpers ───────────────────────────────────────────────────
sf::Vector2f c2p(int cell){
    int i=cell-1,row=i/NC,col=i%NC;
    if(row%2==1)col=NC-1-col;
    return {OX+col*CS+CS/2.f, OY+(NC-1-row)*CS+CS/2.f};
}
void tline(sf::RenderWindow& w,sf::Vector2f a,sf::Vector2f b,float t,sf::Color c){
    sf::Vector2f d=b-a; float L=std::sqrt(d.x*d.x+d.y*d.y);
    sf::RectangleShape r({L,t}); r.setFillColor(c); r.setOrigin({0,t/2});
    r.setPosition(a); r.setRotation(sf::degrees(std::atan2(d.y,d.x)*180.f/3.14159f));
    w.draw(r);
}
void drawTxt(sf::RenderWindow& w,const sf::Font& f,const std::string& s,unsigned sz,sf::Color c,float x,float y,bool center=false){
    sf::Text t(f,s,sz); t.setFillColor(c);
    if(center){auto b=t.getLocalBounds();t.setOrigin({b.position.x+b.size.x/2,b.position.y+b.size.y/2});}
    t.setPosition({x,y}); w.draw(t);
}

// ── Button (heap-allocated sf::Text via unique_ptr) ────────────
struct Btn{
    sf::RectangleShape box;
    std::unique_ptr<sf::Text> lbl;
    sf::Color base;

    void init(float x,float y,float bw,float bh,const std::string& s,const sf::Font& f,
              sf::Color bg={70,130,180},sf::Color fg=sf::Color::White){
        base=bg;
        box.setPosition({x,y}); box.setSize({bw,bh});
        box.setFillColor(bg); box.setOutlineColor({255,255,255,50}); box.setOutlineThickness(1.5f);
        lbl=std::make_unique<sf::Text>(f,s,18);
        lbl->setFillColor(fg);
        auto b=lbl->getLocalBounds();
        lbl->setOrigin({b.position.x+b.size.x/2,b.position.y+b.size.y/2});
        lbl->setPosition({x+bw/2,y+bh/2});
    }
    bool hit(sf::Vector2i m)const{return box.getGlobalBounds().contains({(float)m.x,(float)m.y});}
    void hover(sf::Vector2i m){sf::Color c=base;c.a=hit(m)?200:255;box.setFillColor(c);}
    void setCol(sf::Color c){box.setFillColor(c);}
    void draw(sf::RenderWindow& w){w.draw(box);if(lbl)w.draw(*lbl);}
};

// ── Dice ──────────────────────────────────────────────────────
struct Dice{
    int val=1; bool spin=false; float timer=0;
    void roll(){spin=true;timer=0;}
    void update(float dt){if(!spin)return;timer+=dt;val=rand()%6+1;if(timer>=0.6f)spin=false;}
    void draw(sf::RenderWindow& w,float x,float y,float sz){
        sf::RectangleShape bg({sz,sz}); bg.setPosition({x,y});
        bg.setFillColor({245,240,225}); bg.setOutlineColor({100,90,70}); bg.setOutlineThickness(2); w.draw(bg);
        float c=sz/2,q=sz/4,r=sz*0.07f;
        // dot positions per face value
        std::vector<std::pair<float,float>> dots;
        switch(val){
            case 1: dots={{c,c}}; break;
            case 2: dots={{q,q},{3*q,3*q}}; break;
            case 3: dots={{q,q},{c,c},{3*q,3*q}}; break;
            case 4: dots={{q,q},{3*q,q},{q,3*q},{3*q,3*q}}; break;
            case 5: dots={{q,q},{3*q,q},{c,c},{q,3*q},{3*q,3*q}}; break;
            case 6: dots={{q,q},{3*q,q},{q,c},{3*q,c},{q,3*q},{3*q,3*q}}; break;
        }
        for(auto& p:dots){
            sf::CircleShape d(r); d.setFillColor({30,20,10});
            d.setPosition({x+p.first-r,y+p.second-r}); w.draw(d);
        }
    }
};

// ── Player ────────────────────────────────────────────────────
struct Player{
    std::string name; sf::Color color;
    int pos=0; bool done=false,moving=false,target=0;
    float mt=0; sf::Vector2f apos;
    void startMove(int to){target=to;moving=true;mt=0;apos=c2p(pos>0?pos:1);}
    void update(float dt){
        if(!moving)return; mt+=dt;
        float t=std::min(mt/0.3f,1.f); t=t*t*(3-2*t);
        sf::Vector2f fr=pos>0?c2p(pos):apos, to2=c2p(target);
        apos=fr+t*(to2-fr); apos.y-=std::sin(t*3.14159f)*18;
        if(mt>=0.3f){pos=target;apos=c2p(target);moving=false;}
    }
    sf::Vector2f dp()const{
        if(moving)return apos; if(pos<=0)return{-999,-999}; return c2p(pos);
    }
    void draw(sf::RenderWindow& w,int slot)const{
        auto p=dp(); if(p.x<0)return;
        float ox=slot%2==0?-12:12, oy=slot/2==0?-12:12, r=14;
        sf::CircleShape tk(r); tk.setOrigin({r,r}); tk.setPosition({p.x+ox,p.y+oy});
        tk.setFillColor(color); tk.setOutlineColor(sf::Color::White); tk.setOutlineThickness(2.5f); w.draw(tk);
        sf::CircleShape in(r*0.35f); in.setOrigin({r*0.35f,r*0.35f}); in.setPosition({p.x+ox,p.y+oy});
        in.setFillColor({255,255,255,150}); w.draw(in);
    }
};

// ── Board ─────────────────────────────────────────────────────
void drawBoard(sf::RenderWindow& w){
    for(int cell=1;cell<=100;cell++){
        int i=cell-1,row=i/NC,col=i%NC; if(row%2==1)col=NC-1-col;
        float px=OX+col*CS, py=OY+(NC-1-row)*CS;
        sf::Color fill=(row+col)%2==0?sf::Color(255,248,210):sf::Color(200,230,200);
        if(SNAKES.count(cell))  fill={255,180,180};
        if(LADDERS.count(cell)) fill={180,255,180};
        if(cell==1)   fill={200,240,255};
        if(cell==100) fill={255,215,0};
        sf::RectangleShape r({(float)CS-1,(float)CS-1});
        r.setPosition({px+0.5f,py+0.5f}); r.setFillColor(fill);
        r.setOutlineColor({160,140,110}); r.setOutlineThickness(0.5f); w.draw(r);
    }
}
void drawSnakes(sf::RenderWindow& w){
    for(auto& s:SNAKES){
        auto h=c2p(s.first),t=c2p(s.second);
        for(int i=0;i<=30;i++){
            float tt=(float)i/30,ttt=1-tt;
            sf::Vector2f mid={(h.x+t.x)/2+30*std::sin(tt*6.28f),(h.y+t.y)/2};
            sf::Vector2f pt=ttt*ttt*h+2*ttt*tt*mid+tt*tt*t;
            float r=5-tt*2; sf::CircleShape seg(r); seg.setOrigin({r,r}); seg.setPosition(pt);
            seg.setFillColor({(uint8_t)(220*ttt+160*tt),(uint8_t)(60*ttt+20*tt),60,210}); w.draw(seg);
        }
        sf::CircleShape hc(9); hc.setOrigin({9,9}); hc.setPosition(h);
        hc.setFillColor({200,40,40}); hc.setOutlineColor({80,0,0}); hc.setOutlineThickness(1.5f); w.draw(hc);
        for(int e=-1;e<=1;e+=2){
            sf::CircleShape eye(2); eye.setOrigin({2,2}); eye.setPosition({h.x+e*4,h.y-3});
            eye.setFillColor(sf::Color::White); w.draw(eye);
            sf::CircleShape pu(1); pu.setOrigin({1,1}); pu.setPosition({h.x+e*4,h.y-3});
            pu.setFillColor(sf::Color::Black); w.draw(pu);
        }
    }
}
void drawLadders(sf::RenderWindow& w){
    for(auto& l:LADDERS){
        auto b=c2p(l.first),t=c2p(l.second);
        sf::Vector2f d=t-b; float len=std::sqrt(d.x*d.x+d.y*d.y);
        sf::Vector2f u={d.x/len,d.y/len},p={-u.y*10,u.x*10};
        tline(w,b+p,t+p,4,{139,90,43,200}); tline(w,b-p,t-p,4,{139,90,43,200});
        int nr=(int)(len/22);
        for(int r=1;r<nr;r++){float tt=(float)r/nr;auto rc=b+tt*(t-b);tline(w,rc-p,rc+p,3.5f,{180,130,70,230});}
    }
}

// ── Main ──────────────────────────────────────────────────────
enum class GS{MENU,SEL,PLAY,WIN};

int main(){
    srand((unsigned)time(nullptr));
    sf::RenderWindow win(sf::VideoMode({(unsigned)WW,(unsigned)WH}),"Snakes & Ladders",
                         sf::Style::Titlebar|sf::Style::Close);
    win.setFramerateLimit(60);
    sf::Font font;
    const char* fp[]={"C:/Windows/Fonts/Arial.ttf","C:/Windows/Fonts/calibri.ttf","C:/Windows/Fonts/tahoma.ttf"};
    for(auto& p:fp)if(font.openFromFile(p))break;

    GS state=GS::MENU; int nP=2,cur=0;
    bool waitRoll=true,anim=false,pendSL=false,tkStart=false,secStart=false;
    int pendDest=-1; float evTimer=0;
    std::string statusMsg="Press SPACE or Roll!",evMsg="";
    std::vector<Player> players;
    Dice dice;

    // All buttons on heap so sf::Text default-construct is avoided
    Btn bPlay,bQuit,b2P,b3P,b4P,bStart,bRoll,bMenu,bAgain,bMMenu;

    auto mkBtns=[&](){
        bPlay .init(WW/2-100,320,200,55,"PLAY",      font,{60,160,80});
        bQuit .init(WW/2-100,395,200,55,"QUIT",      font,{160,60,60});
        b2P   .init(WW/2-200,340,110,50,"2 Players", font);
        b3P   .init(WW/2-80, 340,110,50,"3 Players", font);
        b4P   .init(WW/2+40, 340,110,50,"4 Players", font);
        bStart.init(WW/2-90, 430,180,55,"START",     font,{60,160,80});
        bRoll .init(PX,460,PW,50,"Roll (SPACE)",     font,{60,160,80});
        bMenu .init(PX,530,PW,40,"Main Menu",        font,{100,100,100});
        bAgain.init(WW/2-140,450,260,55,"Play Again",font,{60,160,80});
        bMMenu.init(WW/2-140,520,260,50,"Main Menu", font,{80,80,80});
    };
    mkBtns();

    auto initP=[&](){
        players.clear();
        for(int i=0;i<nP;i++){Player p;p.name=PN[i];p.color=PC[i];p.pos=0;p.apos=c2p(1);players.push_back(p);}
        cur=0;waitRoll=true;anim=false;pendSL=false;tkStart=false;secStart=false;
        statusMsg=std::string(PN[0])+"'s turn - Roll!"; evMsg="";
    };
    auto doRoll=[&](){if(!waitRoll||anim)return;dice.roll();anim=true;waitRoll=false;};

    sf::Clock clk;
    while(win.isOpen()){
        float dt=clk.restart().asSeconds();
        auto mp=sf::Mouse::getPosition(win);

        while(auto ev=win.pollEvent()){
            if(ev->is<sf::Event::Closed>())win.close();
            if(auto* k=ev->getIf<sf::Event::KeyPressed>())
                if(k->code==sf::Keyboard::Key::Space&&state==GS::PLAY)doRoll();
            if(auto* mb2=ev->getIf<sf::Event::MouseButtonReleased>()){
                if(mb2->button==sf::Mouse::Button::Left){
                    sf::Vector2i m((int)mb2->position.x,(int)mb2->position.y);
                    if(state==GS::MENU){if(bPlay.hit(m))state=GS::SEL;if(bQuit.hit(m))win.close();}
                    else if(state==GS::SEL){if(b2P.hit(m))nP=2;if(b3P.hit(m))nP=3;if(b4P.hit(m))nP=4;if(bStart.hit(m)){initP();state=GS::PLAY;}}
                    else if(state==GS::PLAY){if(bRoll.hit(m))doRoll();if(bMenu.hit(m))state=GS::MENU;}
                    else if(state==GS::WIN){if(bAgain.hit(m)){initP();state=GS::PLAY;}if(bMMenu.hit(m))state=GS::MENU;}
                }
            }
        }

        if(state==GS::MENU){bPlay.hover(mp);bQuit.hover(mp);}
        else if(state==GS::SEL){b2P.hover(mp);b3P.hover(mp);b4P.hover(mp);bStart.hover(mp);}
        else if(state==GS::PLAY){bRoll.hover(mp);bMenu.hover(mp);}
        else if(state==GS::WIN){bAgain.hover(mp);bMMenu.hover(mp);}

        if(anim){
            dice.update(dt);
            if(!dice.spin&&!players[cur].moving&&!pendSL){
                if(!tkStart){
                    Player& p=players[cur]; int np=(p.pos?p.pos:0)+dice.val;
                    if(np>100)np=100; if(p.pos==0)p.pos=1; p.startMove(np); tkStart=true;
                }else if(!players[cur].moving){
                    tkStart=false; Player& p=players[cur];
                    auto si=SNAKES.find(p.pos),li=LADDERS.find(p.pos);
                    if(si!=SNAKES.end()){pendSL=true;pendDest=si->second;evMsg="SNAKE! "+p.name+" slides to "+std::to_string(pendDest)+"!";evTimer=2.5f;}
                    else if(li!=LADDERS.end()){pendSL=true;pendDest=li->second;evMsg="LADDER! "+p.name+" climbs to "+std::to_string(pendDest)+"!";evTimer=2.5f;}
                    else{if(p.pos>=100){p.done=true;state=GS::WIN;}else{cur=(cur+1)%nP;waitRoll=true;anim=false;statusMsg=std::string(PN[cur])+"'s turn - Roll!";}}
                }
            }
            if(pendSL&&!players[cur].moving){
                if(!secStart){players[cur].startMove(pendDest);secStart=true;}
                else{secStart=false;pendSL=false;Player& p=players[cur];
                    if(p.pos>=100){p.done=true;state=GS::WIN;}else{cur=(cur+1)%nP;waitRoll=true;anim=false;statusMsg=std::string(PN[cur])+"'s turn - Roll!";}}
            }
        }
        for(auto& p:players)p.update(dt);
        if(evTimer>0){evTimer-=dt;if(evTimer<0)evTimer=0;}

        win.clear({28,28,35});

        if(state==GS::MENU){
            drawTxt(win,font,"SNAKES & LADDERS",52,{255,215,60},WW/2.f,180,true);
            drawTxt(win,font,"A Classic Board Game",22,{180,180,200},WW/2.f,250,true);
            bPlay.draw(win);bQuit.draw(win);
        }
        else if(state==GS::SEL){
            drawTxt(win,font,"Select Players",36,{255,215,60},WW/2.f,220,true);
            for(int i=2;i<=4;i++){
                Btn& b=i==2?b2P:i==3?b3P:b4P;
                if(nP==i){sf::RectangleShape hl(b.box.getSize()+sf::Vector2f(6,6));
                    hl.setPosition(b.box.getPosition()-sf::Vector2f(3,3));
                    hl.setFillColor(sf::Color::Transparent);hl.setOutlineColor({255,215,60});hl.setOutlineThickness(3);win.draw(hl);}
                b.draw(win);
            }
            for(int i=0;i<nP;i++){
                sf::CircleShape c(18);c.setFillColor(PC[i]);c.setOutlineColor(sf::Color::White);c.setOutlineThickness(2);
                float px2=WW/2.f-(nP-1)*50.f/2+i*50; c.setPosition({px2-18,500}); win.draw(c);
                drawTxt(win,font,PN[i],13,{200,200,220},px2,524,true);
            }
            bStart.draw(win);
        }
        else if(state==GS::PLAY){
            drawBoard(win);drawLadders(win);drawSnakes(win);
            for(int cell=1;cell<=100;cell++){auto cp=c2p(cell);drawTxt(win,font,std::to_string(cell),10,{80,60,40,180},cp.x,cp.y-CS/2.f+10,true);}
            for(int i=0;i<(int)players.size();i++)players[i].draw(win,i);

            sf::RectangleShape panel({(float)PW,(float)WH-60});panel.setPosition({(float)PX,30});
            panel.setFillColor({40,40,52});panel.setOutlineColor({80,80,100});panel.setOutlineThickness(1.5f);win.draw(panel);
            drawTxt(win,font,"SNAKES &\nLADDERS",24,{255,215,60},(float)PX+10,45);
            sf::RectangleShape dv1({(float)PW-20,1.5f});dv1.setPosition({(float)PX+10,120});dv1.setFillColor({80,80,100});win.draw(dv1);

            float py2=132;
            for(int i=0;i<(int)players.size();i++){
                bool ic=i==cur;
                if(ic){sf::RectangleShape hl({(float)PW-10,38});hl.setPosition({(float)PX+5,py2-4});hl.setFillColor({70,70,90});win.draw(hl);}
                sf::CircleShape dot(10);dot.setFillColor(players[i].color);dot.setOutlineColor(sf::Color::White);dot.setOutlineThickness(1.5f);dot.setPosition({(float)PX+14,py2+4});win.draw(dot);
                std::string info=players[i].name+(players[i].pos>0?" ["+std::to_string(players[i].pos)+"]":" [Start]");
                drawTxt(win,font,info,15,ic?sf::Color(255,215,60):sf::Color(200,200,220),(float)PX+40,py2+2);
                if(ic)drawTxt(win,font,">",16,{255,215,60},(float)PX+4,py2+2);
                py2+=42;
            }
            sf::RectangleShape dv2({(float)PW-20,1.5f});dv2.setPosition({(float)PX+10,py2+4});dv2.setFillColor({80,80,100});win.draw(dv2);
            float dy=py2+20; dice.draw(win,(float)PX+PW/2.f-35,dy,70);
            drawTxt(win,font,"Rolled: "+std::to_string(dice.val),15,{180,180,200},(float)PX+PW/2.f,dy+75,true);
            drawTxt(win,font,statusMsg,14,{220,220,240},(float)PX+8,415);
            if(evTimer>0&&!evMsg.empty()){
                uint8_t a=(uint8_t)(std::min(1.f,evTimer)*255);
                drawTxt(win,font,evMsg,13,{255,200,100,a},(float)PX+8,432);
            }
            bRoll.setCol((!waitRoll||anim)?sf::Color(60,60,70):sf::Color(60,160,80));
            bRoll.draw(win);bMenu.draw(win);
            float ly=WH-120.f;
            drawTxt(win,font,"Snakes: slide DOWN",12,{220,100,100},(float)PX+8,ly);
            drawTxt(win,font,"Ladders: climb UP", 12,{100,200,100},(float)PX+8,ly+20);
            drawTxt(win,font,"Reach 100 to WIN!", 12,{255,215,60}, (float)PX+8,ly+40);
        }
        else if(state==GS::WIN){
            sf::RectangleShape ov({(float)WW,(float)WH});ov.setFillColor({0,0,0,180});win.draw(ov);
            std::string wn=cur<(int)players.size()?players[cur].name:"Someone";
            drawTxt(win,font,"CONGRATULATIONS!",46,{255,215,60},WW/2.f,260,true);
            drawTxt(win,font,wn+" WINS!",34,{200,240,255},WW/2.f,340,true);
            if(cur<(int)players.size()){
                sf::CircleShape tok(36);tok.setOrigin({36,36});tok.setPosition({WW/2.f,405});
                tok.setFillColor(players[cur].color);tok.setOutlineColor(sf::Color::White);tok.setOutlineThickness(4);win.draw(tok);
            }
            bAgain.draw(win);bMMenu.draw(win);
        }
        win.display();
    }
    return 0;
}
