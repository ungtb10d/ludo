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

#include <iostream>

#include "platform_ogl3.h"
#include "openglwidget.h"

#include <QApplication>
#include <QOpenGLWidget>
#include <QFile>
#include <QThread>

#include <assert.h>

#define ALG_ERROR_ARGUMENT 1
#define ALG_ERROR_FILE_READING 2
#define ALG_ERROR_SBS_PACKAGE 3

int main(int argc, char** argv) {
	// Loading sbsar file
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <sbsar filepath>\n";
		return ALG_ERROR_ARGUMENT;
	}

	QFile file(argv[1]);
	if (!file.open(QIODevice::ReadOnly))
		return ALG_ERROR_FILE_READING;
	QByteArray archiveData = file.readAll();

	// Setup the application
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QApplication app(argc, argv);

	QSurfaceFormat format;
	format.setMajorVersion(3);
	format.setMinorVersion(3);
	format.setProfile(QSurfaceFormat::CoreProfile);

	OpenglWidget openglWidget;
	openglWidget.setFormat(format);
	openglWidget.setGeometry(30, 30, 256, 256);
	openglWidget.show();

	// Create render context and his surface
	QOpenGLContext * context = new QOpenGLContext;
	context->setShareContext(openglWidget.context());
	context->create();

	QOffscreenSurface *offscreenSurface = new QOffscreenSurface;
	offscreenSurface->setFormat(format);
	offscreenSurface->create();

	// Make substance render multi threaded
	RenderWorker renderWorker(context, offscreenSurface);
	QThread thread;
	context->moveToThread(&thread);
	offscreenSurface->moveToThread(&thread);
	renderWorker.moveToThread(&thread);
	thread.start();

	RenderCallbacks renderCallbacks;
	QObject::connect(
		&renderCallbacks,
		SIGNAL(render(void*, void*)),
		&renderWorker,
		SLOT(render(void*, void*)));

	{
		using namespace SubstanceAir;

		// Parse package (from Archive)
		PackageDesc packagedesc(
			archiveData.data(),
			archiveData.size());

		if (!packagedesc.isValid())
		{
			std::cerr << "Invalid package.\n";
			return ALG_ERROR_SBS_PACKAGE;
		}

		// Instantiate all graphs of the package
		GraphInstances instances;
		instantiate(instances, packagedesc);

		// Create renderer object
		Renderer renderer;
		renderer.setRenderCallbacks(&renderCallbacks);

		// Push graphs instances into renderer
		renderer.push(instances);

		// Render synchronous
		renderer.run();

		// Grab all outputs of all graphs

		// Iterate on all graphs
		for (GraphInstances::const_iterator grite = instances.begin();
			grite != instances.end();
			++grite)
		{
			// Iterate on all outputs
			const GraphInstance::Outputs& outputs = (*grite)->getOutputs();
			for (GraphInstance::Outputs::const_iterator outite = outputs.begin();
				outite != outputs.end();
				++outite)
			{
				// Grab result (auto pointer on RenderResult)
				OutputInstance::Result result((*outite)->grabResult());
				if (result.get()!=NULL && result->isImage()) {
					assert(dynamic_cast<RenderResultImage*>(result.get())->getTexture().textureName != 0);
					openglWidget.pushResult(dynamic_cast<RenderResultImage*>(result.release()));
				}
			}
		}

		std::cout << "Done.\n";
	}

	thread.quit();
	while (thread.isRunning()) { QCoreApplication::processEvents(); }

	return app.exec();
}
