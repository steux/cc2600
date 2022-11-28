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

char i, j, k, l, m;
signed char X, Y;

void init_sprites()
{
  nb_sprites = 0;
}

signed char xx, yy, model;

void new_sprite()
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

  X = i;
  sp_br[X] = -1; // Mark as free
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
          Y++;
          j = sorted_by_y[Y];
          Y--;
          sorted_by_y[Y] = j;
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
          Y--;
          j = sorted_by_y[Y];
          Y++;
          sorted_by_y[Y] = j;
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
    }
    else {
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

