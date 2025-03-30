#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
using namespace std;
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

// Logger class for handling log output
class Logger {
public:
    ofstream logFile;
    
    Logger() {
        logFile.open("log.txt", ios::out | ios::trunc);
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    template <typename T>
    Logger& operator<<(const T& message) {
        logFile << message;
        return *this;
    }
    
    Logger& operator<<(ostream& (*manip)(ostream&)) {
        manip(logFile);
        return *this;
    }
};

// Global logger instance
extern Logger gLogger;

// Define a macro for logging - with endl support
#define lprint gLogger
#define endl std::endl

#define TILESIZE 64
#define NUM_MENU_BUTTONS 5
#define NUM_BLOCK_TYPES 50
#define NUM_LEVELS 5

typedef enum Screen_e { 
    LEVEL_SELECT,
    LEVEL,
} Screen;

typedef enum State_e {
    //states that define mouse position in the LEVEL_SELECT screen
    LEVSEL_MOUSE_UNKNOWN,
    LEVSEL_MOUSE_INSCROLLZONE,
    LEVSEL_MOUSE_NOTINSCROLLZONE,
    //states for fading between screens
    FADE_OUT,
    FADE_IN,
    //states for the LEVEL screen
    SELECTING_BLOCK, 
    SELECTING_DESTINATION, 
    MOVING_BLOCK, 
    REMOVING_BLOCKS,
    MSG_LEVELFINISHED,
    MSG_LEVELFAILED
} State;

typedef enum TutorialState_e {
    TUTORIAL_WELCOME,
    TUTORIAL_CLICKDEST,
    TUTORIAL_CLICKDEST_RETRY,
    TUTORIAL_MOVETONEIGHTBOUR,
    TUTORIAL_CLEARTHEREST
} TutorialState;

//indexed by selectedBlockMotionDirection: forward in x, forward in y, backward in x, backward in y, down in z
const int block_dx[] = {1,0,-1,0,0};
const int block_dy[] = {0,1,0,-1,0};
const int block_dz[] = {0,0,0,0,-1};

class BlockCoords{
    public:
    int x,y,z;
	BlockCoords(): x(-1), y(-1), z(-1){}
	BlockCoords(int x, int y, int z): x(x), y(y), z(z){}

    inline bool operator==(const BlockCoords& other);
    inline bool operator!=(const BlockCoords& other); 
};

//we store an array of these in a file to track the user's progress
typedef struct _LevelProgress{
    bool isCompleted;
    struct tm completionTime;
} LevelProgress;

#define FIRST_MOVABLE_BLOCKTYPE 33

#define LEVSEL_THUMBS_PER_ROW 4
#define LEVSEL_THUMB_W (600/4)
#define LEVSEL_THUMB_H (600/4)

class LevelMap{ 
    public:
   	LevelMap (int l, int w, int h);

    LevelMap (ifstream &mapfile, string name);
	LevelMap(const LevelMap &lm);
	~LevelMap();

    string name;
	int length, width, height;
    char *data;
    int nearestBlockCoordSum;

	static bool isBlockMovable(int blockType);
	static bool isBlockSupportive(int blockType);

	bool isBlockAboveVoid(int z, int y, int x);
	bool getUnsupportedBlock(BlockCoords &coords);
	bool getClumpedBlock(BlockCoords &coords);
	bool isBlockTouchingIdenticalBlock(BlockCoords &bc);
	bool getBlockGroup(BlockCoords &bc, LevelMap &group);
	int checkCompletion();
	
	char getData(int x, int y, int z);
	char* getDataPtr(int x, int y, int z);
    char* getDataPtr(BlockCoords &bc);
    char getData(BlockCoords &bc);

    private:
    void growBlockGroup(char blockType, int x, int y, int z, LevelMap &tempmap);
};

class Blockenspiel {
    public: 
    Blockenspiel(): numLevels(0), 
        screen(LEVEL_SELECT),
        state(LEVSEL_MOUSE_UNKNOWN),
        levsel_underMouseThumb(-1), 
        levsel_scrollOffset(0), 
        map(NULL), 
        drawDirection(0),
        arrowLength(0),
        repaintRequest(false),
        animationRequest(false),
        highlightedmenuitem(0),
        window(NULL),
        renderer(NULL) {};
        
	int originX, originY;
    TTF_Font *font;	

	//state variables
    Screen screen;
	State state;
    
    // SDL2 specific objects
    SDL_Window *window;
    SDL_Renderer *renderer;

    //variables for level select screen
    SDL_Surface *logoImg;
    vector<SDL_Surface *> levsel_levelThumbs;
    int levsel_underMouseThumb;
    int levsel_scrollOffset;
    vector<LevelProgress> finishedlevels;

    //variables for fades
    SDL_Surface *fadeImg; 
    int fadeAlpha;
    int levelAfterFade;

    //variables for game screen
	BlockCoords underMouseBlock,   //mouse is hovering here - highlight it
                selectedBlock,     //block selected with first click
                destinationBlock,  //block selected with 2nd click
                movingBlock,       //block which is currently moving
                moveDest;          //where it is moving to
	bool isMoveDestinationMet;
	int drawDirection, motionDirection, selectedBlockMotionDirection;
    int arrowLength, arrowDestLength;
	bool litUpBlocks[10][10]; 
	LevelMap *removeGroup; //blocks which are currently being removed
	int level;
	int highlightedmenuitem;
    TutorialState turorialstate; 

	//screen coords of menu and it's items
	int menu_x, menu_y;
	static const int menuitem_x[5];
	static const int menuitem_y[5];
	static string menuItemDescriptions[5];

	//for moves
	int movePosX;
	int movePosY;
	float disappearingBlockAlpha;
	
    //screen coordinates of each block
	int screen_x[10][10];
	int screen_y[10][10][10];
	
	//images
    SDL_Surface *screenImg;

    SDL_Surface *texMapImg;
	SDL_Surface *blockImgs[NUM_BLOCK_TYPES];
    SDL_Surface *blockDarkenImgs[NUM_BLOCK_TYPES];
	SDL_Surface *selectorImg, *highlightImg, *blockWhiteImg;
	SDL_Surface *InGameMenu;
	SDL_Surface *backgroundImg;
    SDL_Surface *arrowImg;

    int numLevels;
    vector<LevelMap *> levelMaps;
    LevelMap *map;
	stack<LevelMap *> movestack;

    SDL_Thread *animationThread, *paintThread;
    bool repaintRequest, animationRequest;    // These flags are set by the calling thread to request a repaint/animation
    SDL_cond *repaintCV, *animateCV;          // Condvar signals thesee flags
    SDL_mutex *repaintCVLock, *animateCVLock; // mutex protects them
    SDL_mutex *stateLock;

    void init();
    void run();

private:
    void runLevel();

    static int paintThreadEntry(void *data);
    int paintThreadFcn();
    void kickPaintThread();
	void paint(SDL_Surface *s);
    void drawMap(SDL_Surface *s);
    void drawblock(int x, int y, int z, SDL_Surface *s);

    static int animationThreadEntry(void *data);
    int animationThreadFcn();
    void kickAnimationThread();

    bool handleMouseMotion(int x, int y);
    void handleMouseClick(int x, int y);

    //find the block under mouse coords
    void selectBlock(int mx, int my, BlockCoords *ret);
    void selectDestinationBlock(int mx, int my, BlockCoords *ret);

    //called by the animation thread when move has finished.
    void moveFinished();
    void updateProgressFile(int levelcompleted);
};

//prototypes for SDL wrapper functions
void dumpsurf(SDL_Surface *s);
SDL_Surface *getImage( std::string filename );
SDL_Surface *createSurface(int w, int h);
void blitSurface(SDL_Surface *src, SDL_Surface *dst, int x, int y);
void blitClippedSurface(SDL_Surface *src, int x1, int y1, int w1, int h1, SDL_Surface *dst, int x2, int y2);
SDL_Surface *getImageFromTexMap(SDL_Surface *tm, int x, int y, int w, int h);
Uint32 *getPixelPtr(SDL_Surface *s, int x, int y);
bool isPixelTransparent(SDL_Surface *s, int x, int y);
void shrinkSurface(SDL_Surface *s, SDL_Surface *d);
void drawText(SDL_Surface *dst, const string &s, TTF_Font *font, int x, int y);
void drawTextCentered(SDL_Surface *dst, const string &s, TTF_Font *font, int x, int y);
void drawTextBox(SDL_Surface *dst, const string s[], int numLines, TTF_Font *font, int x, int y);
void drawTextBoxCentered(SDL_Surface *dst, const string s[], int numLines, TTF_Font *font, int x, int y);
