#include "Physics.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CVolume_t CVolume_merge(CVolume_t * pv1, CVolume_t * pv2)
{
  CVolume_t rv;

  rv.lod = -1;

  if(NULL ==pv1 && NULL ==pv2)
  {
    return rv;
  }
  else if(NULL ==pv2)
  {
    return *pv1;
  }
  else if(NULL ==pv1)
  {
    return *pv2;
  }
  else
  {
    bool_t binvalid;

    // check for uninitialized volumes
    if(-1==pv1->lod && -1==pv2->lod)
    {
      return rv;
    }
    else if(-1==pv1->lod)
    {
      return *pv2;
    }
    else if(-1==pv2->lod)
    {
      return *pv1;
    };

    // merge the volumes

    rv.x_min = MIN(pv1->x_min, pv2->x_min);
    rv.x_max = MAX(pv1->x_max, pv2->x_max);

    rv.y_min = MIN(pv1->y_min, pv2->y_min);
    rv.y_max = MAX(pv1->y_max, pv2->y_max);

    rv.z_min = MIN(pv1->z_min, pv2->z_min);
    rv.z_max = MAX(pv1->z_max, pv2->z_max);

    rv.xy_min = MIN(pv1->xy_min, pv2->xy_min);
    rv.xy_max = MAX(pv1->xy_max, pv2->xy_max);

    rv.yx_min = MIN(pv1->yx_min, pv2->yx_min);
    rv.yx_max = MAX(pv1->yx_max, pv2->yx_max);

    // check for an invalid volume
    binvalid = (rv.x_min >= rv.x_max) || (rv.y_min >= rv.y_max) || (rv.z_min >= rv.z_max);
    binvalid = binvalid ||(rv.xy_min >= rv.xy_max) || (rv.yx_min >= rv.yx_max);

    rv.lod = binvalid ? -1 : 1;
  }

  return rv;
}

//--------------------------------------------------------------------------------------------
CVolume_t CVolume_intersect(CVolume_t * pv1, CVolume_t * pv2)
{
  CVolume_t rv;

  rv.lod = -1;

  if(NULL ==pv1 || NULL ==pv2)
  {
    return rv;
  }
  else
  {
    // check for uninitialized volumes
    if(-1==pv1->lod && -1==pv2->lod)
    {
      return rv;
    }
    else if(-1==pv1->lod)
    {
      return *pv2;
    }
    else if (-1==pv2->lod)
    {
      return *pv1;
    }

    // intersect the volumes
    rv.x_min = MAX(pv1->x_min, pv2->x_min);
    rv.x_max = MIN(pv1->x_max, pv2->x_max);
    if(rv.x_min >= rv.x_max) return rv;

    rv.y_min = MAX(pv1->y_min, pv2->y_min);
    rv.y_max = MIN(pv1->y_max, pv2->y_max);
    if(rv.y_min >= rv.y_max) return rv;

    rv.z_min = MAX(pv1->z_min, pv2->z_min);
    rv.z_max = MIN(pv1->z_max, pv2->z_max);
    if(rv.z_min >= rv.z_max) return rv;

    if(pv1->lod>0 && pv2->lod>0)
    {
      rv.xy_min = MAX(pv1->xy_min, pv2->xy_min);
      rv.xy_max = MIN(pv1->xy_max, pv2->xy_max);
      if(rv.xy_min >= rv.xy_max) return rv;

      rv.yx_min = MAX(pv1->yx_min, pv2->yx_min);
      rv.yx_max = MIN(pv1->yx_max, pv2->yx_max);
      if(rv.yx_min >= rv.yx_max) return rv;
    }
    else if(pv1->lod>0)
    {
      rv.xy_min = pv1->xy_min;
      rv.xy_max = pv1->xy_max;
      if(rv.xy_min >= rv.xy_max) return rv;

      rv.yx_min = pv1->yx_min;
      rv.yx_max = pv1->yx_max;
      if(rv.yx_min >= rv.yx_max) return rv;
    }
    else if(pv2->lod>0)
    {
      rv.xy_min = pv2->xy_min;
      rv.xy_max = pv2->xy_max;
      if(rv.xy_min >= rv.xy_max) return rv;

      rv.yx_min = pv2->yx_min;
      rv.yx_max = pv2->yx_max;
      if(rv.yx_min >= rv.yx_max) return rv;
    }

    rv.lod = (pv1->lod>0 || pv2->lod>0) ? 1 : 0;
  }

  return rv;
}

//--------------------------------------------------------------------------------------------
bool_t CPhysAccum_clear(PhysAccum_t * paccum)
{
  if(NULL == paccum) return bfalse;

  VectorClear( paccum->acc.v );
  VectorClear( paccum->vel.v );
  VectorClear( paccum->pos.v );

  return btrue;
}

//--------------------------------------------------------------------------------------------
void phys_integrate(Orientation_t * pori, Orientation_t * pori_old, PhysAccum_t * paccum, float dFrame)
{
  float diff;

  diff = ABS(pori->pos.x - pori_old->pos.x) + ABS(pori->pos.y - pori_old->pos.y);
  if(diff > 1)
  {
    // if the difference is greater than one tile, store the new value
    pori_old->pos = pori->pos;
  }

  pori->pos.x += pori->vel.x * dFrame + paccum->pos.x;
  pori->pos.y += pori->vel.y * dFrame + paccum->pos.y;
  pori->pos.z += pori->vel.z * dFrame + paccum->pos.z;

  pori->vel.x += paccum->acc.x * dFrame + paccum->vel.x;
  pori->vel.y += paccum->acc.y * dFrame + paccum->vel.y;
  pori->vel.z += paccum->acc.z * dFrame + paccum->vel.z;

  CPhysAccum_clear( paccum );

};