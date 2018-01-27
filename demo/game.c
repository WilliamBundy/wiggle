/* This is a pretty messy implementation of space invaders, but I hope it gets the 
 * point across as a neat way of showing off some of the rendering.
 * As far as "magic numbers" go, most of them are just constants I found that 
 * worked, except for a few sizes (64 slots in all the arrays, for example), and 
 * things baked into the sprite sheet
 *
 * Sorry the UFO didn't make it in.
 *
 * The general idea for updating (in pseudocode) is:
 *  - check the mode
 *  	- do main menu/read some text stuff
 *		- if the game is over, print a message saying so
 *	- Get input
 *	- loop over all the bullets
 *		- loop over all the blocks
 *			- if they hit, destroy the bullet, damage the block
 *		- if the bullet is going up, loop over all the invaders
 *			- if they hit, destroy both
 *		- if the bullet hits the player, end the game
 *		- draw the bullet
 *	- loop over all the blocks
 *		- increment timer, recover if it ticks out
 *		- draw the block
 *	- count how many invaders are still alive
 *		- if there aren't any, the player wins; end the game
 *	- calculate the rate from remaining invaders
 *	- loop over all the invaders
 *		- draw them, with an effect if they're going fast or low
 *		- move based on the rate
 *		- if they hit the right edge, loop
 *	- if it's time for the invaders to shoot
 *		- figure out how many shots we want
 *		- choose some invaders to shoot, make them shoot
 */
wFontInfo* font;

#define SpritesX 1664
#define SpritesY 505
typedef struct
{
	i32 exists;
	f32 x, y, v;
} Bullet;

typedef struct
{
	i32 exists;
	f32 x, y;
	f32 t, l, w, h;
	char c;
} Invader;

typedef struct
{
	i32 exists;
	i32 hp;
	f32 x, y;
	f32 timer;
} Block;


int collideBox(f32 x1, f32 y1, f32 w1, f32 h1, f32 x2, f32 y2, f32 w2, f32 h2)
{
	if(wabsf(x2 - x1) > (w1 + w2)) return 0;
	if(wabsf(y2 - y1) > (h1 + h2)) return 0;
	return 1;
}

void drawInvader(Invader* a, f32 rate)
{
	f32 jitterX = 0;
	f32 jitterY = 0;
	if(rate > 0.75) {
		u32 r = badRand() % 256;
		jitterX = (f32)r / 256.0f - 0.5f;; 
		r = badRand() % 256;
		jitterY = (f32)r / 256.0f - 0.5f;; 
	}
	wSpriteList s = wDrawText(&g, font, 
			a->x + jitterX*4, a->y + jitterY*4, 
			&a->c, 1,
			32, Anchor_TopLeft, rate > 0.75 ? 0xFF3300FF : 0xFFFFFFFF, 1);
	a->t = s.t;
	a->l = s.l;
	a->w = (s.r - s.l) * 0.5;
	a->h = (s.b - s.t) * 0.5;
	wGroupAddRaw(&g, Anchor_TopLeft | Sprite_NoTexture, 0xFFFFFF33, 
			s.l, s.t, a->w*2, a->h*2,
			0, 0, 0, 0);

}

void drawBlock(f32 x, f32 y, i32 hp)
{
	wGroupAddRaw(&g, 0, hp == 1 ? 0x888888FF : 0xFFFFFFFF,
			x, y, 14, 14,
			SpritesX+32, SpritesY, 16, 16);
}

void drawBullet(f32 x, f32 y, f32 v)
{
	wGroupAddRaw(&g, v > 0 ? Sprite_FlipVert: 0, 0xFFFFFFFF,
			x, y, 14, 14,
			SpritesX+33, SpritesY+17, 14, 14);
}

void drawShip(f32 x, f32 y)
{
	wGroupAddRaw(&g, 0, 0xFFFFFFFF,
			x, y, 30, 30,
			SpritesX+2, SpritesY+2, 28, 28);
}

#define ShotInterval 0.4f
#define AlienInterval 1.6f
#define BlockInterval 2.4f
#define ShipY (480-33)
struct {
	int mode;
	int alive;
	f32 shipX;
	f32 shotTimer;
	f32 alienShotTimer;

	const char* message;

	Bullet bullets[64];
	Block blocks[64];
	Invader invaders[64];

	i32 textDark;
	i32 flipTimer;
	i32 fontSize; 
	f32 smoothing;
} gstate;


void shootBullet(f32 x, f32 y, f32 v)
{
	for(isize i = 0; i < 64; ++i) {
		Bullet* b = gstate.bullets + i;
		if(!b->exists) {
			b->exists = 1;
			b->x = x;
			b->y = y;
			b->v = v;
			return;
		}
	}

}

void addBlock(f32 x, f32 y, int hp)
{
	for(isize i = 0; i < 64; ++i) {
		Block* b = gstate.blocks + i;
		if(!b->exists) {
			b->exists = 1;
			b->x = x;
			b->y = y;
			b->hp = hp;
			b->timer = BlockInterval;
			return;
		}
	}
}

void addInvader(f32 x, f32 y, char c)
{
	for(isize i = 0; i < 64; ++i) {
		Invader* a = gstate.invaders + i;
		if(!a->exists) {
			a->exists = 1;
			a->x = x;
			a->y = y;
			a->c = c;
			return;
		}
	}	
}

void init()
{
	gstate.alive = 1;
	gstate.shipX = 640 / 2;
	gstate.shotTimer = 0;
	gstate.alienShotTimer = AlienInterval;

	for(isize i = 0; i < 64; ++i) {
		Bullet* b = gstate.bullets + i;
		b->exists = 0;
	}

	for(isize i = 0; i < 64; ++i) {
		Block* b = gstate.blocks + i;
		b->exists = 0;
	}

	for(isize i = 0; i < 64; ++i) {
		Invader* a = gstate.invaders + i;
		a->exists = 0;
	}

	for(isize i = 0; i < 640/16; i+=6) {
		addBlock(i * 16 + 32, ShipY - 32, 3);
		addBlock(i * 16+16 + 32, ShipY - 48, 3);
		addBlock(i * 16+32 + 32, ShipY - 32, 3);
	}


	for(isize i = 0; i < 60; ++i) {
		f32 x = (i * 48) % 620;
		f32 y = (i * 48) / 630;
		addInvader(16 + x, 48 + y * 48, badRand() % 94 + 33);
	}

}

void update()
{
	if(keys[KeyReturn]) {
		gstate.mode = 0;
	}

	if(gstate.mode == 0) {
		const char* title = "Text Invaders";
		wDrawText(&g, font,
				16, 16,
				title, badstrlen(title),
				36, Anchor_TopLeft,
				0xFFFFFFFF, 0.5);
		const char* options = "Press LEFT for text, RIGHT for game";
		const char* instr = "(game instructions: Left/Right to move, Space to shoot)\n"
			"(you can always press RETURN to return here)";
		wDrawText(&g, font,
				16, 128,
				options, badstrlen(options),
				18, Anchor_TopLeft,
				0xFFFFFFFF, 2);

		wDrawText(&g, font,
				16, 480 - 64,
				instr, badstrlen(instr),
				12, Anchor_TopLeft,
				0xFFFFFF77, 2.5);

		if(keys[KeyLeft]) {
			gstate.mode = 1;
			gstate.smoothing = 2.0;
			gstate.fontSize = 12;
		} else if(keys[KeyRight]) {
			gstate.mode = 2;
			init();
		}

		return;
	} else if(gstate.mode == 1) {
		gstate.flipTimer--;
		if(gstate.flipTimer <= 0 && keys[KeySpace])  {
			gstate.textDark = !gstate.textDark;
			gstate.flipTimer = 15;
		}

		if(keys[KeyLeft]) gstate.fontSize--;
		if(keys[KeyRight]) gstate.fontSize++;
		if(keys[KeyUp]) gstate.smoothing += 0.025;
		if(keys[KeyDown]) gstate.smoothing -= 0.025;
		if(keys[KeySpace]) {
			gstate.smoothing = 2;
			gstate.fontSize = 12;
		}

		u32 color = 0xFFFFFFFF;
		if(!gstate.textDark) {
			color = 0xFF;
			wGroupAddRaw(&g, Anchor_TopLeft | Sprite_NoTexture, 0xFFFFFFFF,
					0, 0, 640, 480,
					0, 0, 0, 0);
		}

		wDrawText(&g, font,
				16, 8,
				textSample, badstrlen(textSample),
				gstate.fontSize, Anchor_TopLeft,
				color, gstate.smoothing);

		return;
	}

	if(!gstate.alive) {
		wSpriteList s = wDrawText(&g, font,
				0, 64,
				gstate.message, badstrlen(gstate.message),
				36, Anchor_TopLeft,
				0xFFFFFFFF, 0.5);
		f32 w = s.r - s.l;
		for(isize i = 0; i < s.count; ++i) {
			wSprite* sprite = g.sprites + i + s.start;
			sprite->x += (640.0f - w) / 2.0f;
		}

		const char* instr = "(press RETURN to go back to the menu)";
		wDrawText(&g, font,
				16, 480 - 64,
				instr, badstrlen(instr),
				12, Anchor_TopLeft,
				0xFFFFFF77, 2.5);
		return;
	}

	//background
	wGroupAddRaw(&g, Anchor_TopLeft | Sprite_NoAA, 0xFFFFFFFF,
			0, 0, 640, 480,
			1024, 505, 640, 480);

	{ // controls
		f32 move = 4.0f;
		if(keys[KeyLeft]) {
			gstate.shipX -= move;
		}
		if(keys[KeyRight]) {
			gstate.shipX += move;
		}

		if(gstate.shipX - 15 < 0) {
			gstate.shipX = 15;
		}

		if(gstate.shipX + 15 > 640) {
			gstate.shipX = 640 - 15;
		}

		if(gstate.shotTimer <= 0 && keys[KeySpace]) {
			shootBullet(gstate.shipX, ShipY-16, -8);
			gstate.shotTimer = ShotInterval;
		} else {
			gstate.shotTimer -= 1.0f/60.f;
			if(gstate.shotTimer <= 0 ) gstate.shotTimer = 0;
		}
	}
	drawShip(gstate.shipX, ShipY);


	for(isize i = 0; i < 64; ++i) {
		Bullet* b = gstate.bullets + i;
		if(!b->exists) continue;
		for(isize j = 0; j < 64; ++j) {
			Block* block = gstate.blocks + j;
			if(!block->exists) continue;
			if(collideBox(b->x, b->y, 2, 2, block->x, block->y, 8, 8)) {
				b->exists = 0;
				block->hp--;
				if(block->hp == 0) {
					block->exists = 0;
				}
				break;
			}
		}

		if(b->v < 0)
		for(isize j = 0; j < 64; ++j) {
			Invader* a = gstate.invaders + j;
			if(!a->exists) continue;
			if(collideBox(b->x, b->y, 2, 2, a->l+a->w, a->t+a->h, a->w, a->h)) {
				b->exists = 0;
				a->exists = 0;
				break;
			}
		}


		if(collideBox(b->x, b->y, 2, 2, gstate.shipX, ShipY, 8, 4)) {
			gstate.alive = 0;
			gstate.message = "You Have Failed";
			return;
		}

		if(b->y < 0 || b->y > 480) {
			b->exists = 0;
			continue;
		}

		drawBullet(b->x, b->y, b->v);
		b->y += b->v;
	}

	for(isize i = 0; i < 64; ++i) {
		Block* b = gstate.blocks + i;
		if(!b->exists) continue;
		drawBlock(b->x, b->y, b->hp);
		if(b->hp == 1) {
			b->timer -= 1.0f / 60.0f;
			if(b->timer <= 0) {
				b->hp = 2;
				b->timer = BlockInterval;
			}
		} 
	}

	f32 rate = 0;
	i32 aliveCount = 0;
	for(isize i = 0; i < 64; ++i) {
		Invader* a = gstate.invaders + i;
		if(!a->exists) continue;
		aliveCount += 1;
	}
	
	if(aliveCount == 0) {
		gstate.alive = 0;
		gstate.message = "You Win!";
	}

	rate = aliveCount;
	rate /= 64.0f;
	rate = 1 - rate + 0.05;
	if(rate > 0.75) rate *= 1.5;

	for(isize i = 0; i < 64; ++i) {
		Invader* a = gstate.invaders + i;
		if(!a->exists) continue;
		if(a->y > (ShipY - 160)) {
			drawInvader(a, 1);
		} else {
			drawInvader(a, rate);
		}
		a->x += rate * 2;
		if(a->x + a->w*2 > 640) {
			a->x = -a->w;
			a->y += 48;
		}

		if(collideBox(gstate.shipX, ShipY, 8, 4, a->l+a->w, a->t+a->h, a->w, a->h)) {
			gstate.alive = 0;
		}
	}

	if(gstate.alienShotTimer <= 0) {
		//this is hilariously inefficient
		i32 count = 6;
		if(aliveCount > 40) {
			count = 10;
		} else if(aliveCount < 8) {
			count = 4;
		}
		for(isize times = 0; times < count; ++times) {
			u32 startIndex = badRand() % 60;
			while(!gstate.invaders[startIndex].exists) {
				startIndex++;
				startIndex = startIndex % 64;
			}
			Invader* a = gstate.invaders + startIndex;
			shootBullet(a->x+ a->w*2, a->y + a->h*2, 4);
		}
		gstate.alienShotTimer = AlienInterval;
	} else {
		gstate.alienShotTimer -= 1.0f/60.0f;
	}

}
