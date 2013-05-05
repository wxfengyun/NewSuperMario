#include <stdio.h>
#include <allegro.h>
#include "mappyal.h"

#define MODE GFX_AUTODETECT_WINDOWED
#define WIDTH 640
#define HEIGHT 480
#define JUMPIT 1600

//define the sprite structure
typedef struct SPRITE
{
    int dir,alive;
    int x,y;
    int width,height;
    int xspeed,yspeed;
    int xdelay,ydelay;
    int xcount,ycount;
    int curframe,maxframe,anidir;
    int framecount,framedelay;
}SPRITE;

//decalre the bitmaps and sprites
BITMAP *player_image[8];
BITMAP *enemy_image[2];
BITMAP *turtle_image[2];

SPRITE *player;
SPRITE *enemy; 
SPRITE *turtle;

BITMAP *buffer;
BITMAP *temp;
BITMAP *animTemp;
BITMAP *back;

//tile grabber
BITMAP *grabframe(BITMAP *source,
                  int width, int height,
                  int startx,int starty,
                  int columns,int frame)
{
    BITMAP *temp = create_bitmap(width,height);
    int x = startx + (frame%columns) * width;
    int y = starty + (frame/columns) * height;
    blit(source, temp, x, y, 0, 0, width, height);
    return temp;
}

int collided(int x, int y)
{
    BLKSTR *blockdata;
    
    if (y < 0 || y >= mapblockheight*mapheight)
        return 0;
    
    blockdata = MapGetBlock(x/mapblockwidth, y/mapblockheight);
    return blockdata->tl;
}

int main(void)
{
    int mapxoff,mapyoff;
    int oldpy,oldpx;
    int facing = 0;
    int jump = JUMPIT;
    int n;
    int tempX, tempY;
    int touchBrick=0;
    int animDelay = 7;
    
    SAMPLE *sample;
    int panning = 128;
    int pitch = 1000;
    int volume = 255;
    
    allegro_init();
    install_timer();
    install_keyboard();
    set_color_depth(16);
    set_gfx_mode(MODE,WIDTH,HEIGHT,0,0);
    
    MapInitAnims();
    
    if(install_sound(DIGI_AUTODETECT,MIDI_NONE,"")!=0)
    {
        allegro_message("Error initializing sound system");
        return 1;
    }
    
    sample = load_sample("hundouluo.wav");
    if(!sample)
    {
        allegro_message("Error reading wave file");
        return 1;
    }
    
    play_sample(sample,volume,panning,pitch,TRUE);
    
    //load the player sprite
    temp = load_bitmap("guy.bmp", NULL);
    for(n=0; n<8; n++)
    {
        player_image[n] = grabframe(temp,50,60,0,0,8,n);
    }
    
    destroy_bitmap(temp); 
    
    //load the enemy sprite
    temp = load_bitmap("enemy.bmp", NULL);
    for(n=0; n<2; n++)
        enemy_image[n] = grabframe(temp, 32, 32, 0, 0, 2, n);

    destroy_bitmap(temp);
    
    //load the turtle sprite
    temp = load_bitmap("turtle.bmp", NULL);
    for(n=0; n<2; n++)
        turtle_image[n] = grabframe(temp, 40, 40, 0, 0, 2, n);

    destroy_bitmap(temp);
    
    //initialize the sprite
    player = malloc(sizeof(SPRITE));
    player->x = 220;
    player->y = 100;
    player->curframe = 0;
    player->framecount = 0;
    player->framedelay = 6;
    player->maxframe = 7;
    player->width = player_image[0]->w;
    player->height = player_image[0]->h;
    
    enemy = malloc(sizeof(SPRITE));
    enemy->x = 300;
    enemy->y = 200;
    enemy->curframe = 0;
    enemy->framecount = 0;
    enemy->framedelay = 10;
    enemy->maxframe = 1;
    enemy->width = enemy_image[0]->w;
    enemy->height = enemy_image[0]->h;
    
    //load the map
    MapLoad("platform.fmp");
    
    //create the double buffer
    buffer = create_bitmap(mapwidth*mapblockwidth, mapheight*mapblockheight);
    animTemp = create_bitmap(mapblockwidth, mapblockheight);
    back = create_bitmap(mapblockwidth, mapblockheight);
    clear(buffer);
    clear(animTemp);
    clear(back);
    
    MapDrawBG(back,0, 0, 0,0, mapblockwidth, mapblockheight);
    
    //main loop
    while(!key[KEY_ESC])
    {
        oldpy = player->y;
        oldpx = player->x;
        
        
        
        if(key[KEY_RIGHT])
        {
            facing = 1;
            player->x += 3;
            if(++player->framecount > player->framedelay)
            {
                player->framecount = 0;
                if(++player->curframe > player->maxframe)
                    player->curframe = 1;
            }
        }
        else if(key[KEY_LEFT])
        {
            facing = 0;
            player->x -= 3;
            if(++player->framecount > player->framedelay)
            {
                player->framecount = 0;
                if(++player->curframe > player->maxframe)
                    player->curframe = 1;
            }
        }
        else player->curframe = 0;
        
        //handle jumping
        if(jump == JUMPIT)
        {
            if(!collided(player->x + player->width/2, player->y + player->height+5))
                jump = 0;
            
            if(key[KEY_UP])
                jump = 38;
        }
        else
        {
        
            if(collided(player->x + player->width/2, player->y ) )
            {
                jump = 0;
                player->y++;
            }
            jump--;
            player->y -= jump/3;
            
        }
        
        if(jump<0)
        {
             
            if(collided(player->x+player->width/2, player->y+player->height))
            {
                jump = JUMPIT;
                while(collided(player->x + player->width/2, player->y+player->height))
                    player->y -= 2;
            }
        }
        
        //check for collision with foreground tiles
        if ( player->y > 0 && player->y < (mapblockheight*mapheight-player->height) )
        {
           
            BLKSTR * myblock, *myblock1, *myblock2;
            
            if(!facing)
            {         
                myblock = MapGetBlock (player->x/mapblockwidth, (player->y + player->height)/mapblockheight);
                if (myblock->tl) 
                { 
                    player->x = oldpx;
                }
    
            }
            else
            {
                myblock = MapGetBlock((player->x + player->width)/mapblockwidth, (player->y + player->height)/mapblockheight);
                if (myblock->tl) 
                { 
                    player->x = oldpx;
                }
            }
            
            //头顶碰到障碍 
            myblock1 = MapGetBlock ((player->x+ player->width/2)/mapblockwidth, (player->y)/mapblockheight);
            //头顶碰到金币
            if(myblock1->user6 == 1)
            {
                MapSetBlock((player->x+ player->width/2)/mapblockwidth, player->y/mapblockheight, 1);
            }
            
            //头顶碰到砖头
            if(myblock1->user6 == 2) 
            {
                
                touchBrick = 1;
                tempX = player->x + player->width/2;
                tempY = player->y ;
                
                tempX = MapGetXOffset(tempX,tempY);
                tempY = MapGetYOffset(tempX,tempY);
                tempX *= mapblockwidth;
                tempY *= mapblockheight;
                
                MapDrawFG(animTemp, tempX, tempY, 0,0, mapblockwidth, mapblockheight, 1 ); 
                          
                //MapSetBlock((player->x + player->width/2)/mapblockwidth, player->y/mapblockheight, 8);
           
            }
            
            //脚下碰到金币 
            myblock2 = MapGetBlock ((player->x+ player->width/2)/mapblockwidth, (player->y+player->height+5)/mapblockheight);
            if(myblock2->user6 == 1)
            {
                MapSetBlock((player->x+ player->width/2)/mapblockwidth, (player->y+player->height+5)/mapblockheight, 1);
            }
            
            
        }
        
        //update the map scroll position
        
        if(player->x > (mapwidth*mapblockwidth))
            player->x = mapwidth*mapblockwidth;

            

        if(player->y >= mapblockheight*mapheight)
        {
            player->x = 220;
            player->y = 100;
            jump == JUMPIT;
            continue;
        }
        
        mapxoff = player->x + player->width/2  - WIDTH/2 + 10;
        mapyoff = player->y + player->height/2 - HEIGHT/2 + 10;
        
        
        //avoid moving beyond the map edge
        if(mapxoff<0)
            mapxoff = 0;
        if(mapxoff > (mapwidth * mapblockwidth - WIDTH))
            mapxoff = mapwidth * mapblockwidth - WIDTH;
        if(mapyoff<0)
            mapyoff = 0;
        if(mapyoff > (mapheight * mapblockheight - HEIGHT))
            mapyoff = mapheight * mapblockheight - HEIGHT;
            
        
        //draw the background tiles
        MapDrawBG(buffer,mapxoff, mapyoff, mapxoff,mapyoff, WIDTH-1, HEIGHT-1);
        
        //draw the foreground tiles
        MapDrawFG(buffer,mapxoff, mapyoff, mapxoff,mapyoff, WIDTH-1, HEIGHT-1,0 );
        MapDrawFG(buffer,mapxoff, mapyoff, mapxoff,mapyoff, WIDTH-1, HEIGHT-1,1 );
        MapDrawFG(buffer,mapxoff, mapyoff, mapxoff,mapyoff, WIDTH-1, HEIGHT-1,2 );
        
        
        //draw the palyer's sprite
        if(facing)
            draw_sprite(buffer,player_image[player->curframe], player->x, player->y);
        else
            draw_sprite_h_flip(buffer,player_image[player->curframe], player->x, player->y);
           
        if(++enemy->framecount > enemy->framedelay)
        {
            if(++enemy->curframe > enemy->maxframe)
                enemy->curframe = 0;
            
            enemy->framecount = 0;
        }
        draw_sprite(buffer, enemy_image[enemy->curframe], enemy->x, enemy->y);
        
        //blit the double buffer
        vsync();
        acquire_screen();
        if(touchBrick)
        {   
                    
            //blit(buffer, animTemp,  tempX, tempY, 0, 0, mapblockwidth, mapblockheight);
            
            blit(back, buffer, 0,0, tempX, tempY, mapblockwidth, mapblockheight);
            blit(animTemp, buffer, 0, 0, tempX, tempY-animDelay, mapblockwidth, mapblockheight);
            animDelay--;
            if(animDelay == 0 )
            {
                touchBrick = 0;
                animDelay = 7;
            }
        }
        masked_blit(buffer,screen,mapxoff,mapyoff,0,0,WIDTH-1,HEIGHT-1);
     
        release_screen();
        
        MapUpdateAnims();
    }//while
    
    //clean up
    for(n=0; n<8; n++)
        destroy_bitmap(player_image[n]);
    free(player);
    destroy_bitmap(buffer);
    MapFreeMem();
    allegro_exit();
}
END_OF_MAIN()
































































































































































































































