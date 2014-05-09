#include "blockedin.hpp"


/* for debugging*/
void dumpsurf(SDL_Surface *s){
    cout << "dumpsurf " << std::hex << (unsigned) s->flags << " "
            << (unsigned) s->format->BitsPerPixel << " " 
            << (unsigned) s->format->BytesPerPixel << " " << endl;
    if(s->flags & SDL_SRCCOLORKEY){
        cout << " colorkey: " << hex << (unsigned)s->format->colorkey << endl;
    }
    cout << (unsigned) s->format->Rloss << " "
            << (unsigned) s->format->Gloss << " "
            << (unsigned) s->format->Bloss << " "
            << (unsigned) s->format->Aloss << " "<< endl
            << (unsigned) s->format->Rshift << " "
            << (unsigned) s->format->Gshift << " "
            << (unsigned) s->format->Bshift << " "
            << (unsigned) s->format->Ashift << " "<< endl
            << (unsigned) s->format->Rmask << " "
            << (unsigned)  s->format->Gmask << " "
            << (unsigned) s->format->Bmask << " "
            << (unsigned) s->format->Amask << " "
            << endl;
}

SDL_Surface *getImage( std::string filename )
{
    SDL_Surface* loadedImage = NULL;
    SDL_Surface* optimizedImage = NULL;

    loadedImage = IMG_Load( filename.c_str() );


    if( loadedImage != NULL )
    {
        //This creates an image withotu an alpha channel: 
        //It has flags SDL_SRCCOLORKEY|SDL_SRCALPHA, so the transparent pixels are colour keyed, and we can use global alpha as well. 
        //I tried using SDL_DisplayFormatAlpha, but this created an alpha channel for the image, and I wasn't able to use global alpha.
        optimizedImage = SDL_DisplayFormat( loadedImage );
        if(!(optimizedImage->flags & SDL_SRCCOLORKEY)){
            cerr << "ERROR: (" << filename <<") colour key image expected!" << endl;
        }
        //cout << "loaded " << filename << " " << hex << *((Uint32 *)optimizedImage->pixels)<<endl;
        //dumpsurf(optimizedImage);
        SDL_FreeSurface( loadedImage );
    } else {
        //cout << "failed to load " << filename << endl;
        return NULL;
    }
    //Return the optimized image
    return optimizedImage;
}

SDL_Surface *createSurface(int w, int h)
{
    Uint32 rmask, gmask, bmask;
    SDL_Surface *temp, *temp2;
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00; 
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;

#endif

    temp = SDL_CreateRGBSurface(SDL_SRCCOLORKEY, w, h, 32, rmask, gmask, bmask, 0); 
    temp2 = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    return temp2;
}

void blitSurface(SDL_Surface *src, SDL_Surface *dst, int x, int y){
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = dst->w;
    rect.h = dst->h;
    SDL_BlitSurface(src, NULL, dst, &rect);
}

void blitClippedSurface(SDL_Surface *src, int x1, int y1, int w1, int h1, SDL_Surface *dst, int x2, int y2){
    SDL_Rect rect_src, rect_dst;

    rect_src.x = x1;
    rect_src.y = y1;
    rect_src.w = w1;
    rect_src.h = h1;

    rect_dst.x = x2;
    rect_dst.y = y2;
    rect_dst.w = dst->w;
    rect_dst.h = dst->h;
    SDL_BlitSurface(src, &rect_src, dst, &rect_dst);
}

SDL_Surface *getImageFromTexMap(SDL_Surface *tm, int x, int y, int w, int h){
    SDL_Surface *dst = createSurface(w,h); 
    SDL_SetColorKey( dst, SDL_RLEACCEL | SDL_SRCCOLORKEY, 0 );
    blitClippedSurface(tm, x, y, w, h, dst, 0, 0);

    return dst;
}

Uint32 *getPixelPtr(SDL_Surface *s, int x, int y){
    Uint8 *pix8 = (Uint8 *)s->pixels + y * s->pitch + x * s->format->BytesPerPixel;
    return ((Uint32 *)pix8);
}

bool isPixelTransparent(SDL_Surface *s, int x, int y){
    SDL_LockSurface(s);
    Uint32 val = *getPixelPtr(s,x,y);
    SDL_UnlockSurface(s);
    return val == s->format->colorkey;
}

void shrinkSurface(SDL_Surface *s, SDL_Surface *d)
{
    SDL_LockSurface(s);
    SDL_LockSurface(d);
    for(int x=0; x<d->w; x++){
        for(int y=0; y<d->h; y++){
            Uint8 *dst = (Uint8 *)getPixelPtr(d,x,y);
            Uint8 *src = (Uint8 *)getPixelPtr(s,x*4,y*4);
            int chan[4] = {0};
            for(int i=0; i<4; i++){
                for(int j=0; j<4; j++){
                    for(int ch=0; ch<4; ch++){
                         chan[ch] += src[s->pitch*i + j*4+ch];
                    }
                }
            }
            for(int ch=0; ch<4; ch++){
                dst[ch] = chan[ch] / (4*4);
            }
        }
    }
    SDL_UnlockSurface(s);
    SDL_UnlockSurface(d);
}

void drawText(SDL_Surface *dst, const string &s, TTF_Font *font, int x, int y){
    SDL_Color col = {0xFF, 0xFF, 0xFF, 0xFF};
    SDL_Surface * textImg = TTF_RenderText_Blended(font, s.c_str(), col);
    blitSurface(textImg, dst, x,y);
    SDL_FreeSurface(textImg);
}

void drawTextCentered(SDL_Surface *dst, const string &s, TTF_Font *font, int x, int y){
    SDL_Color col = {0xFF, 0xFF, 0xFF, 0xFF};
    SDL_Surface * textImg = TTF_RenderText_Blended(font, s.c_str(), col);
    x-= textImg->w / 2;
    blitSurface(textImg, dst, x,y);
    SDL_FreeSurface(textImg);
}

void drawTextBox(SDL_Surface *dst, const string s[], int numLines, TTF_Font *font, int x, int y){
    SDL_Color col = {0xDD, 0xDD, 0xDD, 0xDD};
    int max_w = 0, pos = y, w, h;
    SDL_Surface **textImgs = new SDL_Surface *[numLines];
    for(int i=0; i<numLines; i++){
        TTF_SizeText(font, s[i].c_str(), &w, &h);
        textImgs[i] = TTF_RenderText_Blended(font, s[i].c_str(), col);
        if(w > max_w) max_w = w;
    }

    SDL_Surface *bg = createSurface(max_w + 8, h*numLines);
    SDL_FillRect(bg, NULL, 0);
    SDL_SetAlpha(bg, SDL_SRCALPHA, 128);
    x -= bg->w/2;
    blitSurface(bg, dst, x, y);
    SDL_FreeSurface(bg);

    for(int i=0; i<numLines; i++){
        if(textImgs[i]){
            blitSurface(textImgs[i], dst, x+4 ,y+h*i);
            SDL_FreeSurface(textImgs[i]);
        }
    }
    delete [] textImgs;
}

void drawTextBoxCentered(SDL_Surface *dst, const string s[], int numLines, TTF_Font *font, int x, int y){
    SDL_Color col = {0xDD, 0xDD, 0xDD, 0xDD};
    int max_w = 0, pos = y, w, h;
    SDL_Surface **textImgs = new SDL_Surface *[numLines];
    for(int i=0; i<numLines; i++){
        TTF_SizeText(font, s[i].c_str(), &w, &h);
        textImgs[i] = TTF_RenderText_Blended(font, s[i].c_str(), col);
        if(w > max_w) max_w = w;
    }

    SDL_Surface *bg = createSurface(max_w + 8, h*numLines);
    SDL_FillRect(bg, NULL, 0);
    SDL_SetAlpha(bg, SDL_SRCALPHA, 128);
    blitSurface(bg, dst, x - bg->w/2, y);
    SDL_FreeSurface(bg);

    for(int i=0; i<numLines; i++){
        if(textImgs[i]){
            blitSurface(textImgs[i], dst, x-textImgs[i]->w/2 ,y+h*i);
            SDL_FreeSurface(textImgs[i]);
        }
    }
    delete [] textImgs;
}
