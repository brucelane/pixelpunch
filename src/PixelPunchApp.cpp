#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Rand.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;

#include <list>
using namespace std;

#include "SimpleGUI.h"
using namespace mowa::sgui;

#include "TransformUI.h"

#include "pixelpunch/PixelPunch.h"
#include "pixelpunch/PixelScale.h"
#include "pixelpunch/PixelTransform.h"

#include <boost/format.hpp>

void staticMouseDownHandler(MouseEvent event);
void staticMouseUpHandler(MouseEvent event);

// We'll create a new Cinder Application by deriving from the BasicApp class
class PixelPunchApp : public App {
public:
	void fileDrop(FileDropEvent event);
	// Cinder will always call this function whenever the user drags the mouse
	void keyDown(KeyEvent event);
	void keyUp(KeyEvent event);
	void mouseMove(MouseEvent event);
	void mouseDrag(MouseEvent event);
	void mouseDown(MouseEvent event);
	void mouseUp(MouseEvent event);

	// Cinder calls this function 30 times per second by default
	void draw();
	void update();
	void setup();
	void saveResultToFile();

private:
	void initOptions();
	void validateResultImage();

	//GUI
	SimpleGUI*				mGui;
	LabelControl*			mPerfLabel;
	TransformUI				mTransformUI;

	//Scale Options
	typedef std::map<pp::ScaleMethod, std::string> ScaleMethodNames;
	ScaleMethodNames		mScaleOptions;
	bool					mScaleChoice[20];//[hacky] provide room for all possible values of ScaleMethod as index

	//Transform Options
	typedef std::map<pp::TransformMethod, std::string> TransformMethodNames;
	TransformMethodNames	mTransformOptions;
	bool					mTransformChoice[20];//[hacky] provide room for all possible values of TransformMethod as index

	//Transform Options
	typedef std::map<pp::SamplingMethod, std::string> SamplingMethodNames;
	SamplingMethodNames		mSamplingOptions;
	bool					mSamplingChoice[20];//[hacky] provide room for all possible values of TransformMethod as index


	//Current parameters
	pp::ScaleMethod			mScaleMethod;
	pp::TransformMethod		mTransformMethod;
	pp::SamplingMethod		mSamplingMethod;
	float					mPrevMixThreshold;
	float					mMixThreshold;
	bool					mPrevDiffWithSmoothBicubic;
	bool					mDiffWithSmoothBicubic;
	float					mViewScale;
	bool					mDisplaySource;

	//DATA
	std::string				mSourceFileName;
	Surface					mSourceImage;
	Surface					mScaledSrc;
	gl::TextureRef             mPrevTexture;
	Surface					mResultImage;
	gl::TextureRef             mResultTexture;
};


void PixelPunchApp::initOptions()
{
	//SCALE OPTIONS
	mScaleOptions[pp::SM_NONE] = "None";
	mScaleOptions[pp::SM_SCALE2x] = "Scale2x";
	mScaleOptions[pp::SM_SCALE3x] = "Scale3x";
	mScaleOptions[pp::SM_SCALE4x] = "Scale4x";
	mScaleOptions[pp::SM_EAGLE2x] = "Eagle2x";
	mScaleOptions[pp::SM_SCALE2x_HQ] = "Scale2xHQ";
	mScaleOptions[pp::SM_SCALE3x_HQ] = "Scale3xHQ";
	mScaleOptions[pp::SM_SCALE4x_HQ] = "Scale4xHQ";
	mScaleMethod = pp::SM_NONE;

	//TRANSFORM OPTIONS
	mTransformOptions[pp::TM_IDENTITY] = "None";
	mTransformOptions[pp::TM_PROJECTIVE] = "Projective";
	mTransformOptions[pp::TM_BILINEAR] = "Bilinear";
	mTransformMethod = pp::TM_IDENTITY;

	//TRANSFORM OPTIONS
	mTransformOptions[pp::TM_IDENTITY] = "None";
	mTransformOptions[pp::TM_PROJECTIVE] = "Projective";
	mTransformOptions[pp::TM_BILINEAR] = "Bilinear";

	//SAMPLING OPTIONS
	mSamplingOptions[pp::SAMPLE_NEAREST] = "Nearest";
	mSamplingOptions[pp::SAMPLE_BILINEAR] = "Smooth Bilinear";
	mSamplingOptions[pp::SAMPLE_BICUBIC] = "Smooth Bicubic";
	mSamplingOptions[pp::SAMPLE_FIRST_BILINEAR] = "Major Bilinear";
	mSamplingOptions[pp::SAMPLE_SECOND_BILINEAR] = "Second Bilinear";
	mSamplingOptions[pp::SAMPLE_BEST_FIT_NARROW] = "Best Fit Narrow";
	mSamplingOptions[pp::SAMPLE_BEST_FIT_WIDE] = "Best Fit Wide";
	mSamplingOptions[pp::SAMPLE_BEST_FIT_ANY] = "Best Fit Any";
	mSamplingOptions[pp::SAMPLE_MINIMIZE_ERROR] = "Bilinear Mix";
	mSamplingMethod = pp::SAMPLE_NEAREST;
}

void PixelPunchApp::setup()
{
	initOptions();

	mViewScale = 3.0f;
	mDisplaySource = false;

	mGui = new SimpleGUI(this);
	mGui->addLabel("View");
	mGui->addParam("Zoom", &mViewScale, 1, 10, 4); //if we specify group id, we create radio button set

	//RadioButton Group: Upscaling Type
	mGui->addLabel("1. Upscaling");
	bool selected = true;
	for (ScaleMethodNames::iterator it = mScaleOptions.begin(); it != mScaleOptions.end(); ++it)
	{
		mGui->addParam(it->second, &mScaleChoice[it->first], selected, 1);
		selected = false;
	}

	//RadioButton Group: Transform Type
	mGui->addLabel("2. Transform");
	selected = true;
	for (TransformMethodNames::iterator it = mTransformOptions.begin(); it != mTransformOptions.end(); ++it)
	{
		mGui->addParam(it->second, &mTransformChoice[it->first], selected, 2);
		selected = false;
	}

	//RadioButton Group: Sampling Type
	mGui->addLabel("3. Sampling");
	selected = true;
	for (SamplingMethodNames::iterator it = mSamplingOptions.begin(); it != mSamplingOptions.end(); ++it)
	{
		mGui->addParam(it->second, &mSamplingChoice[it->first], selected, 3);
		selected = false;
	}
	mGui->addParam("Mix Threshold", &mMixThreshold, 0.0f, 1.0f, 0.5f); //if we specify group id, we create radio button set
	mGui->addParam("Show Diff", &mDiffWithSmoothBicubic, false);

	mPerfLabel = mGui->addLabel("Perf: 0 ms");
}

void PixelPunchApp::fileDrop(FileDropEvent event)
{
	mSourceFileName = event.getFile(0).string();
	mSourceImage = loadImage(mSourceFileName);
	mPrevTexture = gl::Texture::create(mSourceImage);
	mPrevTexture->setMagFilter(GL_NEAREST);
	mResultImage = Surface();
	mScaledSrc = Surface();

	mTransformUI.setShape(cinder::Rectf(0, 0, (float)mSourceImage.getWidth(), (float)mSourceImage.getHeight()));
	mTransformUI.center();

	validateResultImage();
}

void PixelPunchApp::validateResultImage()
{
	bool isValid = (mResultImage.getData() != NULL);

	//ScaleMethod changed?
	pp::ScaleMethod newScaleMethod = mScaleMethod;
	for (ScaleMethodNames::iterator it = mScaleOptions.begin(); it != mScaleOptions.end(); ++it)
		if (mScaleChoice[it->first] == true)
			newScaleMethod = it->first;

	isValid = isValid && (newScaleMethod == mScaleMethod);

	//TransformMethod changed
	pp::TransformMethod newTransformMethod = mTransformMethod;
	for (TransformMethodNames::iterator it = mTransformOptions.begin(); it != mTransformOptions.end(); ++it)
		if (mTransformChoice[it->first] == true)
			newTransformMethod = it->first;

	isValid = isValid && (newTransformMethod == mTransformMethod);

	//SamplingMethod changed
	pp::SamplingMethod newSamplingMethod = mSamplingMethod;
	for (SamplingMethodNames::iterator it = mSamplingOptions.begin(); it != mSamplingOptions.end(); ++it)
		if (mSamplingChoice[it->first] == true)
			newSamplingMethod = it->first;

	isValid = isValid && (newSamplingMethod == mSamplingMethod);
	isValid = isValid && (mPrevMixThreshold == mMixThreshold);
	isValid = isValid && (mPrevDiffWithSmoothBicubic == mDiffWithSmoothBicubic);

	if (mSourceImage.getData() && !isValid)
	{
		mPrevMixThreshold = mMixThreshold;
		mPrevDiffWithSmoothBicubic = mDiffWithSmoothBicubic;
		double t1 = getElapsedSeconds();

		if (mResultTexture)
			mPrevTexture = mResultTexture;

		//UPSCALE SOURCE
		if (newScaleMethod != mScaleMethod || !mScaledSrc.getData())
		{
			mScaleMethod = newScaleMethod;
			mScaledSrc = pp::scale(mSourceImage, mScaleMethod);
		}

		//TRANSFORM
		mTransformMethod = newTransformMethod;

		if (mTransformMethod == pp::TM_IDENTITY)
		{
			mResultImage = mScaledSrc;
		}
		else
		{
			pp::TransformMapping tfx = pp::TransformMapping(mTransformUI.shape);
			//SAMPLING
			mSamplingMethod = newSamplingMethod;
			pp::Palette colors;

			switch (mSamplingMethod)
			{
			case pp::SAMPLE_NEAREST:
			{
				pp::NearestNeighbourSampler NNS = pp::NearestNeighbourSampler(mScaledSrc);
				mResultImage = pp::transform(NNS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_BILINEAR:
			{
				pp::BilinearSampler BS = pp::BilinearSampler(mScaledSrc);
				mResultImage = pp::transform(BS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_BICUBIC:
			{
				pp::BicubicSampler BCS = pp::BicubicSampler(mScaledSrc);
				mResultImage = pp::transform(BCS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_FIRST_BILINEAR:
			{
				pp::BilinearDominanceSampler BDSF = pp::BilinearDominanceSampler(mScaledSrc, 0);
				mResultImage = pp::transform(BDSF, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_SECOND_BILINEAR:
			{
				pp::BilinearDominanceSampler BDSS = pp::BilinearDominanceSampler(mScaledSrc, 1);
				mResultImage = pp::transform(BDSS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_BEST_FIT_NARROW:
			{
				pp::BicubicBestFitSampler BSFS = pp::BicubicBestFitSampler(mScaledSrc, false);
				mResultImage = pp::transform(BSFS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_BEST_FIT_WIDE:
			{
				pp::BicubicBestFitSampler BSFW = pp::BicubicBestFitSampler(mScaledSrc, true);
				mResultImage = pp::transform(BSFW, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_BEST_FIT_ANY:
			{
				pp::getColors(mSourceImage, colors);
				pp::BicubicBestFitSampler BBFS = pp::BicubicBestFitSampler(mScaledSrc, colors);
				mResultImage = pp::transform(BBFS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_FIRST_WEIGHT:
			{
				pp::WeightSampler WSF = pp::WeightSampler(mScaledSrc, 0);
				mResultImage = pp::transform(WSF, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_SECOND_WEIGHT:
			{
				pp::WeightSampler WSS = pp::WeightSampler(mScaledSrc, 1);
				mResultImage = pp::transform(WSS, tfx, mTransformMethod);
				break;
			}
			case pp::SAMPLE_MINIMIZE_ERROR:
			{

				pp::BicubicSampler BCS = pp::BicubicSampler(mScaledSrc);
				pp::BilinearDominanceSampler BDSF = pp::BilinearDominanceSampler(mScaledSrc, 0);
				pp::BilinearDominanceSampler BDSS = pp::BilinearDominanceSampler(mScaledSrc, 1);
				pp::WeightSampler WSF = pp::WeightSampler(mScaledSrc, 0);
				Surface bicubic = pp::transform(BCS, tfx, mTransformMethod);
				Surface first = pp::transform(BDSF, tfx, mTransformMethod);
				Surface second = pp::transform(BDSS, tfx, mTransformMethod);
				Surface secondWeight = pp::transform(WSF, tfx, mTransformMethod);
				Surface compare = pp::compare(bicubic, first);
				mResultImage = pp::choose(first, second, compare, secondWeight, mMixThreshold*mMixThreshold);
			}
			}
			if (mDiffWithSmoothBicubic)
			{
				pp::BicubicSampler BCS = pp::BicubicSampler(mScaledSrc);
				Surface bicubic = pp::transform(BCS, tfx, mTransformMethod);
				mResultImage = pp::compare(bicubic, mResultImage);
			}

		}
		mResultTexture = gl::Texture::create(mResultImage);
		mResultTexture->setMagFilter(GL_NEAREST);

		//PRINT TIME TAKEN
		double t2 = getElapsedSeconds();
		int ms = (int)((t2 - t1) * 1000);
		mPerfLabel->setText(str(boost::format("Perf: %i ms") % ms));
	}
}

void PixelPunchApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	else if (event.getChar() == ' ')
		mDisplaySource = true;
	else if (event.getChar() == 's')
		saveResultToFile();
}

void PixelPunchApp::keyUp(KeyEvent event)
{
	if (event.getChar() == ' ')
		mDisplaySource = false;
}

void PixelPunchApp::mouseMove(MouseEvent event)
{
	if (mTransformUI.mouseMove(event))
		mResultImage = Surface();
}
void PixelPunchApp::mouseDown(MouseEvent event)
{
	if (mTransformUI.mouseDown(event))
		mResultImage = Surface();
}
void PixelPunchApp::mouseUp(MouseEvent event)
{
	if (mTransformUI.mouseUp(event))
		mResultImage = Surface();
}

void PixelPunchApp::mouseDrag(MouseEvent event)
{
	if (mTransformUI.mouseDrag(event))
		mResultImage = Surface();
}

void PixelPunchApp::update()
{
	validateResultImage();
}

void PixelPunchApp::draw()
{
	vec2 windowSize = getWindowSize();
	gl::setMatricesWindow(windowSize);
	gl::clear(Color(0.1f, 0.1f, 0.1f));
	gl::color(1.0f, 1.0f, 1.0f);

	//scaledSrc
	gl::TextureRef& tex = mDisplaySource ? mPrevTexture : mResultTexture;
	if (mTransformMethod == pp::TransformMethod::TM_IDENTITY && tex)
	{
		gl::pushMatrices();
		float ratio = (float)mSourceImage.getWidth() / (float)tex->getWidth();
		float scale = max<float>(1.0f, mViewScale * ratio);
		gl::scale(vec3(scale, scale, scale));
		vec2 pos = 0.5f * (windowSize / scale - (vec2)tex->getSize());

		gl::draw(tex, pos);
		gl::popMatrices();
	}
	else //mTransformMethod != pp::TransformMethod::TM_IDENTITY)
	{
		if (tex && mResultImage.getData())
		{
			gl::pushMatrices();
			gl::multModelMatrix(mTransformUI.getShapeToView());
			gl::draw(tex, tex->getBounds(), mTransformUI.getBounds());
			gl::popMatrices();
		}
		gl::disableDepthRead();
		gl::disableDepthWrite();
		mTransformUI.setView(windowSize, mViewScale);
		mTransformUI.draw();
	}


	// Draw the interface
	//params::InterfaceGl::draw();
	mGui->draw();
}

void PixelPunchApp::saveResultToFile()
{
	if (mResultImage.getData() != NULL)
	{
		std::vector<std::string> extensions = ImageIo::getWriteExtensions();
		std::string suffix = mScaleOptions[mScaleMethod];
		std::string path = mSourceFileName;
		path.insert(path.find_last_of('.'), suffix);
		std::string savePath = getSaveFilePath(path, extensions).string();
		if (!savePath.empty())
			writeImage(savePath, mResultImage);
	}

}


// This line tells Flint to actually create the application
CINDER_APP(PixelPunchApp, RendererGl);
