#include <fstream>
#include <cstring>
#include "blockedin.hpp"

LevelMap::LevelMap (int l, int w, int h):length(l),width(w),height(h),nearestBlockCoordSum(0){
	data = new char[length * width * height];
	for (int z=0; z<height; z++){
        for (int y=0; y<width; y++){
		    for (int x=0; x<length; x++){
                *getDataPtr(x,y,z) = 0;
            }
        }
    }
}

LevelMap::LevelMap (ifstream &mapfile, string name) : name(name), nearestBlockCoordSum(0){
    mapfile >> length >> width >> height;
    lprint << "Loaded level " << name << " lwh:" << length << width << height << endl;

    data = new char[length * width * height];
    for (int z=0; z<height; z++){
        for (int y=0; y<width; y++){
		    for (int x=0; x<length; x++){
                int temp;
				mapfile >> temp;  
                *getDataPtr(x,y,z) = (char) temp;
                if( temp && nearestBlockCoordSum < x+y+z) nearestBlockCoordSum = x+y+z;
                //lprint << (int) *getDataPtr(x,y,z) << " ";
            }
            //lprint << endl;
        }
        //lprint << endl;
    }
}

LevelMap::LevelMap(const LevelMap &lm){
	length=lm.length; width=lm.width; height=lm.height; name=lm.name; nearestBlockCoordSum = lm.nearestBlockCoordSum;
	data = new char[length * width * height];
    memcpy(data, lm.data, length * width * height);
}

LevelMap::~LevelMap(){
	delete [] data;
}

bool LevelMap::isBlockMovable(int blockType){
	if (blockType>=FIRST_MOVABLE_BLOCKTYPE)
		return true;
	return false;
}
	
bool LevelMap::isBlockSupportive(int blockType){
	if (blockType <8 || blockType>19)
		return true;
	return false;
}

/* Check if the block is above a void.
 * ie: all squares below it are empty */ 
bool LevelMap::isBlockAboveVoid(int z, int y, int x){
	for(int i=z; i>=0; i--){
		if (getData(x,y,i)!=0){
			if (isBlockSupportive(getData(x,y,i)))
				return false;
			else
				return true;
		}
	}
	return true;
}

//TODO: return object instead of pointer?
bool LevelMap::getUnsupportedBlock(BlockCoords &coords){
	for (int z=1;z< height;z++) {
		for (int x=0;x<length;x++) {
			for (int y=0;y< width;y++) {
				if (isBlockMovable(getData(x,y,z)) && getData(x,y,z-1)==0) {
                    coords.x = x; coords.y = y; coords.z = z;
                    return true;
                }
            }
        }
    }
	return false;
}

bool LevelMap::getClumpedBlock(BlockCoords &coords){
	BlockCoords bc;
	for (bc.z=0;bc.z< height;bc.z++) {
		for (bc.x=0;bc.x<length;bc.x++) {
			for (bc.y=0;bc.y< width;bc.y++) {
				if (isBlockMovable(getData(bc)) && isBlockTouchingIdenticalBlock(bc)) {
                    coords = bc;
                    return true;
                }
            }
        }
    }
	return false;
}
	
bool LevelMap::isBlockTouchingIdenticalBlock(BlockCoords &bc){
	char blockType = getData(bc.x,bc.y, bc.z);
	if (blockType==0)
		return false;
	if (getData(bc.x+1,bc.y  , bc.z)==blockType ||
		getData(bc.x-1,bc.y  , bc.z)==blockType ||
		getData(bc.x  ,bc.y+1, bc.z)==blockType ||
		getData(bc.x  ,bc.y-1, bc.z)==blockType ||
		getData(bc.x  ,bc.y  , bc.z+1)==blockType ||
		getData(bc.x  ,bc.y  , bc.z-1)==blockType)
		return true;
	return false;
}
	
bool LevelMap::getBlockGroup(BlockCoords &bc, LevelMap &group){
	char blockType = getData(bc);
	if (blockType==0)
		return false;
	growBlockGroup(blockType, bc.x, bc.y, bc.z, group);
    return true;
}
	
void LevelMap::growBlockGroup(char blockType, int x, int y, int z, LevelMap &group) {
	if (group.getData(x,y,z)!=0)
		return; //this block already visited
	if (blockType == getData(x,y,z)){
		*group.getDataPtr(x,y,z) = blockType;
		growBlockGroup(blockType,x+1,y,z,group);
		growBlockGroup(blockType,x-1,y,z,group);
		growBlockGroup(blockType,x,y+1,z,group);
		growBlockGroup(blockType,x,y-1,z,group);
		growBlockGroup(blockType,x,y,z+1,group);
		growBlockGroup(blockType,x,y,z-1,group);
	}
}
	
int LevelMap::checkCompletion(){
	const int numMovableBlocks = NUM_BLOCK_TYPES - FIRST_MOVABLE_BLOCKTYPE + 1;
    int blocksCount[numMovableBlocks] = {0};

	bool isFinished=true;
	for (int z=0; z<height; z++)
		for (int x=0; x<length; x++)
			for (int y=0; y<width; y++)
				if (isBlockMovable(getData(x,y,z))){
					blocksCount[getData(x,y,z)-FIRST_MOVABLE_BLOCKTYPE]++;
					isFinished=false;
				}
	if (isFinished) return 1;
	//check for single blocks
	for (int i=0; i<numMovableBlocks; i++)
		if (blocksCount[i]==1){
			return 2;
		}
	return 0;
}
	
char* LevelMap::getDataPtr(int x, int y, int z){
	if (x<0 || y<0 || z<0 ||
		x>=length ||y>=width ||z>=height)
		return NULL;
	return &data[(z * length * width) + (y * length) + x];
}

char LevelMap::getData(int x, int y, int z){
    char *blockptr = getDataPtr(x,y,z);
	return (blockptr) ? *blockptr : 0;
}

char* LevelMap::getDataPtr(BlockCoords &bc){
    return getDataPtr(bc.x, bc.y, bc.z);
}

char LevelMap::getData(BlockCoords &bc){
    return getData(bc.x, bc.y, bc.z);
}

