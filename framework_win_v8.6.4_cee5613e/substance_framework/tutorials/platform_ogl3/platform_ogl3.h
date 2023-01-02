/*************************************************************************
* ADOBE CONFIDENTIAL
* ___________________ *
*  Copyright 2020-2022 Adobe
*  All Rights Reserved.
* * NOTICE:  All information contained herein is, and remains
* the property of Adobe and its suppliers, if any. The intellectual
* and technical concepts contained herein are proprietary to Adobe
* and its suppliers and are protected by all applicable intellectual
* property laws, including trade secret and copyright laws.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from Adobe.
**************************************************************************/

#pragma once

#define SUBSTANCE_PLATFORM_OGL3

// Substance Framework include
#include <substance/framework/framework.h>

#include <QOpenGLContext>
#include <QOffscreenSurface>

class RenderWorker : public QObject
{
	Q_OBJECT
public:
	RenderWorker(QOpenGLContext* context, QOffscreenSurface* offscreenSurface) : mContext(context), mOffscreenSurface(offscreenSurface) {}

public Q_SLOTS :
	void render(
		void* renderFunc,
		void *params)
	{
		mContext->makeCurrent(mOffscreenSurface);
		((SubstanceAir::RenderFunction)renderFunc)(params);
		glFinish();
		mContext->doneCurrent();
	}

protected:
	QOpenGLContext* mContext;
	QOffscreenSurface* mOffscreenSurface;
};

struct RenderCallbacks : public QObject, public SubstanceAir::RenderCallbacks
{
	Q_OBJECT
public:
	bool runRenderProcess(
		SubstanceAir::RenderFunction renderFunction,
		void* renderParams)
	{
		Q_EMIT render((void*)renderFunction, renderParams);
		return true;
	}

Q_SIGNALS:
	void render(void* renderFunc, void *params);
};
