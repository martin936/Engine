#ifndef PHYSICS_BALLS_INC
#define PHYSICS_BALLS_INC

/*
typedef struct {

  Mesh3D* mesh;
  CSoftbody* m_pSoftbody;

  float radius;
  float compr;
  float minvol;
  float v;
  float friction;
  
  int n_holes;
  PartSystem** stream;

  int n_forces;
  Force** forces;

  int should_flush;
  float wall_norm[2];

  double m_fLastCollisionTime;

}Ball;


Ball* physics_ball_new( float radius, float K, float Kmin, int tesselation, float Ksurf, float damping, int Id );

Ball* physics_ball_new_from_file( const char* path, float compr, float minvol, float elast, float damping );

void physics_ball_set_position( Ball* ball, float x, float y, float z );

void physics_ball_step( Ball* ball, float dt, Wall* walls, int nw );

void physics_ball_update_volume( Ball* ball, float dt );

void physics_ball_delete_stream( Ball* ball, int idx );

void physics_ball_update_streams( Ball* ball );

void physics_ball_delete( Ball* ball );

void physics_ball_add_stream( Ball* ball, int tri, float u, float v );

int physics_ball_force_set_speed_target( Ball* ball, const char* Name, float Vx, float Vy, float Vz );

int physics_ball_force_edit( Ball* ball, const char* Name, float Fx, float Fy, float Fz );

int physics_ball_force_edit_duration( Ball* ball, const char* Name, float duration );

void physics_ball_add_force( Ball* ball, const char* Name, float Fx, float Fy, float Fz, ForceUsage usage, float duration );

void physics_ball_add_total_force( Ball* ball, float* total_force, float dt );

bool physics_ball_force_is_activated( Ball* ball, const char* Name );

int physics_ball_force_activate( Ball* ball, const char* Name );

int physics_ball_force_deactivate( Ball* ball, const char* Name );

void physics_ball_force_flush_all( Ball* ball );

void physics_ball_force_flush(Ball* ball, const char* Name);

void physics_ball_particles_update_source( Ball* ball, int idx );

void physics_ball_particles_add_moving_source( Ball* ball, int idx, int tri_idx, float u, float v );*/



#endif
