#ifndef __FREECAM_H__
#define __FREECAM_H__


class CFreeCamera : public CCamera
{
public:

	CFreeCamera();
	~CFreeCamera();

	void Update() override;
};


#endif
