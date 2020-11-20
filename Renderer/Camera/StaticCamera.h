#ifndef __STATIC_CAMERA_H__
#define __STATIC_CAMERA_H__


class CStaticCamera : public CCamera
{
public:

	CStaticCamera();
	~CStaticCamera();

	void Update() override;
};


#endif
