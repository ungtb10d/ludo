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

#include "details/detailsrendererimpl.h"
#include "details/detailsutils.h"

#include <substance/framework/renderer.h>
#include <substance/framework/package.h>


SubstanceAir::Renderer::Renderer(const RenderOptions& renderOptions, void* module) :
	mRendererImpl(AIR_NEW(Details::RendererImpl)(renderOptions, module))
{
}


SubstanceAir::Renderer::~Renderer()
{
	AIR_DELETE(mRendererImpl);
}


void SubstanceAir::Renderer::push(GraphInstance& graph)
{
	mRendererImpl->push(graph);
}


void SubstanceAir::Renderer::push(const GraphInstances& graphs)
{
	for (const auto& graph : graphs)
	{
		push(*graph.get());
	}
}


SubstanceAir::UInt SubstanceAir::Renderer::run(UInt runOptions,size_t userData)
{
	return mRendererImpl->run(runOptions,userData);
}


bool SubstanceAir::Renderer::cancel(UInt runUid)
{
	return mRendererImpl->cancel(runUid);
}


void SubstanceAir::Renderer::cancelAll()
{
	mRendererImpl->cancel();
}


bool SubstanceAir::Renderer::isPending(UInt runUid) const
{
	return mRendererImpl->isPending(runUid);
}


void SubstanceAir::Renderer::hold()
{
	mRendererImpl->hold();
}


void SubstanceAir::Renderer::resume()
{
	mRendererImpl->resume();
}


void SubstanceAir::Renderer::flush()
{
	mRendererImpl->flush();
}


void SubstanceAir::Renderer::restoreRenderStates()
{
	mRendererImpl->restoreRenderStates();
}

void SubstanceAir::Renderer::setOptions(const RenderOptions& renderOptions)
{
	mRendererImpl->setOptions(renderOptions);
}


void SubstanceAir::Renderer::setRenderCallbacks(RenderCallbacks* callbacks)
{
	mRendererImpl->setRenderCallbacks(callbacks);
}


bool SubstanceAir::Renderer::switchEngineLibrary(void* module)
{
	return mRendererImpl->switchEngineLibrary(module);
}


SubstanceVersion SubstanceAir::Renderer::getCurrentVersion() const
{
	return mRendererImpl->getCurrentVersion();
}
