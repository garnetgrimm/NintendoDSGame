#include <nds.h>

#include <ifelya.h>
#include <leaf.h>
#include <background.h>

//---------------------------------------------------------------------
// The ifelya sprite
// he needs a single pointer to sprite memory
// and a reference to his frame graphics so they
// can be loaded as needed
//---------------------------------------------------------------------
typedef struct 
{
	int x;
	int y;

	int frames;
	int size;

	u16* sprite_gfx_mem;
	u8*  frame_gfx;

	int state;
	int anim_frame;
}Sprite;

//---------------------------------------------------------------------
// The state of the sprite (which way it is walking)
//---------------------------------------------------------------------
enum SpriteState {W_RIGHT = 0, W_LEFT_MUNCH = 1, W_RIGHT_MUNCH = 2, W_LEFT = 3};

//---------------------------------------------------------------------
// Screen dimentions
//---------------------------------------------------------------------
enum {SCREEN_TOP = 0, SCREEN_BOTTOM = 192, SCREEN_LEFT = 0, SCREEN_RIGHT = 256};

//---------------------------------------------------------------------
// Animating ifelya requires us to copy in a new frame of data each time
//---------------------------------------------------------------------
void animateSprite(Sprite *sprite)
{
	int frame = sprite->anim_frame + sprite->state * sprite->frames;

	u8* offset = sprite->frame_gfx + frame * sprite->size*sprite->size;

	dmaCopy(offset, sprite->sprite_gfx_mem, sprite->size*sprite->size);
}

//---------------------------------------------------------------------
// Initializing ifelya requires little work, allocate room for one frame
// and set the frame gfx pointer
//---------------------------------------------------------------------
void initSprite(Sprite *sprite, u8* gfx, OamState* screen)
{
	int real_size;

	if(sprite->size == 32) 
		real_size = SpriteSize_32x32;
	else
		real_size = SpriteSize_8x8;

	sprite->sprite_gfx_mem = oamAllocateGfx(screen, real_size, SpriteColorFormat_256Color);
	sprite->frame_gfx = (u8*)gfx;
}


//---------------------------------------------------------------------
// main
//---------------------------------------------------------------------
int main(void) 
{
	Sprite ifelya = {0,160,2,32};
	Sprite leaf = {50,160,1,8};
	//-----------------------------------------------------------------
	// Initialize the graphics engines
	//-----------------------------------------------------------------
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankD(VRAM_D_SUB_SPRITE);

	oamInit(&oamMain, SpriteMapping_1D_128, false);
	oamInit(&oamSub, SpriteMapping_1D_128, false);

	//-----------------------------------------------------------------
	// Initialize the two sprites
	//-----------------------------------------------------------------
	initSprite(&ifelya, (u8*)ifelyaTiles, &oamMain);
	initSprite(&leaf, (u8*)leafTiles, &oamSub);
	
	dmaCopy(ifelyaPal, SPRITE_PALETTE, 512);
	dmaCopy(leafPal, SPRITE_PALETTE_SUB, 512);

	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0,0);	
	dmaCopy(backgroundBitmap, bgGetGfxPtr(bg3), 256*256);
	dmaCopy(backgroundPal, BG_PALETTE, 256*2);
	
	//-----------------------------------------------------------------
	// main loop
	//-----------------------------------------------------------------
	while(1) 
	{
		scanKeys();
		
		int keys = keysHeld();
		
		if(keys & KEY_START) break;

		if(keys)
		{
			if(keys & KEY_LEFT)
			{
				if(ifelya.x >= SCREEN_LEFT) ifelya.x--;
				ifelya.state = W_LEFT;
			}
			if(keys & KEY_RIGHT)
			{
				if(ifelya.x <= SCREEN_RIGHT) ifelya.x++;
				ifelya.state = W_RIGHT;
			}

			if(keys & KEY_A) {
				if(ifelya.state == W_LEFT)
					ifelya.state = W_LEFT_MUNCH;
				if(ifelya.state == W_RIGHT)
					ifelya.state = W_RIGHT_MUNCH;
			}
		
			if(keys & KEY_LEFT || keys & KEY_RIGHT) ifelya.anim_frame++;

			if(ifelya.anim_frame >= ifelya.frames) ifelya.anim_frame = 0;
		} else {
			if(ifelya.state == W_LEFT_MUNCH)
				ifelya.state= W_LEFT;
			if(ifelya.state == W_RIGHT_MUNCH)
				ifelya.state= W_RIGHT;
		}

		animateSprite(&ifelya);
		animateSprite(&leaf);
		//-----------------------------------------------------------------
		// Set oam attributes, notice the only difference is in the sprite 
		// graphics memory pointer argument.  The ifelya only has one pointer
		// while the women has an array of pointers
		//-----------------------------------------------------------------
		oamSet(&oamMain, 0, ifelya.x, ifelya.y, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color, 
			ifelya.sprite_gfx_mem, -1, false, false, false, false, false);
		
		oamSet(&oamSub, 0, leaf.x, leaf.y, 0, 0, SpriteSize_8x8, SpriteColorFormat_256Color, 
			leaf.sprite_gfx_mem, -1, false, false, false, false, false);

		swiWaitForVBlank();

		oamUpdate(&oamMain);
		oamUpdate(&oamSub);
	}

	return 0;
}
