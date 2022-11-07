#define MAX_SPRITES 5
#ifdef TEST
#define SPRITE_SIZE 12
#else
#define SPRITE_SIZE 8 
#endif
#define SPRITE_INTERVAL (SPRITE_SIZE + 4) 
signed char nb_sprites;

unsigned char sp_x[MAX_SPRITES];
unsigned char sp_y[MAX_SPRITES];
signed char sp_br[MAX_SPRITES];
signed char sp_overlap[MAX_SPRITES];
signed char sp_model[MAX_SPRITES];

signed char sorted_by_y[MAX_SPRITES];

signed char i, j, k, l, m;
signed char X, Y;

void init_sprites()
{
  nb_sprites = 0;
}

signed char xx, yy, model;

// Create a new sprite at xx, yy (model provided)
// Parameters: xx, yy, model
// Uses: i, j, k, l
int new_sprite()
{
  // Look for right yy position
  for (X = nb_sprites; X != 0; X--) {
    X--;
    Y = sorted_by_y[X];
    X++;
    if (yy >= sp_y[Y]) break;
    sorted_by_y[X] = Y;
  }

  // Put new sprite data
  // Look for a free place
  for (Y = 0; Y != nb_sprites; Y++) {
    if (sp_br[Y] == -1) break;
  }

  sorted_by_y[X] = Y;
  sp_x[Y] = xx;
  sp_y[Y] = yy;
  sp_br[Y] = MAX_SPRITES;
  sp_model[Y] = model;
  i = X;
  l = Y;
  X = nb_sprites;
  
  // Update number of sprites
  nb_sprites++;
  
/* Not necessary if overlap update is done in another function  
  // Compute overlap of this sprite
  sp_overlap[Y] = 0;
  X = i;
  for (X++; X != nb_sprites; X++) {
    j = Y;
    Y = sorted_by_y[X];
    if (sp_y[Y] > yy + SPRITE_INTERVAL) break;
    Y = j;
    sp_overlap[Y]++;
  }  
  
  // Update overlap of upper sprites
  for (X = i - 1; X >= 0; X--) {
    Y = sorted_by_y[X];
    k = X;
    X++;
    X = sorted_by_y[X];
    if (sp_y[X] > sp_y[Y] + SPRITE_INTERVAL) break; 
    X = k;
    sp_overlap[Y] = 1;
    j = X;
    for (X = X + 2; X != nb_sprites; X++) {
      k = X;
      X = sorted_by_y[X];
      if (sp_y[X] > sp_y[Y] + SPRITE_INTERVAL) break;
      X = k;
      sp_overlap[Y]++;
    }
    X = j;
  }
*/
  return l;
}

// Parameter i (index of sprite)  
void delete_sprite()
{
  // Remove from sorted_by_y array
  for (X = 0; X != nb_sprites; X++) {
    if (sorted_by_y[X] == i) break;
  }
  l = X;
  for (; X < nb_sprites - 1; X++) {
    Y = X + 1;
    sorted_by_y[X] = sorted_by_y[Y];
  }
/* Not necessary if overlap update is done in another function  
  // Update overlap of upper sprites
  for (X = l - 1; X >= 0; X--) {
    Y = sorted_by_y[X];
    k = X;
    X++;
    X = sorted_by_y[X];
    if (sp_y[X] > sp_y[Y] + SPRITE_INTERVAL) break; 
    X = k;
    sp_overlap[Y] = 1;
    j = X;
    for (X = X + 2; X != nb_sprites; X++) {
      k = X;
      X = sorted_by_y[X];
      if (sp_y[X] > sp_y[Y] + SPRITE_INTERVAL) break;
      X = k;
      sp_overlap[Y]++;
    }
    X = j;
  }
*/

  sp_br[i] = -1; // Mark as free
  nb_sprites--;
}

// Parameter i (index of sprite), xx = new X pos, yy = new Y pos
void move_sprite()
{
    X = i;
    sp_x[X] = xx;
    if (sp_y[X] == yy) return; // No vertical move, so nothing to check
    Y = 0;
    while (sorted_by_y[Y] != i) Y++;
    // Update sorted_by_y if needed    
    if (sp_y[X] < yy) {
      // We have gone downwards
      sp_y[X] = yy;
      for (; Y != nb_sprites; Y++) {
        Y++;
        X = sorted_by_y[Y];
        Y--;
        if (yy > sp_y[X]) {
          sorted_by_y[Y] = sorted_by_y[Y + 1];
        } else break;
      }
      sorted_by_y[Y] = i;
    } else {
      // We have gone upwards
      sp_y[X] = yy;
      for (; Y != 0; Y--) {
        Y--;
        X = sorted_by_y[Y];
        Y++;
        if (yy < sp_y[X]) {
          sorted_by_y[Y] = sorted_by_y[Y - 1];
        } else break;
      }
      sorted_by_y[Y] = i;
    }
}

// This one should be called regularly. For instance every 5 frames or so...
void update_overlap()
{
  m = nb_sprites - 1;
  for (X = 0; X != m; X++) {
    Y = sorted_by_y[X];
    j = X;
    X++;
    X = sorted_by_y[X];
    if (sp_y[X] > sp_y[Y] + SPRITE_INTERVAL) {
      sp_overlap[Y] = 0;
    } else {
      i = sp_y[Y] + SPRITE_INTERVAL;
      k = Y;
      l = 1;
      for (Y = j + 2; Y != nb_sprites; Y++) {
        X = sorted_by_y[Y];
        if (sp_y[X] > i) break;
        l++;
      }
      Y = k;
      sp_overlap[Y] = l; 
    }
    X = j;
  }
  Y = sorted_by_y[X];
  sp_overlap[Y] = 0;
}

// Display update algo :
// In the kernel, when selecting next sprite :
// - check sprite_br: if < overlap, skip and increment sprite_br
// else if in bounds, display it and reset sprite_br.
// else skip.

#ifdef TEST
#include <assert.h>
#define test1 main
void test1()
{
  init_sprites();
  xx = 0; model = 0; 
  yy = 0; new_sprite();
  yy = 10; new_sprite();
  yy = 20; new_sprite();
  yy = 30; new_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  for (i = 0; i < nb_sprites; i++) {
    assert(sorted_by_y[i] == i);
  }
  for (i = 0; i < nb_sprites - 1; i++) {
    assert(sp_overlap[i] == 1);
  }
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);
  
  i = 1; xx = 10; yy = 10; move_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  for (i = 0; i < nb_sprites; i++) {
    assert(sorted_by_y[i] == i);
  }
  for (i = 0; i < nb_sprites - 1; i++) {
    assert(sp_overlap[i] == 1);
  }
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);

  i = 1; xx = 10; yy = 17; move_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  for (i = 0; i < nb_sprites; i++) {
    assert(sorted_by_y[i] == i);
  }
  assert(sp_overlap[0] == 0);
  assert(sp_overlap[1] == 2);
  assert(sp_overlap[2] == 1);
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);

  i = 1; xx = 10; yy = 25; move_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  assert(sorted_by_y[0] == 0);
  assert(sorted_by_y[1] == 2);
  assert(sorted_by_y[2] == 1);
  assert(sorted_by_y[3] == 3);
  assert(sp_overlap[0] == 0);
  assert(sp_overlap[1] == 1);
  assert(sp_overlap[2] == 2);
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);

  i = 1; xx = 0; yy = 0; move_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  for (i = 0; i < nb_sprites; i++) {
    assert(sorted_by_y[i] == i);
  }
  assert(sp_overlap[0] == 1);
  assert(sp_overlap[1] == 0);
  assert(sp_overlap[2] == 1);
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);

  nb_sprites = 0;
  yy = 0; new_sprite();
  yy = 5; new_sprite();
  yy = 10; new_sprite();
  yy = 20; new_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  for (i = 0; i < nb_sprites; i++) {
    assert(sorted_by_y[i] == i);
  }
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);
  assert(sp_overlap[0] == 2);
  assert(sp_overlap[1] == 2);
  assert(sp_overlap[2] == 1);
  
  nb_sprites = 0;
  yy = 0; new_sprite();
  yy = 5; new_sprite();
  yy = 10; new_sprite();
  yy = 23; new_sprite();
  assert(nb_sprites == 4);
  update_overlap();
  for (i = 0; i < nb_sprites; i++) {
    assert(sorted_by_y[i] == i);
  }
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);
  assert(sp_overlap[0] == 2);
  assert(sp_overlap[1] == 1);
  assert(sp_overlap[2] == 1);
  
  nb_sprites = 0;
  yy = 0; assert(new_sprite() == 0);
  yy = 20; assert(new_sprite() == 1);
  yy = 5; assert(new_sprite() == 2);
  yy = 10; assert(new_sprite() == 3);
  assert(nb_sprites == 4);
  update_overlap();
  assert(sorted_by_y[0] == 0);
  assert(sorted_by_y[1] == 2);
  assert(sorted_by_y[2] == 3);
  assert(sorted_by_y[3] == 1);
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);
  assert(sp_overlap[0] == 2);
  assert(sp_overlap[2] == 2);
  assert(sp_overlap[3] == 1);

  i = 2; delete_sprite();
  update_overlap();
  assert(sorted_by_y[0] == 0);
  assert(sorted_by_y[1] == 3);
  assert(sorted_by_y[2] == 1);
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);
  assert(sp_overlap[0] == 1);
  assert(sp_overlap[3] == 1);
  assert(nb_sprites == 3);

  yy = 5; assert(new_sprite() == 2);
  update_overlap();
  assert(sorted_by_y[0] == 0);
  assert(sorted_by_y[1] == 2);
  assert(sorted_by_y[2] == 3);
  assert(sorted_by_y[3] == 1);
  assert(sp_overlap[sorted_by_y[nb_sprites - 1]] == 0);
  assert(sp_overlap[0] == 2);
  assert(sp_overlap[2] == 2);
  assert(sp_overlap[3] == 1);
}
#endif
