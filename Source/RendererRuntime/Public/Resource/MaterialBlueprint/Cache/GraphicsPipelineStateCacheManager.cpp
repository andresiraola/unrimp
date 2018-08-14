/*********************************************************\
 * Copyright (c) 2012-2018 The Unrimp Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCacheManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCompiler.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCache.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Renderer::IGraphicsPipelineStatePtr GraphicsPipelineStateCacheManager::getGraphicsPipelineStateCacheByCombination(uint32_t serializedGraphicsPipelineStateHash, const ShaderProperties& shaderProperties, bool allowEmergencySynchronousCompilation)
	{
		// TODO(co) Asserts whether or not e.g. the material resource is using the owning material resource blueprint
		assert(IResource::LoadingState::LOADED == mMaterialBlueprintResource.getLoadingState());

		// Generate the graphics pipeline state signature
		mTemporaryGraphicsPipelineStateSignature.set(mMaterialBlueprintResource, serializedGraphicsPipelineStateHash, shaderProperties);
		{
			GraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId::const_iterator iterator = mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.find(mTemporaryGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId());
			if (iterator != mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.cend())
			{
				// There's already a pipeline state cache for the pipeline state signature ID
				// -> We don't care whether or not the pipeline state cache is currently using fallback data due to asynchronous complication
				return iterator->second->getGraphicsPipelineStateObjectPtr();
			}
		}

		// Fallback: OK, the pipeline state signature is unknown and we now have to perform more complex and time consuming work. So first, check whether or not this work
		// should be performed asynchronous (usually the case). If asynchronous, we need to return a fallback pipeline state cache while the pipeline state compiler is working.
		GraphicsPipelineStateCache* fallbackGraphicsPipelineStateCache = nullptr;
		GraphicsPipelineStateCompiler& graphicsPipelineStateCompiler = mMaterialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getGraphicsPipelineStateCompiler();
		if (graphicsPipelineStateCompiler.isAsynchronousCompilationEnabled())
		{
			// Asynchronous

			// Look for a suitable already available pipeline state cache which content we can use as fallback while the pipeline state compiler is working. We
			// do this by reducing the shader properties set until we find something, hopefully. In case no fallback can be found we have to switch to synchronous processing.

			// Start with the full shader properties and then clear one shader property after another
			ShaderProperties fallbackShaderProperties(shaderProperties);	// TODO(co) Optimization: There are allocations for vector involved in here, we might want to get rid of this
			ShaderProperties::SortedPropertyVector& sortedFallbackPropertyVector = fallbackShaderProperties.getSortedPropertyVector();
			while (nullptr == fallbackGraphicsPipelineStateCache && !sortedFallbackPropertyVector.empty())
			{
				{ // Remove a fallback shader property
					// Find the most useless shader property, we're going to sacrifice it
					ShaderProperties::SortedPropertyVector::iterator worstHitShaderPropertyIterator = sortedFallbackPropertyVector.end();
					int32_t worstHitVisualImportanceOfShaderProperty = getInvalid<int32_t>();
					ShaderProperties::SortedPropertyVector::iterator iterator = sortedFallbackPropertyVector.begin();
					while (iterator != sortedFallbackPropertyVector.end())
					{
						// Do not remove mandatory shader combination shader properties, at least not inside this pass
						const int32_t visualImportanceOfShaderProperty = mMaterialBlueprintResource.getVisualImportanceOfShaderProperty(iterator->shaderPropertyId);
						if (MaterialBlueprintResource::MANDATORY_SHADER_PROPERTY != visualImportanceOfShaderProperty)
						{
							if (isValid(worstHitVisualImportanceOfShaderProperty))
							{
								// Lower visual importance value = lower probability that someone will miss the shader property
								if (worstHitVisualImportanceOfShaderProperty > visualImportanceOfShaderProperty)
								{
									worstHitVisualImportanceOfShaderProperty = visualImportanceOfShaderProperty;
									worstHitShaderPropertyIterator = iterator;
								}
							}
							else
							{
								worstHitShaderPropertyIterator = iterator;
								worstHitVisualImportanceOfShaderProperty = visualImportanceOfShaderProperty;
							}
						}

						// Next shader property on the to-kill-list, please
						++iterator;
					}

					// Sacrifice our victim
					if (sortedFallbackPropertyVector.end() == worstHitShaderPropertyIterator)
					{
						// No chance, no goats left
						break;
					}
					sortedFallbackPropertyVector.erase(worstHitShaderPropertyIterator);
				}

				// Generate the current fallback graphics pipeline state signature
				GraphicsPipelineStateSignature fallbackGraphicsPipelineStateSignature(mMaterialBlueprintResource, serializedGraphicsPipelineStateHash, fallbackShaderProperties);	// TODO(co) Optimization: There are allocations for vector and map involved in here, we might want to get rid of those
				GraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId::const_iterator iterator = mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.find(fallbackGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId());
				if (iterator != mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.cend())
				{
					// We don't care whether or not the graphics pipeline state cache is currently using fallback data due to asynchronous complication
					fallbackGraphicsPipelineStateCache = iterator->second;
				}
			}

			// If we're here and still not having any fallback graphics pipeline state cache we'll end up with a runtime hiccup, we don't want that
			// -> Kids, don't try this at home: We'll trade the runtime hiccup against a nasty major graphics artifact. If we're in luck no one
			//    will notice it, depends on the situation. A runtime hiccup on the other hand will always be notable. So this trade in here
			//    might not involve our first born.
			if (!allowEmergencySynchronousCompilation && nullptr == fallbackGraphicsPipelineStateCache)
			{
				// TODO(co) Optimization: There are allocations for vector and map involved in here, we might want to get rid of those
				fallbackShaderProperties.clear();
				GraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId::const_iterator iterator = mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.find(GraphicsPipelineStateSignature(mMaterialBlueprintResource, serializedGraphicsPipelineStateHash, fallbackShaderProperties).getGraphicsPipelineStateSignatureId());
				if (iterator != mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.cend())
				{
					// We don't care whether or not the graphics pipeline state cache is currently using fallback data due to asynchronous complication
					fallbackGraphicsPipelineStateCache = iterator->second;
				}
			}
		}

		// Create the new graphics pipeline state cache instance
		GraphicsPipelineStateCache* graphicsPipelineStateCache = new GraphicsPipelineStateCache(mTemporaryGraphicsPipelineStateSignature);
		mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.emplace(mTemporaryGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId(), graphicsPipelineStateCache);

		// If we've got a fallback graphics pipeline state cache then commit the asynchronous pipeline state compiler request now, else we must proceed synchronous (risk of notable runtime hiccups)
		if (nullptr != fallbackGraphicsPipelineStateCache)
		{
			// Asynchronous, the light side
			graphicsPipelineStateCache->mGraphicsPipelineStateObjectPtr = fallbackGraphicsPipelineStateCache->mGraphicsPipelineStateObjectPtr;
			graphicsPipelineStateCache->mIsUsingFallback = true;
			graphicsPipelineStateCompiler.addAsynchronousCompilerRequest(*graphicsPipelineStateCache);
		}
		else
		{
			// Synchronous, the dark side
			graphicsPipelineStateCompiler.instantSynchronousCompilerRequest(mMaterialBlueprintResource, *graphicsPipelineStateCache);
		}

		// Done
		// TODO(co) Mark material cache as dirty
		return graphicsPipelineStateCache->getGraphicsPipelineStateObjectPtr();
	}

	void GraphicsPipelineStateCacheManager::clearCache()
	{
		for (auto& graphicsPipelineStateCacheElement : mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId)
		{
			delete graphicsPipelineStateCacheElement.second;
		}
		mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.clear();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime