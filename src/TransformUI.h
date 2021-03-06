#pragma once

#include "cinder/Vector.h"
#include "cinder/Rect.h"
#include "cinder/Matrix.h"
#include "cinder/app/MouseEvent.h"

class TransformUI
{
public:
	TransformUI(void);
	~TransformUI(void);
	void setView(ci::ivec2 windowSize, float scale);
	void setShape(ci::Rectf rect);

	
	//inpute handlers return true if shape changed
	bool mouseMove(ci::app::MouseEvent evt);
	bool mouseDown(ci::app::MouseEvent evt);
	bool mouseUp(ci::app::MouseEvent evt);
	bool mouseDrag(ci::app::MouseEvent evt);

	void draw();
	void center();
	ci::Rectf getBounds();
	ci::mat4 getShapeToView() { return mShapeToView; };
	ci::mat4 getViewToShape() { return mViewToShape; };
	
	static const int NONE = -1;
	static const int TOP_LEFT = 0;
	static const int TOP_RIGHT = 1;
	static const int BOTTOM_RIGHT = 2;
	static const int BOTTOM_LEFT = 3;

	static const int TOP = 0;
	static const int RIGHT = 1;
	static const int BOTTOM = 2;
	static const int LEFT = 3;

	ci::vec2 shape[4];
	
private:
	void updateMatrix();
	ci::vec2 rotatePoint(const ci::vec2& point, const ci::vec2& rotBase, float angleRadian);
	void drawRotationCone();
	float getAngle(const ci::vec2& v1, const ci::vec2& v2);
	ci::mat4 mShapeToView;
	ci::mat4 mViewToShape;
    ci::mat4 mRotShape;
    ci::vec2 mRotDest;
	ci::vec2 mShapeOffset;
	ci::vec2 mViewSize;
	float mViewScale;
	bool mLeftMouseDown;
	bool mRightMouseDown;
    float angleDeg;
	//dragging
	int mDraggedCorner;
	int mDraggedEdge;
	ci::vec2 mEdgeStartToMouse;
	ci::vec2 mMouseToEdgeEnd;
	ci::vec2 mMouseDragStart;
	//rotation
	bool mIsRotating;
	ci::vec2 mRotationStartDir;
	ci::vec2 mRotationCurrentDir;
	float mRotationAngle;
	ci::vec2 mPreRotShape[4];
	//config
	float mHandleRadius;
	float mRotationHandleLength;
};

