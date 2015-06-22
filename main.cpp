#include <string.h>
#include <vector>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <sstream> 
#include "blockedin.hpp"
#include <stdio.h>
#include <fstream>
#include <ctime>
using namespace std;
 
int main(int argc, char *argv[]) {
    Blockenspiel b;
    b.init(); 
    b.run();

    SDL_Quit();
 
    return 0;
}

const int Blockenspiel::menuitem_x[5] = {30,122,214,306,398};
const int Blockenspiel::menuitem_y[5] = {17,17,17,17,17};
string Blockenspiel::menuItemDescriptions[5] = { "restart level","undo move","previous level", "next level", "menu" };

void Blockenspiel::init(){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER )<0) {
        cerr << "Failed SDL_Init " << SDL_GetError() << endl;
        return;
    }

    SDL_WM_SetCaption("Blockenspiel", "Blockenspiel");

    if(TTF_Init() == -1){
        cerr << "Failed TTF_Init " << TTF_GetError() << endl;
        SDL_Quit();
        return;
    }

    screenImg = SDL_SetVideoMode(600,650,32,SDL_HWSURFACE | SDL_DOUBLEBUF);
    cout << "w " << screenImg->w << endl;
    if(screenImg == NULL) {
        cerr << "Failed SDL_SetVideoMode: " << SDL_GetError() << endl;
        SDL_Quit();
        return;
    }

	originX = screenImg->w/2;
	originY = 340;

    //load font
    font = TTF_OpenFont("font.ttf", 14);
    if (font == NULL) {
        cerr << "Unable to load font. error " << TTF_GetError() << endl;
        SDL_Quit();
        return;
    }

	//load images
    texMapImg = getImage("texmap.gif");
    for (int i=0; i<NUM_BLOCK_TYPES; i++){
		blockImgs[i] = getImageFromTexMap(texMapImg, (i % 16)*TILESIZE, (i / 16)*TILESIZE, TILESIZE, TILESIZE);
        //fill out a black mask of this bitmap, so we can darken it
        blockDarkenImgs[i] = createSurface(TILESIZE, TILESIZE);
        SDL_SetColorKey( blockDarkenImgs[i], SDL_RLEACCEL | SDL_SRCCOLORKEY, 0xffffffff );

        SDL_LockSurface(blockImgs[i]);
        SDL_LockSurface(blockDarkenImgs[i]);
        for(int x=0; x<blockImgs[i]->w; x++){
            for(int y=0; y<blockImgs[i]->h; y++){

                if((*getPixelPtr(blockImgs[i], x, y)) != blockImgs[i]->format->colorkey){
                    *getPixelPtr(blockDarkenImgs[i], x, y) = 0;
                } else {
                    *getPixelPtr(blockDarkenImgs[i], x, y) = blockDarkenImgs[i]->format->colorkey;
                }
            }
        }
        SDL_UnlockSurface(blockImgs[i]);
        SDL_UnlockSurface(blockDarkenImgs[i]);
    }
	selectorImg = getImageFromTexMap(texMapImg, 0, 3*TILESIZE, TILESIZE, TILESIZE);
	highlightImg = getImageFromTexMap(texMapImg, TILESIZE, 3*TILESIZE, TILESIZE, TILESIZE);
    blockWhiteImg = getImageFromTexMap(texMapImg, 2*TILESIZE, 3*TILESIZE, TILESIZE, TILESIZE);
    logoImg = getImageFromTexMap(texMapImg, 0, 4*TILESIZE, 600, 247);
    backgroundImg = getImageFromTexMap(texMapImg, 600, 4*TILESIZE, 555, 553);
    arrowImg = getImageFromTexMap(texMapImg, 1155, 4*TILESIZE, 64, 497);
    InGameMenu = getImageFromTexMap(texMapImg, 0, 4*TILESIZE+247, 492, 100);

    fadeImg = createSurface(screenImg->w, screenImg->h);
    SDL_FillRect(fadeImg, NULL, 0);

    //calculate screen coordinates of each block
	for (int x=0; x<10; x++){
		for (int y=0; y<10; y++){
			for (int z=0; z<10; z++){
				screen_x[y][x] = originX -TILESIZE/2 + TILESIZE/2*x -TILESIZE/2*y;
				screen_y[z][y][x] = originY - TILESIZE/2 + TILESIZE/4*x +TILESIZE/4*y -TILESIZE/2*z;
			}
        }
    }

    //read all the levels
	ifstream mapfile ("map.txt");
    if (!mapfile.is_open()){
        cerr << "failed to open map!" << endl;
        return;
    }

    do{
        string line;
        //skip to the next line starting with "Level"
        getline(mapfile, line);
        if(line.compare(0,5,"Level") == 0){
            //parse the file and create a level object
            levelMaps.resize(++numLevels);
            levelMaps[numLevels-1] = new LevelMap(mapfile, line.substr(line.find(":") + 1));
        }
    } while (!mapfile.eof());
	mapfile.close(); 
	
    //read the progress file
    finishedlevels.resize(numLevels);
    ifstream progressFile ("progress.dat",ios::binary);
    if (progressFile.is_open()){
        LevelProgress buf;
        for(int i=0; i < numLevels; i++){
            progressFile.read ((char*)&buf, sizeof(LevelProgress));
            if(progressFile.eof()) break;
            //cout << "read "(int) buf[0] << " "<< (int) buf[1] << " " << (int) buf[2] << " " << (int) buf[3] <<endl;
            /*cout << "read " << buf.isCompleted << (buf.completionTime.tm_year + 1900) << '-' 
         << (buf.completionTime.tm_mon + 1) << '-'
         <<  buf.completionTime.tm_mday
         << endl;*/
            finishedlevels[i] = buf;
        }
    } else {
        for(int i=0; i < numLevels; i++){
            finishedlevels[i].isCompleted = false;
        }
    }

    //initialize the the thumbnails that are used by the menu
    levsel_levelThumbs.resize(numLevels);
    SDL_Surface *buftoshrink = createSurface(600, 650);

    for(int i=0; i < numLevels; i++){
        SDL_FillRect(buftoshrink, NULL, 0);
        map = levelMaps[i];
        blitSurface(backgroundImg, buftoshrink, originX-273, originY-273);
        drawMap(buftoshrink);
        levsel_levelThumbs[i] = createSurface(600/4, 650/4); 
        shrinkSurface(buftoshrink, levsel_levelThumbs[i]);
    }
    map = NULL;

	menu_x = screen_x[0][0]+TILESIZE/2-InGameMenu->w/2;
	menu_y = 10;
}

void Blockenspiel::runLevel() {
    if (map) delete map;
    map = new LevelMap(*levelMaps[level]);
    removeGroup = new LevelMap(map->length, map->width, map->height);
	while(!movestack.empty()){
        delete movestack.top();
        movestack.pop();
    }
    screen = LEVEL;
	isMoveDestinationMet = true;
    if(level == 0) turorialstate = TUTORIAL_WELCOME;
    //cout << "Starting level " << level << ": " << map->length << map->width<< map->height<<endl;
	kickPaintThread();
}

void Blockenspiel::run(){
    stateLock = SDL_CreateMutex();

    repaintCVLock = SDL_CreateMutex();
    repaintCV = SDL_CreateCond();
    paintThread = SDL_CreateThread( paintThreadEntry, this );

    animateCVLock = SDL_CreateMutex();
    animateCV = SDL_CreateCond();
    animationThread = SDL_CreateThread( animationThreadEntry, this );

    kickPaintThread();

    while(1) {
        SDL_Event event;
        if(SDL_WaitEvent(&event)) {
            SDL_mutexP( stateLock );
            switch (event.type) {
                case SDL_MOUSEMOTION:
                    if(handleMouseMotion(event.motion.x, event.motion.y)) {
                        kickPaintThread();
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if(event.button.button == SDL_BUTTON_LEFT){
                        handleMouseClick(event.button.x, event.button.y);
                        kickPaintThread();
                    }
                    break;
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_q ) {
                        SDL_Quit();
                        return;
                    }
                    break;
                case SDL_QUIT:
                    SDL_Quit();
                    return;
            }
            SDL_mutexV( stateLock );
        }
    }
}

int Blockenspiel::paintThreadEntry(void *data) {
    return ((Blockenspiel *)data)->paintThreadFcn();
}

int Blockenspiel::paintThreadFcn() {
    for(;;) {
        SDL_mutexP( repaintCVLock );

        //repaintRequest+repaintCV will get signaled when we need to repaint.  
        while(!repaintRequest) {
            SDL_CondWait( repaintCV, repaintCVLock );
        }
        //cout <<"paint thread: repaint" <<endl;
        repaintRequest = false; //signal that we've received the request and are processing.
        SDL_mutexV( repaintCVLock );
 
        SDL_mutexP( stateLock );
        paint(screenImg);
        SDL_mutexV( stateLock );
        SDL_Flip(screenImg);
    }
}

void Blockenspiel::paint(SDL_Surface *s){
	if (screen == LEVEL_SELECT){
        SDL_FillRect(s, NULL, 0); 
        blitSurface(logoImg, s, 0, - levsel_scrollOffset);
        for (int i=0;i<numLevels;i++){
            if(levsel_underMouseThumb == i){
                SDL_SetAlpha(levsel_levelThumbs[i], SDL_SRCALPHA, 255);
            } else {
                SDL_SetAlpha(levsel_levelThumbs[i], SDL_SRCALPHA, 128); 
            }
            int x = (i % LEVSEL_THUMBS_PER_ROW) * LEVSEL_THUMB_W,
                y = (i / LEVSEL_THUMBS_PER_ROW) * LEVSEL_THUMB_H + logoImg->h - levsel_scrollOffset;

            blitSurface(levsel_levelThumbs[i], s, x, y);
            if(levsel_underMouseThumb == i){
                drawTextCentered(s, levelMaps[i]->name, font, x + LEVSEL_THUMB_W / 2, y + LEVSEL_THUMB_H - 20);
                if(finishedlevels[levsel_underMouseThumb].isCompleted){
                    stringstream sstm;
                    sstm << (finishedlevels[levsel_underMouseThumb].completionTime.tm_year + 1900) << '-' 
                        << (finishedlevels[levsel_underMouseThumb].completionTime.tm_mon + 1) << '-'
                        <<  finishedlevels[levsel_underMouseThumb].completionTime.tm_mday;
                    string tb[] = {"completed" , sstm.str()};
                    drawTextBoxCentered(s, tb, 2, font, x + LEVSEL_THUMB_W / 2, y + LEVSEL_THUMB_H/2);
                }
            }
        }
    } else {
		//fill with black
        SDL_FillRect(s, NULL, 0);
		//draw bg
		blitSurface(backgroundImg, s, originX-273, originY-273);
			
		//draw menu
        stringstream sstm;
        sstm << "Level " << level << ": " << map->name;
        string str = sstm.str();
		drawText(s, str, font, menu_x+20, 0);
		blitSurface(InGameMenu, s, menu_x, menu_y);
		if (highlightedmenuitem!=0){
			SDL_SetAlpha(blockWhiteImg, SDL_SRCALPHA, 128);
			blitSurface(blockWhiteImg, s, menu_x+menuitem_x[highlightedmenuitem-1], menu_y+menuitem_y[highlightedmenuitem-1]);
            drawTextCentered(s, menuItemDescriptions[highlightedmenuitem-1], font, 
                            menu_x+menuitem_x[highlightedmenuitem-1]+TILESIZE/2, menu_y+menuitem_y[highlightedmenuitem-1]+TILESIZE);
		}
			
		//draw map
		drawMap(s);
		if (state == MSG_LEVELFINISHED){
            if(level < numLevels-1) {
                string tb[] = {"Level complete!" , "Click to continue..."};
                drawTextBoxCentered(s, tb, 2, font, screen_x[0][0]+TILESIZE/2, screen_y[0][0][0]);
            } else {
                string tb[] = { "Way to go, you finished the game!", 
                                "",
                                "Kind of anticlimatic, isn't it? No final boss, no",
                                "epic ending sequence, just a boring old text box :(",
                                "Sorry, I'm not Nintendo. I can't be bothered to make",
                                "a fancy ending. I'm just one guy, and I have a",
                                "day job, you know.",
                                "",
                                "Still, I hope you liked the game! If you're some kind",
                                "of masochist, and you're still itching for more levels,",
                                "please make your own! The map file is in a simple text",
                                "format, and you can edit it with notepad. Send me your",
                                "cool homemade levels, and I'll include them in the next",
                                "version of the game.",
                                "",
                                "For feedback, level suggestions, love letters, death",
                                "threats, Nigerian prince scams, etc, please email me",
                                "at ashenfie@gmail.com"
                                };
                drawTextBox(s, tb, 18, font, screen_x[0][0]+TILESIZE/2, screen_y[0][0][0]/2);            
            }
		} else if (state == MSG_LEVELFAILED){
            string tb[] = {"Unmatched block remaining!", "Level failed", " ", "Undo last move or retry level..."};
            drawTextBoxCentered(s, tb, 4, font, screen_x[0][0]+TILESIZE/2, screen_y[0][0][0]);
		}

        if(level==0 && state != MSG_LEVELFINISHED && state != FADE_OUT) {
            switch(turorialstate){
            case TUTORIAL_WELCOME:{
                string tb[] = {"Welcome to Blockenspiel!",
                               "The goal of the game is make all the coloured blocks disappear.",
                               "Click on a Block to Select it..."};
                drawTextBox(s, tb, 3, font, 300, 300);
                break;
                }
            case TUTORIAL_CLICKDEST:{
                string tb[] = {"Move the mouse around to see where you can move this block.",
                               "Click to move it."};
                drawTextBox(s, tb, 2, font, 300, 300);
                break;
                }
                break;
            case TUTORIAL_CLICKDEST_RETRY:{
                string tb[] = {"You can't move there!",
                               "Move the mouse around to find a valid path."};
                drawTextBox(s, tb, 2, font, 300, 300);
                break;
                }
                break;
            case TUTORIAL_MOVETONEIGHTBOUR:{
                string tb[] = {"Great!",
                               "Now move the block again so that it touches another",
                               "block of the same colour"};
                drawTextBox(s, tb, 3, font, 300, 300);
                break;
                }
                break;
            case TUTORIAL_CLEARTHEREST:{
                string tb[] = {"When blocks of the same type touch, they vanish.",
                               "Join the two remaining blocks to finish the level."};
                drawTextBox(s, tb, 2, font, 300, 300);
                break;
                }
            }
        }
	}

    if(state == FADE_IN || state == FADE_OUT){ 
        SDL_SetAlpha(fadeImg, SDL_SRCALPHA, fadeAlpha);
        blitSurface(fadeImg, s, 0, 0);
    }
}

void Blockenspiel::drawMap(SDL_Surface *s){ 
	switch (drawDirection){ //our drawing order depends on the direction of motion
		case 0: //in x direction
		case 2:
			for (int z=0;z< map->height;z++)
				for (int y=0;y< map->width;y++)
					for (int x=0;x<map->length;x++){
						drawblock(x,y,z,s);
					}
			break;
		case 1: //in y direction
		case 3:
			for (int z=0;z< map->height;z++) 
				for (int x=0;x<map->length;x++)
					for (int y=0;y< map->width;y++){
						drawblock(x,y,z,s);
					}
			break;
		case 4: //in z direction
		case 5:
			for (int x=0;x<map->length;x++) {
				for (int y=0;y< map->width;y++) {
					for (int z=0;z< map->height;z++) {
                        if(state == SELECTING_DESTINATION && 
                            arrowLength && 
                            underMouseBlock.x == x && underMouseBlock.y == y && underMouseBlock.z == z)
                        {
                            blitClippedSurface(arrowImg, 0, arrowImg->h - arrowLength, arrowImg->w, arrowLength, 
                                          s, screen_x[y][x] , screen_y[z-1][y][x] + TILESIZE/4);
                        }
						drawblock(x,y,z,s);
					}
                }
            }
			break;
	}
}

void Blockenspiel::drawblock(int x, int y, int z, SDL_Surface *s){
    //cout <<"drawblock"<<x<<y<<z<<" "<< (int)map->getData(x,y,z)<<endl;
    char block = map->getData(x,y,z); 
	if (block!=0){
        int drawX, drawY;
		//draw the block itself
		if (state == MOVING_BLOCK && movingBlock.x==x && movingBlock.y==y && movingBlock.z==z){
            drawX = movePosX;
            drawY = movePosY; 
		} else {
            drawX = screen_x[y][x];
            drawY = screen_y[z][y][x]; 
		}

        
        blitSurface(blockImgs[block-1], s, drawX, drawY);
        //cout << map->nearestBlockCoordSum << " "<< map->nearestBlockCoordSum*4 << " " << map->nearestBlockCoordSum*4 - (x+y+z)*4 << endl;
        if(map->nearestBlockCoordSum > x+y+z){
		    SDL_SetAlpha(blockDarkenImgs[block-1], SDL_SRCALPHA, (map->nearestBlockCoordSum - (x+y+z))*10);
		    blitSurface(blockDarkenImgs[block-1],s,drawX,drawY);
        }

		//draw white-outing blocks
		if (state == REMOVING_BLOCKS && removeGroup->getData(x,y,z) != 0){
            SDL_SetAlpha(blockWhiteImg, SDL_SRCALPHA, disappearingBlockAlpha);
			blitSurface(blockWhiteImg,s,screen_x[y][x] ,screen_y[z][y][x]);
		}
			
		//draw block selector
		if ((state == SELECTING_BLOCK && underMouseBlock.x == x && underMouseBlock.y == y  && underMouseBlock.z == z)||
			(state == SELECTING_DESTINATION && selectedBlock.x == x && selectedBlock.y == y  && selectedBlock.z == z))
        { 
            SDL_SetAlpha(selectorImg, SDL_SRCALPHA, 128);
            blitSurface(selectorImg,s,screen_x[y][x] ,screen_y[z][y][x]);
		}
		
	}			
		
    if (state == SELECTING_DESTINATION){
		if ( selectedBlock.z == z && litUpBlocks[x][y]==true){
            SDL_SetAlpha(highlightImg, SDL_SRCALPHA, 128);
            blitSurface(highlightImg,s,screen_x[y][x] ,screen_y[z][y][x]);
		}
	}
}

bool Blockenspiel::handleMouseMotion(int x, int y){
    if(state == FADE_IN || state == FADE_OUT){
        return false;
    }

    if(screen == LEVEL_SELECT) {
        int scr_y = y - logoImg->h + levsel_scrollOffset;
        int last_levsel_underMouseThumb = levsel_underMouseThumb;
        if(scr_y > 0) {
            levsel_underMouseThumb = (scr_y / LEVSEL_THUMB_H) * LEVSEL_THUMBS_PER_ROW + (x / LEVSEL_THUMB_W);
            if(levsel_underMouseThumb > numLevels) {
                levsel_underMouseThumb = -1;
            }
        } else {
            levsel_underMouseThumb = -1;
        }

        bool inScrollZone = y < 40 || screenImg->h-y < 40; 
        if(state == LEVSEL_MOUSE_NOTINSCROLLZONE && inScrollZone){
            kickAnimationThread(); 
        }
        state = (inScrollZone)? LEVSEL_MOUSE_INSCROLLZONE : LEVSEL_MOUSE_NOTINSCROLLZONE;

        return last_levsel_underMouseThumb != levsel_underMouseThumb;
    } else if(screen == LEVEL) {
        BlockCoords lastUnderMouseBlock = underMouseBlock;
	    switch (state){
            case SELECTING_BLOCK:
			    selectBlock(x, y, &underMouseBlock);
    			break;
            case SELECTING_DESTINATION:
		    	selectDestinationBlock(x, y, &underMouseBlock);
                if(lastUnderMouseBlock != underMouseBlock)
                {
                    //block under the mouse changed, recalculate params for animating the arrow
                    arrowLength = 0;
                    arrowDestLength = 0;
                    for(int z = underMouseBlock.z-1; z>0 && map->getData(underMouseBlock.x, underMouseBlock.y, z) == 0; z--){
                        arrowDestLength += TILESIZE/2;
                    }
                    if(arrowDestLength > 0){
                        //new block has space under it: kick animation thread to draw downward arrow
                        kickAnimationThread();
                    }
                }
                break;				
    	}

        //see if we are over a menu item
        int mouseWRTmenuitemX, mouseWRTmenuitemY;
        int lasthighlightedmenuitem = highlightedmenuitem;
	    highlightedmenuitem = 0;
    	for (int i=0; i<NUM_MENU_BUTTONS; i++){
	    	mouseWRTmenuitemX = x - (menu_x + menuitem_x[i]);
		    mouseWRTmenuitemY = y - (menu_y + menuitem_y[i]);
    		if (mouseWRTmenuitemX >=0 && mouseWRTmenuitemX< TILESIZE && 
	    		mouseWRTmenuitemY >=0 && mouseWRTmenuitemY < TILESIZE){
		    	if (!isPixelTransparent(blockImgs[0], mouseWRTmenuitemX, mouseWRTmenuitemY)){
			    	highlightedmenuitem = i+1;
    			}
	    	}
	    }

        return (lastUnderMouseBlock != underMouseBlock)  || (lasthighlightedmenuitem != highlightedmenuitem);
    }
}

void Blockenspiel::handleMouseClick(int x, int y){
    if(state == FADE_IN || state == FADE_OUT){
        return;
    }

    if(screen == LEVEL_SELECT) {
        if(levsel_underMouseThumb != -1){
            //fade to the level screen
            state = FADE_OUT; 
            levelAfterFade = levsel_underMouseThumb;
            kickAnimationThread();
        }
    } else if(screen == LEVEL) {
        switch (highlightedmenuitem){
		    case 1:  //reset level
			    runLevel();
                state = SELECTING_BLOCK;
			    return;
		    case 2:  //undo move
                if(!movestack.empty()){
			        map = movestack.top();
                    movestack.pop();
	    		    state = SELECTING_BLOCK;
		    	    isMoveDestinationMet = true; 
                }
	    		return;
		    case 3:  //prev level
			    if (level==0) break;
			    state = FADE_OUT; 
                levelAfterFade = level - 1;
                kickAnimationThread();
		    	return;
    		case 4:  //next level
                if (level==numLevels-1) break;
			    state = FADE_OUT; 
                levelAfterFade = level + 1;
                kickAnimationThread();
    			return;
            case 5:  //menu
                state = FADE_OUT; 
                levelAfterFade = -1;
                kickAnimationThread();
		        return;
    	}

        switch (state){
            case SELECTING_BLOCK:
		        selectBlock(x, y, &selectedBlock); 
                if (selectedBlock.x !=-1){
				    state = SELECTING_DESTINATION;
				    for (x=0; x<map->length; x++)   //clear old lit up blocks
					    for (y=0; y<map->width; y++)
						    litUpBlocks[x][y]=false;

                 //cout << "selectedBlock = " << selectedBlock.x << selectedBlock.y << selectedBlock.z << ". SELECTING_DESTINATION" << endl;
                    if(level==0 && turorialstate == TUTORIAL_WELCOME) turorialstate = TUTORIAL_CLICKDEST;
			    }
            break;
            case SELECTING_DESTINATION:
	            selectDestinationBlock(x, y, &destinationBlock);
                
                //whether or not this is a valid destination, get rid of the arrow.
                arrowLength = 0;
                arrowDestLength = 0; 

                if (destinationBlock.x != -1){
				    //we are moving to this block
                    state = MOVING_BLOCK;
	                movingBlock = selectedBlock;
                    movePosX=screen_x[movingBlock.y][movingBlock.x];    //ins screen coords
			        movePosY=screen_y[movingBlock.z][movingBlock.y][movingBlock.x];

		    		drawDirection = motionDirection = selectedBlockMotionDirection;
                    //destination is 1 block away
                    moveDest.x = movingBlock.x + block_dx[motionDirection];
    				moveDest.y = movingBlock.y + block_dy[motionDirection];
	    			moveDest.z = movingBlock.z + block_dz[motionDirection];
		    		isMoveDestinationMet = false;

	    			//add copy of current Map to movestack
    				movestack.push(new LevelMap(*map)); 
                    
                    kickAnimationThread();
		    	} else {
                    if(level==0 && (turorialstate == TUTORIAL_CLICKDEST || turorialstate == TUTORIAL_CLICKDEST_RETRY)) {
                        //don't let then fail!
                        turorialstate = TUTORIAL_CLICKDEST_RETRY; 
                        break;
                    }
				    //no dest block selected
			        state = SELECTING_BLOCK;
			    }
    			break;

	    	case MSG_LEVELFINISHED:
			    state = FADE_OUT; 
                levelAfterFade = level + 1;
                if(levelAfterFade == numLevels){
                    levelAfterFade = -1; //back to level select screen
                }
                kickAnimationThread();
                break;
    		case MSG_LEVELFAILED:
                delete map;
		    	map = new LevelMap(*levelMaps[level]);
			    state = SELECTING_BLOCK;	
	    		break;
    	}
    }
}

void Blockenspiel::selectBlock(int mx, int my, BlockCoords *ret){
	int mouseWRTblockX, mouseWRTblockY;
	//loop through blocks in reverse of drawing order
	for (int z=map->height-1; z>=0; z--){
		for (int x=map->length-1; x>=0; x--){
			for (int y=map->width-1; y>=0; y--){
				if (LevelMap::isBlockMovable(map->getData(x,y,z))){
					mouseWRTblockX = mx - screen_x[y][x];
					mouseWRTblockY = my - screen_y[z][y][x];
					if (mouseWRTblockX >= 0 && mouseWRTblockX < TILESIZE &&
						mouseWRTblockY >= 0 && mouseWRTblockY < TILESIZE &&
						!isPixelTransparent(blockImgs[0], mouseWRTblockX, mouseWRTblockY)){
							//map.data[z][y][x]=0;
							ret->x=x; ret->y=y; ret->z=z;
							//we've got our block! breakout of loops
							return;
					}
				}
            }
        }
    }
	//no block under these coords
    ret->x=-1;
	return;
}

//this is like selectBlock except it only selects blocks which are on 
//the same X or Y axis as the current block,
//and have a clear, supported path between them.
void Blockenspiel::selectDestinationBlock(int mx, int my, BlockCoords *ret){
	int mouseWRTblockX, mouseWRTblockY;	
	
	for (int dir=0; dir<4; dir++){ //iterate through 4 directions
        bool path_supported = true;
        for (int offset=1; path_supported; offset++){
            int x = selectedBlock.x+block_dx[dir]*offset;
            int y = selectedBlock.y+block_dy[dir]*offset;
            //cout << "xy " <<x<<y<<endl; 
            if( x<0 || y<0 || x>map->length-1 || y>map->width-1) {
                //we've moved out of map bounds. move on to the next direction.
                break;
            }
            if(map->getData(x,y,selectedBlock.z) !=0 || map->isBlockAboveVoid(selectedBlock.z,y,x)) {
                //the space is occupied or above a void. Move on to the next direction.
                break;
            }
            //if there is nothing under this spot, this is the last spot we can travel in this direction.
            path_supported = map->getData(x, y, selectedBlock.z-1) != 0;

		    //if we get here, the current block is a valid destination
		    mouseWRTblockX = mx - screen_x[y][x];
		    mouseWRTblockY = my - screen_y[selectedBlock.z][y][x];
		    if (mouseWRTblockX >= 0 && mouseWRTblockX < TILESIZE &&
			    mouseWRTblockY >= 0 && mouseWRTblockY < TILESIZE &&
			    !isPixelTransparent(blockImgs[0], mouseWRTblockX, mouseWRTblockY))
            {
			    ret->x=x; ret->y=y; ret->z=selectedBlock.z;
			    selectedBlockMotionDirection = dir;
			    //clear old lit up blocks
			    for (x=0; x<map->length; x++)
				    for (y=0; y<map->width; y++)
					    litUpBlocks[x][y]=false;
    			//set new lit up blocks
	    		x=selectedBlock.x; y=selectedBlock.y;
		    	for (;offset>0;offset--){
                    x = selectedBlock.x+block_dx[dir]*offset;
                    y = selectedBlock.y+block_dy[dir]*offset;
				    litUpBlocks[x][y]=true;
			    }
			    return;
		    }
	    }
    }
    //we went in every direction, but found no paths under the mouse
    memset(litUpBlocks,0, sizeof(litUpBlocks));
    ret->x = -1;       //no valid block under these coords	
}

int Blockenspiel::animationThreadEntry(void *data) {
    return ((Blockenspiel *)data)->animationThreadFcn();
}

int Blockenspiel::animationThreadFcn() { 
	while (true){
        SDL_mutexP( animateCVLock );
        while(!animationRequest) {
            SDL_CondWait( animateCV, animateCVLock );
        }
        animationRequest = false; //signal that we've received the request and are processing.
        SDL_mutexV( animateCVLock );

        SDL_mutexP( stateLock );
        if (state == FADE_OUT){ 
            for(fadeAlpha=0;fadeAlpha<256;fadeAlpha+=8){
                kickPaintThread(); 
                SDL_mutexV( stateLock ); //unlock mutex while we sleep
                SDL_Delay(1000/60);
                SDL_mutexP( stateLock );           
            }
            //fade done, transition to Fade in
            if(levelAfterFade != -1){
                level = levelAfterFade;
                runLevel();
            } else {
                screen = LEVEL_SELECT;
            }
            state = FADE_IN;
            animationRequest = true;
        } else if (state == FADE_IN){
            for(fadeAlpha=255;fadeAlpha>=0;fadeAlpha-=8){
                kickPaintThread();
                SDL_mutexV( stateLock ); //unlock mutex while we sleep
                SDL_Delay(1000/60);
                SDL_mutexP( stateLock );           
            }
            //fade done
            if(levelAfterFade != -1){
                state = SELECTING_BLOCK; //transition to main LEVEL screen
            } else {
                state = LEVSEL_MOUSE_UNKNOWN; //transition to LEVEL_SELECT screen
            }
            kickPaintThread();
        } else if (screen == LEVEL_SELECT){
            static int maxScroll = logoImg->h + ((numLevels + 3) / 4) * LEVSEL_THUMB_H - screenImg->h;
            int x,y=0;
            while(state == LEVSEL_MOUSE_INSCROLLZONE) {
                SDL_GetMouseState(&x,&y);
                if(y < 40) {
                   levsel_scrollOffset -= (40-y) / 4;
                   if(levsel_scrollOffset < 0) levsel_scrollOffset = 0;
                } else if (screenImg->h - y < 40) {
                   levsel_scrollOffset += (40-(screenImg->h - y)) / 4;
                   if(levsel_scrollOffset > maxScroll) levsel_scrollOffset = maxScroll;        
                }
                kickPaintThread(); 
                SDL_mutexV( stateLock ); //unlock mutex while we sleep
                SDL_Delay(1000/60);
                SDL_mutexP( stateLock );
            }
        } else if (state == SELECTING_DESTINATION){
            //animate downward arrow
            drawDirection = 4; //arrow moves in the z direction 
            //cout << "animate arrow to length " << maxLength << endl;
            while(underMouseBlock.z > 0 && map->getData(underMouseBlock.x, underMouseBlock.y, underMouseBlock.z - 1) == 0 && arrowLength<arrowDestLength) {
                arrowLength+=2;
                kickPaintThread();
                SDL_mutexV( stateLock ); //unlock mutex while we sleep
				SDL_Delay(20); 
                SDL_mutexP( stateLock );
            }
        } else if (state == MOVING_BLOCK) {  
	        const int screen_dx[] = {2,-2,-2,2,0};
	        const int screen_dy[] = {1,1,-1,-1,2};
			int moveDestX=screen_x[moveDest.y][moveDest.x];   //where are we going?
			int moveDestY=screen_y[moveDest.z][moveDest.y][moveDest.x];
			int moveDeltaX = screen_dx[motionDirection]*2;   	//how do we get there?
			int moveDeltaY = screen_dy[motionDirection]*2;	
			while (state == MOVING_BLOCK &&	(movePosX != moveDestX || movePosY != moveDestY)){
				movePosX+=moveDeltaX;
				movePosY+=moveDeltaY;
                kickPaintThread();
                SDL_mutexV( stateLock ); //unlock mutex while we sleep
				SDL_Delay(20);
                SDL_mutexP( stateLock );
			}
            moveFinished();
		} else if (state == REMOVING_BLOCKS) {
			disappearingBlockAlpha=0;
			while (state == REMOVING_BLOCKS && disappearingBlockAlpha < 255*0.8){    //fade in the block
				kickPaintThread();
				disappearingBlockAlpha+=5;
                SDL_mutexV( stateLock ); //unlock mutex while we sleep
		    	SDL_Delay(20);
                SDL_mutexP( stateLock );
			}
            moveFinished();
		}
        SDL_mutexV( stateLock ); //unlock mutex
	}
}

void Blockenspiel::kickPaintThread(){
    SDL_mutexP( repaintCVLock );
    repaintRequest = true; 
    SDL_CondSignal( repaintCV );
    SDL_mutexV( repaintCVLock );
}

void Blockenspiel::kickAnimationThread(){
    SDL_mutexP( animateCVLock );
    animationRequest = true; 
    SDL_CondSignal( animateCV );
    SDL_mutexV( animateCVLock );
}

void Blockenspiel::moveFinished(){
	bool preventfallthroughflag = false;
		
	if (state == MOVING_BLOCK){
        //cout << "moved " << movingBlock.x << movingBlock.y<< movingBlock.z << " to " << moveDest.x << moveDest.y<< moveDest.z << endl;
		//update map
		*map->getDataPtr(moveDest) = map->getData(movingBlock);
		*map->getDataPtr(movingBlock) = 0;
			
		//update selectedBlock, since it just finished moving
		if (movingBlock == selectedBlock){
			selectedBlock = moveDest;
			if (selectedBlock == destinationBlock){
				isMoveDestinationMet = true;
                if(level==0 && (turorialstate == TUTORIAL_CLICKDEST || turorialstate == TUTORIAL_CLICKDEST_RETRY)) turorialstate = TUTORIAL_MOVETONEIGHTBOUR;
            }
		}
			
		//is this block about to fall past a clump? 
		//we need to stop it from falling through!
		if (map->getData(moveDest.x, moveDest.y, moveDest.z-1) == 0 &&                //block that just moved will fall
			map->isBlockTouchingIdenticalBlock(moveDest))                        //past another block it is clumped with
		{
			preventfallthroughflag = true;
		}
	} else if (state==REMOVING_BLOCKS){
		//We've just finished removing blocks. update the map:
		for (int x=0;x<map->length;x++)
			for (int y=0;y< map->width;y++)
				for (int z=0;z< map->height;z++) 
					if (removeGroup->getData(x,y,z)!=0)
						*map->getDataPtr(x,y,z) = 0;
 
        if(level==0 && turorialstate == TUTORIAL_MOVETONEIGHTBOUR) turorialstate = TUTORIAL_CLEARTHEREST;
		//block disappeared.
        kickPaintThread();
	}

	BlockCoords unsupportedBlock, clumpedBlock;
    //check for unsupported blocks
	if (preventfallthroughflag == false && map->getUnsupportedBlock(unsupportedBlock)){
		state = MOVING_BLOCK;
		movingBlock = unsupportedBlock;
        movePosX=screen_x[movingBlock.y][movingBlock.x];    //in screen coords
		movePosY=screen_y[movingBlock.z][movingBlock.y][movingBlock.x];
		moveDest = unsupportedBlock;
		moveDest.z--;
		drawDirection = motionDirection = 4; //gravity
        kickAnimationThread();
	} else if (map->getClumpedBlock(clumpedBlock)) {		//check map for clumps of touching blocks
		//the block that just moved is up for removal
		state = REMOVING_BLOCKS;
        //find all touching blocks

        memset(removeGroup->data, 0, sizeof(char) * removeGroup->length * removeGroup->width * removeGroup->height);  
		map->getBlockGroup(clumpedBlock, *removeGroup);
		//is selectedBlock about to be removed?
		if (removeGroup->getData(selectedBlock) != 0) {
			isMoveDestinationMet = true;
        }
        kickAnimationThread();
	} else {
		if (isMoveDestinationMet){
			state = SELECTING_BLOCK;
			underMouseBlock.x = underMouseBlock.y = underMouseBlock.z = -1;
			switch (map->checkCompletion()){
				case 0:
                    //repaint with new shading
                    kickPaintThread();
					break;
				case 1:
					state = MSG_LEVELFINISHED;
                    updateProgressFile(level);
					kickPaintThread();
					break;
				case 2:
					state = MSG_LEVELFAILED;
					kickPaintThread();
					break;	
			}	
		} else {
            //continue with the move
			state = MOVING_BLOCK;
			movingBlock = selectedBlock;
            movePosX=screen_x[movingBlock.y][movingBlock.x];    //in screen coords
		    movePosY=screen_y[movingBlock.z][movingBlock.y][movingBlock.x];
			drawDirection = motionDirection = selectedBlockMotionDirection;
			moveDest.x = movingBlock.x + block_dx[motionDirection];
			moveDest.y = movingBlock.y + block_dy[motionDirection];
			moveDest.z = movingBlock.z + block_dz[motionDirection]; 
            kickAnimationThread();
		}			
	}	
}

void Blockenspiel::updateProgressFile(int levelcompleted){
    time_t t = time(0);
    struct tm *now = localtime( &t );
    cout <<"now " << (now->tm_year + 1900) << '-' 
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday
         << endl;
    finishedlevels[levelcompleted].isCompleted = true;
    finishedlevels[levelcompleted].completionTime = *localtime( &t );

    ofstream pf ("progress.dat",ios::binary);
    if (pf.is_open()){
        char buf[4];
        for(int i=0; i < numLevels; i++){ 
            pf.write((char *)(&finishedlevels[i]),sizeof(LevelProgress));
        }
    }
    pf.close();
}

inline bool BlockCoords::operator==(const BlockCoords& other){
    return x == other.x && y == other.y && z == other.z;
}
inline bool BlockCoords::operator!=(const BlockCoords& other){
    return !(*this == other);
}
