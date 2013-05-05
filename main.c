#include <stdio.h>
#include <allegro.h>
#include "mappyal.h"

#define MODE GFX_AUTODETECT_WINDOWED
#define WIDTH 640
#define HEIGHT 480
#define JUMPIT 1600

// a little bounding box quickie
#define check_collision(x1,y1,w1,h1,x2,y2,w2,h2) (!( ((x1)>=(x2)+(w2)) || ((x2)>=(x1)+(w1)) || \
                                                        ((y1)>=(y2)+(h2)) || ((y2)>=(y1)+(h1)) ))

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
SPRITE *enemy[10]; 
SPRITE *turtle;

BITMAP *buffer;
BITMAP *temp;
BITMAP *animTemp;
BITMAP *back;

int jump = JUMPIT;

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

void MoveEnemy()
{
    int i=0;
    
    for(i=0; i<10; i++)
    {
        if(collided(enemy[i]->x, enemy[i]->y+enemy[i]->height/2) || collided(enemy[i]->x+enemy[i]->width, enemy[i]->y+enemy[i]->height/2))
        {
            enemy[i]->dir = -enemy[i]->dir;
        }
        
        if(!collided(enemy[i]->x, enemy[i]->y+enemy[i]->height) || !collided(enemy[i]->x+enemy[i]->width, enemy[i]->y+enemy[i]->height))
        {
            enemy[i]->dir = -enemy[i]->dir;
        }
        
        enemy[i]->x += enemy[i]->dir*2;
        
        if(enemy[i]->x < 0)
        {
            enemy[i]->x = 0;
            enemy[i]->dir = -enemy[i]->dir;
        }
        
        if(enemy[i]->x > (mapwidth*mapblockwidth))
        {
            enemy[i]->x = mapwidth*mapblockwidth;
            enemy[i]->dir = -enemy[i]->dir;
        }
        
        //first check if player jumped on the enemy
        if(check_collision(player->x, player->y+player->width-5, player->width, 5, \
                enemy[i]->x, enemy[i]->y, enemy[i]->width, 5))
        {
            enemy[i]->alive = FALSE;
            enemy[i]->x = 0;
            enemy[i]->y = 0;
        }
        else if(check_collision(player->x, player->y, player->width, player->width, \
                enemy[i]->x, enemy[i]->y, enemy[i]->width, enemy[i]->height))
        {//check if player touch the enemy
            player->x = 220;
        }
    }
    
}

void InitializeEnemy()
{
    int n=0;
    
    //intialize the mash enemy
    for(n=0; n<6; n++)
    {
        enemy[n] = malloc(sizeof(SPRITE));
        enemy[n]->curframe = 0;
        enemy[n]->framecount = 0;
        enemy[n]->framedelay = 10;
        enemy[n]->maxframe = 1;
        enemy[n]->dir = -1;
        enemy[n]->width = enemy_image[0]->w;
        enemy[n]->height = enemy_image[0]->h;  
    }
    
    //initialize the mash enemys' position
    enemy[0]->x = 800;
    enemy[0]->y = 418;
    
    enemy[1]->x = 800;
    enemy[1]->y = 195;
    
    enemy[2]->x = 2500;
    enemy[2]->y = 418;
    
    enemy[3]->x = 3425;
    enemy[3]->y = 290;
    
    enemy[4]->x = 4540;
    enemy[4]->y = 418;
    
    enemy[5]->x = 6800;
    enemy[5]->y = 418;
    
    //initialize the turtle enemy
    for(n=6; n<10; n++)
    {
        enemy[n] = malloc(sizeof(SPRITE));
        enemy[n]->curframe = 0;
        enemy[n]->framecount = 0;
        enemy[n]->framedelay = 10;
        enemy[n]->maxframe = 1;
        enemy[n]->dir = -1;
        enemy[n]->width = turtle_image[0]->w;
        enemy[n]->height = turtle_image[0]->h;
    }
    
    //nitialize the turtles' position
    enemy[6]->x = 950;
    enemy[6]->y = 410;
    
    enemy[7]->x = 1800;
    enemy[7]->y = 120;
    
    enemy[8]->x = 4600;
    enemy[8]->y = 410;
    
    enemy[8]->x = 7000;
    enemy[8]->y = 410;
}

int main(void)
{
    int mapxoff,mapyoff;
    int oldpy,oldpx;
    int facing = 0;
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
    
    
    InitializeEnemy();
  
  
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
            
        //move enemy
        MoveEnemy();
            
        
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
        
        for(n=0; n<10; n++)
        {
            if(enemy[n]->alive)  
            { 
                if(++enemy[n]->framecount > enemy[n]->framedelay)
                {
                    if(++enemy[n]->curframe > enemy[n]->maxframe)
                    enemy[n]->curframe = 0;
                
                    enemy[n]->framecount = 0;
                }
                
                if(n<6)
                    draw_sprite(buffer, enemy_image[enemy[n]->curframe], enemy[n]->x, enemy[n]->y);
                else
                {
                    if(enemy[n]->dir > 0)
                        draw_sprite(buffer, turtle_image[enemy[n]->curframe], enemy[n]->x, enemy[n]->y);
                    else
                        draw_sprite_h_flip(buffer, turtle_image[enemy[n]->curframe], enemy[n]->x, enemy[n]->y);
                }
                    
            }
        }
        
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
































































































































































































































