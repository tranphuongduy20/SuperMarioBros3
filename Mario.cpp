#include <algorithm>
#include <assert.h>
#include "Utils.h"

#include "Mario.h"
#include "Game.h"

#include "Goomba.h"
#include "Portal.h"
#include "Brick.h"

CMario::CMario(float x, float y) : CGameObject()
{
	level = MARIO_LEVEL_SMALL;
	untouchable = 0;
	SetState(MARIO_STATE_IDLE);
	start_x = x; 
	start_y = y; 
	this->x = x; 
	this->y = y; 
}

void CMario::Update(DWORD dt, vector<LPGAMEOBJECT> *coObjects)
{
	// Calculate dx, dy 
	CGameObject::Update(dt);

	// Simple fall down
	vy += MARIO_GRAVITY*dt;
	//vx += 0.001 * dt;

	vector<LPCOLLISIONEVENT> coEvents;
	vector<LPCOLLISIONEVENT> coEventsResult;

	coEvents.clear();

	// reset untouchable timer if untouchable time has passed
	if ( GetTickCount() - untouchable_start > MARIO_UNTOUCHABLE_TIME) 
	{
		untouchable_start = 0;
		untouchable = 0;
	}

	// turn off collision when die 
	if (state!=MARIO_STATE_DIE)
		CalcPotentialCollisions(coObjects, coEvents);

	// No collision occured, proceed normally
	if (coEvents.size()==0)
	{
		x += dx; 
		y += dy;
	}
	else
	{
		float min_tx, min_ty, nx = 0, ny;
		float rdx = 0; 
		float rdy = 0;

		// TODO: This is a very ugly designed function!!!!
		FilterCollision(coEvents, coEventsResult, min_tx, min_ty, nx, ny, rdx, rdy);

		// how to push back Mario if collides with a moving objects, what if Mario is pushed this way into another object?
		//if (rdx != 0 && rdx!=dx)
		//	x += nx*abs(rdx); 
		
		// block every object first!
		x += min_tx*dx + nx*0.4f;
		y += min_ty*dy + ny*0.4f;

		if (nx!=0) vx = 0;
		if (ny!=0) vy = 0;


		//
		// Collision logic with other objects
		//
		for (UINT i = 0; i < coEventsResult.size(); i++)
		{
			LPCOLLISIONEVENT e = coEventsResult[i];

			if (dynamic_cast<CGoomba *>(e->obj)) // if e->obj is Goomba 
			{
				CGoomba *goomba = dynamic_cast<CGoomba *>(e->obj);

				// jump on top >> kill Goomba and deflect a bit 
				if (e->ny < 0)
				{
					if (goomba->GetState()!= GOOMBA_STATE_DIE)
					{
						goomba->SetState(GOOMBA_STATE_DIE);
						vy = -MARIO_JUMP_DEFLECT_SPEED;
					}
				}
				else if (e->nx != 0)
				{
					if (untouchable==0)
					{
						if (goomba->GetState()!=GOOMBA_STATE_DIE)
						{
							if (level > MARIO_LEVEL_SMALL)
							{
								level = MARIO_LEVEL_SMALL;
								StartUntouchable();
							}
							else 
								SetState(MARIO_STATE_DIE);
						}
					}
				}
			} // if Goomba
			else if (dynamic_cast<CBrick*>(e->obj)) // if e->obj is CBrick 
			{
				CBrick* brick = dynamic_cast<CBrick*>(e->obj);
			
				if (e->ny != 0)
				{
					isGround = true;
				}
			} // if Brick
			else if (dynamic_cast<CPortal *>(e->obj))
			{
				CPortal *p = dynamic_cast<CPortal *>(e->obj);
				CGame::GetInstance()->SwitchScene(p->GetSceneId());
			}
		}
	}

	// clean up collision events
	for (UINT i = 0; i < coEvents.size(); i++) delete coEvents[i];
}

void CMario::Render()
{

	/*if (vx > 0)
		nx = 1;
	else
		nx = -1;*/
	int ani = -1;
	if (state == MARIO_STATE_DIE)
		ani = MARIO_ANI_DIE;
	else
	if (level == MARIO_LEVEL_BIG)
	{
		
		ani = MARIO_ANI_BIG_IDLE_RIGHT;
		/*if (vx == 0)
		{
			if (nx > 0) ani = MARIO_ANI_BIG_IDLE_RIGHT;
			else ani = MARIO_ANI_BIG_IDLE_LEFT;
		}
		else if (vx > 0) 
			ani = MARIO_ANI_BIG_WALKING_RIGHT; 
		else ani = MARIO_ANI_BIG_WALKING_LEFT;*/
	}
	else if (level == MARIO_LEVEL_SMALL)
	{
		ani = MARIO_ANI_SMALL_IDLE_RIGHT;
		/*if (vx == 0)
		{
			
			if (nx>0) ani = MARIO_ANI_SMALL_IDLE_RIGHT;
			else ani = MARIO_ANI_SMALL_IDLE_LEFT;
		}
		else if (vx > 0)
			ani = MARIO_ANI_SMALL_WALKING_RIGHT;
		else ani = MARIO_ANI_SMALL_WALKING_LEFT;*/
	}
	if (state == MARIO_STATE_CROUCH)
		ani = MARIO_STATE_CROUCH;
	int alpha = 255;
	if (untouchable) alpha = 128;
	DebugOut(L"[xxxxxxxxxxxx] direction \n", nx);
	animation_set->at(ani)->Render(nx, x, y, alpha);

	//RenderBoundingBox();
}

void CMario::SetState(int state)
{
	CGameObject::SetState(state);

	switch (state)
	{
	case MARIO_STATE_WALKING_RIGHT:
		vx = MARIO_WALKING_SPEED;
		nx = 1;
		//Go();
		break;
	case MARIO_STATE_WALKING_LEFT: 
		vx = -MARIO_WALKING_SPEED;
		nx = -1;
		//Go();
		break;
	case MARIO_STATE_JUMP:
		// TODO: need to check if Mario is *current* on a platform before allowing to jump again
		if (isGround) {
			vy = -MARIO_JUMP_SPEED_Y;
			isGround = false;
		}
		break; 
	case MARIO_STATE_JUMP_MAX:		
		vy = -MARIO_JUMP_SPEED_YMAX;
		break;
	case MARIO_STATE_IDLE:
		vx = 0;
		if (vx * nx <= 0)
		{
			vx = 0;
			break;
		}vx -= MARIO_ACC * nx;
		break;
	case MARIO_STATE_DIE:

		vy = -MARIO_DIE_DEFLECT_SPEED;
		break;
	case MARIO_STATE_CROUCH:
		vx = 0;
		nx = 1;
		break;
	}
}

void CMario::GetBoundingBox(float &left, float &top, float &right, float &bottom)
{
	left = x;
	top = y; 

	if (level==MARIO_LEVEL_BIG)
	{
		right = x + MARIO_BIG_BBOX_WIDTH;
		bottom = y + MARIO_BIG_BBOX_HEIGHT;
	}
	else
	{
		right = x + MARIO_SMALL_BBOX_WIDTH;
		bottom = y + MARIO_SMALL_BBOX_HEIGHT;
	}
}

/*
	Reset Mario status to the beginning state of a scene
*/
void CMario::Reset()
{
	SetState(MARIO_STATE_IDLE);
	SetLevel(MARIO_LEVEL_BIG);
	SetPosition(start_x, start_y);
	SetSpeed(0, 0);
}

void CMario::speedUp()
{
	if (vx < MARIO_VMAX)
		vx += MARIO_SPEEEDUP*nx;
	else
		vx = MARIO_VMAX * nx;
	DebugOut(L"%.2f", vx);
}

void CMario::speedUpY()
{
	if (vy < MARIO_JUMP_SPEED_YMAX)
		vy += MARIO_JUMP_Y * -1;
	else
		vy = MARIO_JUMP_SPEED_YMAX * -1;
	DebugOut(L"%.2f", vy);
}

//void CMario::Go()
//{
//	if (vx < MARIO_GO)
//		vx += MARIO_GOMAX * nx;
//	else
//		vx = MARIO_VMAX;
//	DebugOut(L"%.2f", vx);
//}

